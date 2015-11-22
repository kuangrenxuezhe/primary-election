#include "core/item_table.h"
#include "util/buffer.h"

#include "util/util.h"
#include "util/crc32c.h"
#include "glog/logging.h"
#include "proto/record.pb.h"
#include "proto/service.pb.h"

#include <sstream>

namespace rsys {
  namespace news {
    static const fver_t kItemFver(1, 0);
    static const std::string kItemAheadLog = "wal-item";
    static const std::string kItemTable = "table-item.dat";

    static const int32_t kSecondPerHour = 3600;
    static const int32_t kWindowLockSize = 32;

    // 时间校正因子
    static const int32_t kTimeFactor = 2*60;

    class ItemAheadLog: public AheadLog {
      public:
        ItemAheadLog(const std::string& path, const fver_t& fver)
          : AheadLog(path, kItemAheadLog, fver) {
          }
        ~ItemAheadLog() {
        }

      public:
        virtual Status rollback(const std::string& data) {
          return Status::OK();
        }
    };

    ItemTable::ItemTable(const Options& opts)
      : TableBase(opts.work_path, kItemTable, kItemFver), item_window_(NULL), window_lock_(NULL), item_index_(NULL)
    {
      options_ = opts;

      //补足一个对齐位置
      window_size_ = opts.item_hold_time/kSecondPerHour + 1; 
      window_base_ = 0;
      window_time_ = time(NULL) - opts.item_hold_time;

      // 循环列表留一个空位表示结尾     
      item_window_ = new item_list_t[window_size_ + 1];
      window_lock_ = new pthread_rwlock_t[kWindowLockSize];
      for (int32_t i = 0; i < kWindowLockSize; ++i) {
        pthread_rwlock_init(&window_lock_[i], NULL);
      }
      item_index_ = new hash_map_t();
      item_index_->set_empty_key(0UL);
      pthread_mutex_init(&index_lock_, NULL);
    }

    ItemTable::~ItemTable()
    {
      delete [] item_window_;
      for (int32_t i = 0; i < kWindowLockSize; ++i) {
        pthread_rwlock_destroy(&window_lock_[i]);
      }
      delete [] window_lock_;
      delete item_index_;
      pthread_mutex_destroy(&index_lock_);
    }
    
    // 淘汰item数据
    Status ItemTable::eliminate()
    {
      //int index = 0;

      //pthread_rwlock_wrlock(&window_lock_[index%kWindowLockSize]);
      //item_list_t::iterator iter = item_window_[index].begin();
      //for (; iter != item_window_[index].end(); ++iter) {
      //  level_table()->erase((*iter)->item_id);
      //}
      //window_time_ = 0;
      //pthread_rwlock_unlock(&window_lock_[index%kWindowLockSize]);

      return Status::OK();
    }

    // 计算存储在滑窗内的物理位置
    int ItemTable::windowIndex(int32_t ctime)
    {
      int index = (ctime-window_time_)/kSecondPerHour;
      return (window_base_ + index) % window_size_;
    }

    Status ItemTable::addItem(item_info_t* item_info)
    {
      int32_t ctime = time(NULL);

      LogRecord record;
      LogItemInfo log_item_info;

      log_item_info.set_item_id(item_info->item_id);
      log_item_info.set_power(item_info->power);
      log_item_info.set_publish_time(item_info->publish_time);
      log_item_info.set_item_type(item_info->item_type);
      log_item_info.set_picture_num(item_info->picture_num);
      log_item_info.set_click_count(item_info->click_count);
      log_item_info.set_click_time(item_info->click_time);
      log_item_info.set_category_id(item_info->category_id);
      for (map_str_t::iterator iter = item_info->region_id.begin();
          iter != item_info->region_id.end(); ++iter) {
        KeyStr* pair = log_item_info.add_region_id();
        pair->set_key(iter->first);
        pair->set_str(iter->second);
      }
      for (map_str_t::iterator iter = item_info->belongs_to.begin();
          iter != item_info->belongs_to.end(); ++iter) {
        KeyStr* pair = log_item_info.add_belongs_to();
        pair->set_key(iter->first);
        pair->set_str(iter->second);
      }
      record.mutable_record()->PackFrom(log_item_info);

      std::string serialize_item_info = record.SerializeAsString();
      Status status = writeAheadLog(serialize_item_info);
      if (!status.ok()) {
        return status;
      }

      // 判定待保留数据是否越界
      // 若超出则需要淘汰过期数据，否则新数据将插入到列表的末端
      if ((ctime - options_.item_hold_time) - window_time_ > kSecondPerHour) {
        status = eliminate();
        if (!status.ok())
          return status;
      }

      // 若新添加数据两天以前的数据则丢弃 
      if (item_info->publish_time < ctime - options_.new_item_max_age) {
        std::ostringstream oss;

        oss<<"Item too old, id="<<std::hex<<item_info->item_id;
        oss<<", publish_time="<<timeToString(item_info->publish_time);
        oss<<", new_item_max_age="<<options_.new_item_max_age;
        return Status::InvalidArgument(oss.str());
      }
      int index = windowIndex(item_info->publish_time);

      pthread_rwlock_wrlock(&window_lock_[index%kWindowLockSize]);
      addToList(item_window_[index], item_info);
      pthread_rwlock_unlock(&window_lock_[index%kWindowLockSize]);

      return addItemIndex(index, item_info);
    }

    void ItemTable::addToList(item_list_t& item_list, item_info_t* item_info)
    {
      item_list_t::iterator iter = item_list.begin();
      for (; iter != item_list.end(); ++iter) {
        if (item_info->publish_time > (*iter)->publish_time)
          continue;
        item_list.insert(iter, item_info);
        break;
      }
      if (iter == item_list.end())
        item_list.insert(iter, item_info);
    }

    Status ItemTable::addItemIndex(int index, item_info_t* item_info)
    {
      item_index_t item_index;
      Status status = Status::OK();

      item_index.index = index;
      item_index.item_info = item_info;

      LogRecord record;

      pthread_mutex_lock(&index_lock_);
      hash_map_t::iterator iter = item_index_->find(item_info->item_id);
      if (iter == item_index_->end()) {
        if (!item_index_->insert(std::make_pair(item_info->item_id, item_index)).second) {
          std::ostringstream oss;

          oss<<"Insert item failed, id="<<std::hex<<item_info->item_id;
          status = Status::Corruption(oss.str());
        }
      } else {
        // 删除已插入的item
        pthread_rwlock_wrlock(&window_lock_[index%kWindowLockSize]);
        item_list_t::iterator iter_list = item_window_[index].begin();
        for (; iter_list != item_window_[index].end(); ++iter_list) {
          if (iter->second.item_info == *iter_list) {
            item_window_[index].erase(iter_list);
            break;
          }
        }
        pthread_rwlock_unlock(&window_lock_[index%kWindowLockSize]);
        iter->second.item_info = item_info;
      }
      pthread_mutex_unlock(&index_lock_);

      return status;
    }

    Status ItemTable::updateAction(const action_t& action)
    {
      LogRecord record;
      LogAction log_action;

      log_action.set_item_id(action.item_id);
      log_action.set_action(action.action);
      log_action.set_action_time(action.action_time);
      log_action.set_dislike_reason(action.dislike_reason);
      record.mutable_record()->PackFrom(log_action);

      std::string serialize_action = record.SerializeAsString();
      Status status = writeAheadLog(serialize_action);
      if (!status.ok()) {
        return status;
      }

      pthread_mutex_lock(&index_lock_);
      hash_map_t::iterator iter = item_index_->find(action.item_id);

      if (iter == item_index_->end()) {
        std::ostringstream oss;

        pthread_mutex_lock(&index_lock_);
        // 点击了已淘汰的数据则不记录用户点击
        oss<<"Not found item, id=0x"<<std::hex<<action.item_id;
        return Status::InvalidArgument(oss.str());
      } 
      pthread_rwlock_wrlock(&window_lock_[iter->second.index%kWindowLockSize]);
      iter->second.item_info->click_count++;
      iter->second.item_info->click_time = time(NULL);
      pthread_rwlock_unlock(&window_lock_[iter->second.index%kWindowLockSize]);

      pthread_mutex_unlock(&index_lock_);
      return Status::OK();
    }

    Status ItemTable::queryCandidateSet(const query_t& query, candidate_set_t& candset)
    {
      // 由于添加item和淘汰item是并行的
      // 有可能在添加一瞬间window_time发生变更,使得插入的item有可能后移
      // 通过时间校正因子来弥补item后移,可能造成数据丢失的问题
      int start_index = windowIndex(query.start_time);
      int end_index = windowIndex(query.end_time + kTimeFactor) + 1;

      for (int i = start_index; i < end_index; ++i) {
        pthread_rwlock_wrlock(&window_lock_[i%kWindowLockSize]);
        item_list_t::iterator iter = item_window_[i].begin();
        for (; iter != item_window_[i].end(); ++iter) {
          if ((*iter)->publish_time < query.start_time
              || (*iter)->publish_time > query.end_time) {
            continue;
          }
          candidate_t cand;

          cand.item_id = (*iter)->item_id;
          cand.power = (*iter)->power;
          cand.publish_time = (*iter)->publish_time;
          cand.picture_num = (*iter)->picture_num;
          cand.category_id = (*iter)->category_id;
          candset.push_back(cand);
        }
        pthread_rwlock_unlock(&window_lock_[i%kWindowLockSize]);
      }
      return Status::OK();
    }

    AheadLog* ItemTable::createAheadLog() 
    {
      return new ItemAheadLog(options_.work_path, kItemFver);
    }

    Status ItemTable::loadData(const std::string& data)
    {
      LogRecord record;
      LogItemInfo log_item_info;

      if (!record.ParseFromString(data)) {
        return Status::Corruption("Parse item_info failed");
      }
      record.record().UnpackTo(&log_item_info);

      item_info_t* item_info = new item_info_t;

      item_info->item_id = log_item_info.item_id();
      item_info->click_count = log_item_info.click_count();
      item_info->click_time = log_item_info.click_time();
      item_info->publish_time = log_item_info.publish_time();
      item_info->power = log_item_info.power();
      item_info->item_type = log_item_info.item_type();
      item_info->picture_num = log_item_info.picture_num();
      item_info->category_id = log_item_info.category_id();
      for (int i = 0; i < log_item_info.region_id_size(); ++i) {
        const KeyStr& pair = log_item_info.region_id(i);
        item_info->region_id.insert(std::make_pair(pair.key(), pair.str()));
      }
      for (int i = 0; i < log_item_info.belongs_to_size(); ++i) {
        const KeyStr& pair = log_item_info.belongs_to(i);
        item_info->belongs_to.insert(std::make_pair(pair.key(), pair.str()));
      }
      int index = windowIndex(item_info->publish_time);

      addToList(item_window_[index], item_info);
      return addItemIndex(index, item_info);  
    }

    Status ItemTable::dumpToFile(const std::string& temp_name)
    {
      TableFileWriter writer(temp_name);

      Status status = writer.create(kItemFver);
      if (!status.ok()) {
        return status;
      }
      std::string serialized_data;

      for (int i = window_base_; i < window_base_ + window_size_; ++i) {
        int index = i%window_size_;

        pthread_rwlock_rdlock(&window_lock_[index%kWindowLockSize]);
        item_list_t::iterator iter = item_window_[index].begin();
        for (; iter != item_window_[index].end(); ++iter) {
          LogRecord record;
          LogItemInfo log_item_info;

          log_item_info.set_item_id((*iter)->item_id);
          log_item_info.set_power((*iter)->power);
          log_item_info.set_item_type((*iter)->item_type);
          log_item_info.set_publish_time((*iter)->publish_time);
          log_item_info.set_click_count((*iter)->click_count);
          log_item_info.set_click_time((*iter)->click_time);
          log_item_info.set_picture_num((*iter)->picture_num);
          log_item_info.set_category_id((*iter)->category_id);
          for (map_str_t::const_iterator citer = (*iter)->region_id.begin();
              citer != (*iter)->region_id.end(); ++citer) {
            KeyStr* pair = log_item_info.add_region_id();
            pair->set_key(citer->first);
            pair->set_str(citer->second);
          }
          for (map_str_t::const_iterator citer = (*iter)->belongs_to.begin();
              citer != (*iter)->belongs_to.end(); ++citer) {
            KeyStr* pair = log_item_info.add_belongs_to();
            pair->set_key(citer->first);
            pair->set_str(citer->second);
          }
          record.mutable_record()->PackFrom(log_item_info);

          std::string serialized_data = record.SerializeAsString();
          status = writer.write(serialized_data);
          if (!status.ok()) {
            pthread_rwlock_unlock(&window_lock_[index%kWindowLockSize]);
            writer.close();
            return status;
          }
        }
        pthread_rwlock_unlock(&window_lock_[index%kWindowLockSize]);
      }
      writer.close();

      return Status::OK();
    }

    // 查询item
    Status ItemTable::queryItem(item_info_t& item_info)
    {
      pthread_mutex_lock(&index_lock_);
      hash_map_t::iterator iter = item_index_->find(item_info.item_id);
      if (iter == item_index_->end()) {
        pthread_mutex_unlock(&index_lock_);
        std::ostringstream oss;

        oss<<std::hex<<"item_id=0x"<<item_info.item_id;
        return Status::NotFound(oss.str());
      }
      item_info.item_id = iter->second.item_info->item_id;
      item_info.click_count = iter->second.item_info->click_count;
      item_info.click_time = iter->second.item_info->click_time;
      item_info.publish_time = iter->second.item_info->publish_time;
      item_info.power = iter->second.item_info->power;
      item_info.item_type = iter->second.item_info->item_type;
      item_info.picture_num = iter->second.item_info->picture_num;
      item_info.category_id = iter->second.item_info->category_id;
      item_info.region_id = iter->second.item_info->region_id;
      item_info.belongs_to = iter->second.item_info->belongs_to;

      pthread_mutex_unlock(&index_lock_);
      return Status::OK();
    }
  }; // namespace news
}; // namespace rsys
