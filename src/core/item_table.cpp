#include "core/item_table.h"

#include "utils/buffer.h"
#include "utils/util.h"
#include "utils/crc32c.h"
#include "glog/logging.h"
#include "proto/supplement.pb.h"
#include "proto/service.pb.h"

namespace souyue {
  namespace recmd {
    static const fver_t kItemFver(1, 0);

    static const char kLogTypeItem   = 'I';
    static const char kLogTypeAction = 'A';

    static const char kTableTypeNormal = 'N';
    static const char kTableTypeTop    = 'T';

    static const int32_t kSecondPerHour = 3600;
    static const int32_t kWindowLockSize = 32;
    // 时间校正因子
    static const int32_t kTimeFactor = 2*60;

    static const std::string kItemAheadLog = "wal-item";
    static const std::string kItemTable = "table-item.dat";

    class ItemAheadLog: public AheadLog {
      public:
        ItemAheadLog(ItemTable* item_table, const std::string& path)
          : AheadLog(path, kItemAheadLog, kItemFver), item_table_(item_table) {
          }
        ~ItemAheadLog() {
        }

      public:
        virtual Status rollback(const std::string& data) {
          if (data.length() <= 1) { // 数据大小至少一个字节，表示类型
            return Status::Corruption("Invalid user info data");
          }
          Status status = Status::OK();
          const char* c_data = data.c_str();

          if (kLogTypeAction == data[0]) {
            Action log_action;

            if (!log_action.ParseFromArray(c_data + 1, data.length() - 1)) {
              return Status::Corruption("Parse item action");
            }
            action_t action;

            glue::structed_action(log_action, action);
            status = item_table_->updateAction(log_action.user_id(), action);
            if (!status.ok()) {
              LOG(WARNING) << status.toString();
            }
            return Status::OK();
          } else if (kLogTypeItem == data[0]) {
            Item log_item;

            if (!log_item.ParseFromArray(c_data + 1, data.length() -1 )) {
              return Status::Corruption("Parse item");
            }

            if (item_table_->isObsolete(log_item.publish_time())) {
              return Status::InvalidData("Obsolete item, item_id=", log_item.item_id(), 
                  ", publish_time=", timeToString(log_item.publish_time()));
            }
            item_info_t* item_info = new item_info_t;

            glue::structed_item(log_item, *item_info);
            Status status = item_table_->addItem(item_info);
            if (!status.ok()) {
              delete item_info;
              LOG(WARNING) << status.toString();
            }
            return Status::OK();
          }
          return Status::InvalidData("Invalid item data type, type=", data[0]);
        }
      private:
        ItemTable* item_table_;
    };

    ItemTable::ItemTable(const ModelOptions& opts)
      : TableBase(opts.work_path, kItemTable, kItemFver)
      , item_window_(NULL), window_lock_(NULL), item_index_(NULL)
    {
      options_ = opts;
      is_eliminating_ = false;

      if (opts.new_item_max_age > opts.item_hold_time)
        options_.new_item_max_age = opts.item_hold_time;

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
      pthread_rwlock_init(&top_lock_, NULL);

      item_index_ = new hash_map_t();
      item_index_->set_empty_key(0UL);
      item_index_->set_deleted_key(0xFFFFFFFFFFFFFFFFUL); 
      pthread_mutex_init(&index_lock_, NULL);
    }

    ItemTable::~ItemTable()
    {
      delete [] item_window_;
      for (int32_t i = 0; i < kWindowLockSize; ++i) {
        pthread_rwlock_destroy(&window_lock_[i]);
      }
      pthread_rwlock_destroy(&top_lock_);

      delete [] window_lock_;
      delete item_index_;
      pthread_mutex_destroy(&index_lock_);
    }

    // 淘汰item数据
    Status ItemTable::eliminate()
    {
      if (is_eliminating_) {
        return Status::OK();
      }
      int32_t ctime = time(NULL);

      // 判定待保留数据是否越上界
      // 若超出则需要淘汰过期数据，否则新数据将插入到列表的末端
      if ((ctime - (window_time_ + options_.item_hold_time)) > kSecondPerHour) {
        if (is_eliminating_) { // double check
          return Status::OK();
        }
        is_eliminating_ = true;
      }

      pthread_rwlock_wrlock(&top_lock_);
      item_list_t::iterator iter = item_top_.begin();
      while (iter != item_top_.end()) {
        if (isObsoleteTop((*iter)->publish_time, ctime)) {
          delete (*iter);
          item_top_.erase(iter++);
        } else {
          ++iter;
        }
      }
      pthread_rwlock_unlock(&top_lock_);
      for (int index = window_base_; window_time_ < ctime - options_.item_hold_time; 
          index = (index + 1)%window_size_) {
        pthread_rwlock_wrlock(&window_lock_[index%kWindowLockSize]);
        iter = item_window_[index].begin();
        for (; iter != item_window_[index].end(); ++iter) {
          pthread_mutex_lock(&index_lock_);
          item_index_->erase((*iter)->item_id);
          pthread_mutex_unlock(&index_lock_);
          delete (*iter);
          item_window_[index].erase(iter++);
        }
        pthread_rwlock_unlock(&window_lock_[index%kWindowLockSize]);

        window_base_ = (window_base_ + 1)%window_size_;
        window_time_ += kSecondPerHour;
      }
      is_eliminating_ = false;

      return Status::OK();
    }

    bool ItemTable::isObsolete(int32_t publish_time) 
    {
      return publish_time < window_time_ ? true:false;
    }

    bool ItemTable::isObsoleteTop(int32_t publish_time, int32_t ctime)
    {
      return publish_time < ctime - options_.top_item_max_age ? true:false;
    }

    // 计算存储在滑窗内的物理位置
    int ItemTable::windowIndex(int32_t ctime)
    {
      int index = (ctime-window_time_)/kSecondPerHour;
      return (window_base_ + index) % window_size_;
    }

    // 添加新闻数据
    Status ItemTable::addItem(const Item& item)
    {
      std::string serialized_item;

      eliminate(); // 淘汰过期数据

      serialized_item.append(1, kLogTypeItem);
      if (!item.AppendToString(&serialized_item)) {
        return Status::Corruption("Serialize item, item_id=", item.item_id());
      }

      Status status = writeAheadLog(serialized_item);
      if (!status.ok()) {
        return status;
      }
      item_info_t* item_info = new item_info_t; 

      glue::structed_item(item, *item_info); 
      if (item.top_info().top_type() == TOP_TYPE_NONE) {
        status = addItem(item_info);
      } else {
        status = addItemTop(item_info);
      }

      if (!status.ok()) {
        delete item_info;
        return status;
      }
      return Status::OK();
    }

    // 用户操作状态更新
    Status ItemTable::updateAction(const Action& action)
    {
      std::string serialized_action;

      serialized_action.append(1, kLogTypeAction);
      if (!action.AppendToString(&serialized_action)) {
        return Status::Corruption("Serialize action, user_id=", action.user_id(), 
            ", item_id=", action.item_id());
      }

      Status status = writeAheadLog(serialized_action);
      if (!status.ok()) {
        return status;
      }
      action_t user_action;
      
      glue::structed_action(action, user_action);
      return updateAction(action.item_id(), user_action);
    }

    Status ItemTable::addItem(item_info_t* item_info)
    {
      int32_t ctime = time(NULL) + kSecondPerHour;

      // 若新添加数据两天以前的数据则丢弃 
      if (item_info->publish_time < ctime - options_.new_item_max_age) {
        return Status::InvalidData("Item too old, id=", item_info->item_id, ", publish_time=", 
            timeToString(item_info->publish_time), ", new_item_max_age=", options_.new_item_max_age);
      }

      if (item_info->publish_time > ctime) {
        return Status::InvalidData("Item ahead of time, id=", item_info->item_id, ", publish_time=", 
            timeToString(item_info->publish_time));
      }
      assert(item_info->publish_time >= window_time_);

      int index = windowIndex(item_info->publish_time);
      assert(index >= 0);

      pthread_rwlock_wrlock(&window_lock_[index%kWindowLockSize]);
      addToList(item_window_[index], item_info);
      pthread_rwlock_unlock(&window_lock_[index%kWindowLockSize]);

      return addItemIndex(index, item_info);
    }

    Status ItemTable::addItemTop(item_info_t* item_info)
    {
      pthread_rwlock_wrlock(&top_lock_);
      addToList(item_top_, item_info); 
      pthread_rwlock_unlock(&top_lock_);

      return addItemIndex(-1, item_info);
    }

    void ItemTable::addToList(item_list_t& item_list, item_info_t* item_info)
    {
      // 按照时间倒序排序，访问时时间倒序访问，这样在获取时间区段内
      // 指定数量的数据后可以提前终止
      item_list_t::iterator iter = item_list.begin();
      for (; iter != item_list.end(); ++iter) {
        if (item_info->publish_time < (*iter)->publish_time)
          continue;
        item_list.insert(iter, item_info);
        break;
      }
      if (iter == item_list.end())
        item_list.insert(iter, item_info);
    }


    void ItemTable::eraseFromList(item_list_t& item_list, item_info_t* item_info)
    {
      item_list_t::iterator iter = item_list.begin();
      for (; iter != item_list.end(); ++iter) {
        if ((*iter) != item_info) {
          continue;
        }
        delete *iter;
        item_list.erase(iter);
        return;
      }
      // 没有找到删除节点，不应该到达
      assert(item_info->item_id == -1);
    }

    Status ItemTable::addItemIndex(int index, item_info_t* item_info)
    {
      item_index_t item_index;

      item_index.index = index;
      item_index.item_info = item_info;
      // debug
      if (item_info->item_id == 32341212623558882) {
        item_info->item_id = item_info->item_id;
      }

      pthread_mutex_lock(&index_lock_);
      hash_map_t::iterator iter = item_index_->find(item_info->item_id);
      if (iter == item_index_->end()) {
        if (!item_index_->insert(std::make_pair(item_info->item_id, item_index)).second) {
          pthread_mutex_unlock(&index_lock_);
          return Status::Corruption("Insert item failed, id=", item_info->item_id);
        }
      } else {
        if (index < 0) {
          pthread_rwlock_wrlock(&top_lock_);
          item_info->click_count = iter->second.item_info->click_count;
          eraseFromList(item_top_, iter->second.item_info);
          pthread_rwlock_unlock(&top_lock_);
        } else {
          // 删除已插入的item
          pthread_rwlock_wrlock(&window_lock_[iter->second.index%kWindowLockSize]);
          item_info->click_count = iter->second.item_info->click_count;
          eraseFromList(item_window_[iter->second.index], iter->second.item_info);
          pthread_rwlock_unlock(&window_lock_[iter->second.index%kWindowLockSize]);

          iter->second.index = index;
          iter->second.item_info = item_info;
        }
      }
      pthread_mutex_unlock(&index_lock_);

      return Status::OK();
    }

    Status ItemTable::updateAction(uint64_t item_id, const action_t& action)
    {
      pthread_mutex_lock(&index_lock_);
      hash_map_t::iterator iter = item_index_->find(item_id);
      if (iter == item_index_->end()) {
        pthread_mutex_unlock(&index_lock_);
        // 点击了已淘汰的数据则不记录用户点击
        return Status::NotFound("item_id=", item_id);
      } 
      if (iter->second.index < 0) {
        pthread_rwlock_wrlock(&top_lock_);
        iter->second.item_info->click_count++;
        if (action.action_time > iter->second.item_info->click_time)
          iter->second.item_info->click_time = action.action_time;
        pthread_rwlock_unlock(&top_lock_);
      } else {
        pthread_rwlock_wrlock(&window_lock_[iter->second.index%kWindowLockSize]);
        iter->second.item_info->click_count++;
        if (action.action_time > iter->second.item_info->click_time)
          iter->second.item_info->click_time = action.action_time;
        pthread_rwlock_unlock(&window_lock_[iter->second.index%kWindowLockSize]);
      }
      pthread_mutex_unlock(&index_lock_);

      return Status::OK();
    }

    bool ItemTable::isBelongsTo(uint64_t region_id, const map_pair_t& regions)
    {
      return regions.find(region_id) == regions.end() ? false:true;
    }

    Status ItemTable::queryCandidateSet(const query_t& query, candidate_set_t& candset)
    {
      if (query.end_time < window_time_)
        return Status::OK();

      int32_t start_time = query.start_time;
      if (query.start_time < window_time_)
        start_time = window_time_;

      item_list_t::iterator iter;
      if (query.item_type == kNormalItem) {
        pthread_rwlock_rdlock(&top_lock_);
        for (iter = item_top_.begin(); iter != item_top_.end(); ++iter) {
          candset.push_back(*(*iter));
        }
        pthread_rwlock_unlock(&top_lock_);
      }

      // 由于添加item和淘汰item是并行的
      // 有可能在添加一瞬间window_time发生变更,使得插入的item有可能后移
      // 通过时间校正因子来弥补item后移,可能造成数据丢失的问题
      int start_index = windowIndex(start_time);
      int end_index = windowIndex(query.end_time + kTimeFactor);
      int logic_end_index = end_index;

      if (logic_end_index < start_index) {
        logic_end_index += window_size_;
      }
      assert(start_index >= 0 && start_index <= logic_end_index);
      for (int i = logic_end_index; i >= start_index; --i) {
        int idx = i%window_size_;

        pthread_rwlock_rdlock(&window_lock_[idx%kWindowLockSize]);
        iter = item_window_[idx].begin();
        for (; iter != item_window_[idx].end(); ++iter) {
          if ((*iter)->publish_time > query.end_time)
            continue;

          if ((*iter)->publish_time < query.start_time)
            break;

          if ((*iter)->item_type != query.item_type)
            continue;

          if (query.region_id != kInvalidRegionID) {
            if (!isBelongsTo(query.region_id, (*iter)->region_id))
              continue;
          }
          candset.push_back(candidate_t(*(*iter)));
        }
        pthread_rwlock_unlock(&window_lock_[idx%kWindowLockSize]);
      }
      return Status::OK();
    }

    AheadLog* ItemTable::createAheadLog() 
    {
      return new ItemAheadLog(this, options_.work_path);
    }

    Status ItemTable::loadData(const std::string& data)
    {
      int32_t ctime = time(NULL);
      ItemInfo log_item_info;
      const char* c_data = data.c_str();

      if (!log_item_info.ParseFromArray(c_data + 1, data.length() - 1)) {
        return Status::Corruption("Parse item info");
      }
      if (kTableTypeTop == data[0]) {
        if (isObsoleteTop(log_item_info.publish_time(), ctime)) {
          return Status::InvalidData("Obsolete top item, item_id=", log_item_info.item_id(), 
              ", publish_time=", timeToString(log_item_info.publish_time()));
        }
        item_info_t* item_info = new item_info_t;

        glue::structed_item_info(log_item_info, *item_info);
        Status status = addItemTop(item_info);
        if (!status.ok()) {
          delete item_info;
        }
        return status;
      } else if (kTableTypeNormal == data[0]) {
        if (isObsolete(log_item_info.publish_time())) {
          return Status::InvalidData("Obsolete item, item_id=", log_item_info.item_id(), 
              ", publish_time=", timeToString(log_item_info.publish_time()));
        }
        item_info_t* item_info = new item_info_t;

        glue::structed_item_info(log_item_info, *item_info);
        int index = windowIndex(item_info->publish_time);

        addToList(item_window_[index], item_info);
        Status status = addItemIndex(index, item_info);  
        if (!status.ok()) {
          delete item_info;
        }
        return status;
      }
      return Status::InvalidData("Invalid user data type, type=", data[0]);
    }

    Status ItemTable::dumpToFile(const std::string& temp_name)
    {
      TableFileWriter writer(temp_name);

      Status status = writer.create(kItemFver);
      if (!status.ok()) {
        return status;
      }
      std::string serialized_data;

      pthread_rwlock_rdlock(&top_lock_);
      item_list_t::iterator iter = item_top_.begin();
      for (; iter != item_top_.end(); ++iter) {
        std::string data;
        ItemInfo log_item_info;

        data.append(1, kTableTypeTop);
        glue::proto_item_info(*(*iter), log_item_info);
        if (!log_item_info.AppendToString(&data)) {
          pthread_rwlock_unlock(&top_lock_);
          writer.close();
          return Status::Corruption("Serialize item, item_id=", log_item_info.item_id());
        }
      }
      pthread_rwlock_unlock(&top_lock_);

      for (int i = window_base_; i < window_base_ + window_size_; ++i) {
        int index = i%window_size_;

        pthread_rwlock_rdlock(&window_lock_[index%kWindowLockSize]);
        iter = item_window_[index].begin();
        for (; iter != item_window_[index].end(); ++iter) {
          std::string data;
          ItemInfo log_item_info;

          data.append(1, kTableTypeNormal);
          glue::proto_item_info(*(*iter), log_item_info);
          if (!log_item_info.AppendToString(&data)) {
            pthread_rwlock_unlock(&window_lock_[index%kWindowLockSize]);
            writer.close();
            return Status::Corruption("Serialize item, item_id=", log_item_info.item_id());
          }
          status = writer.write(data);
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
    Status ItemTable::queryItem(uint64_t item_id, item_info_t& item_info)
    {
      pthread_mutex_lock(&index_lock_);
      hash_map_t::iterator iter = item_index_->find(item_id);
      if (iter == item_index_->end()) {
        pthread_mutex_unlock(&index_lock_);
        return Status::NotFound("item_id=", item_id);
      }
      item_info = *iter->second.item_info;
      pthread_mutex_unlock(&index_lock_);
      return Status::OK();
    }
  }; // namespace news
}; // namespace rsys
