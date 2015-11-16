#include "core/item_table.h"
#include "util/buffer.h"

#include "util/util.h"
#include "util/crc32c.h"
#include "glog/logging.h"
#include "proto/news_rsys.pb.h"

#include <sstream>

namespace rsys {
  namespace news {
    static const uint32_t kItemTableMajorVersion = 0;
    static const uint32_t kItemTableMinorVersion = 1;

    static const int32_t kSecondPerHour = 3600;
    static const int32_t kWindowLockSize = 32;

    // 时间校正因子
    static const int32_t kTimeFactor = 2*60;

    // 允许新增Item的最大时间, 默认最大两天
    static int32_t kItemMaxAge = 2*24*60*60;

    class ActionUpdater: public LevelTable<uint64_t, item_index_t>::Updater{
      public:
        ActionUpdater(const action_t& item_action)
          : item_action_(item_action) {
        }
        virtual ~ActionUpdater() {
        }
        virtual bool update(item_index_t* item_index) {
          if (item_action_.action == CLICK) {
            item_index->click_count += 1;
            item_index->click_time = item_action_.action_time;
          }
          return true;
        }
        virtual item_index_t* clone(item_index_t* value) {
          return NULL;
        }
      private:
        const action_t& item_action_;
    };

    ItemTable::ItemTable(const Options& opts)
    {
      if (opts.new_item_max_age > 0)
        kItemMaxAge = opts.new_item_max_age;
    }

    ItemTable::~ItemTable()
    {
    }

    // 保存用户表
    Status ItemTable::flushTable()
    {
      
      level_table_t::Iterator iter  = level_table_.snapshot();
      if (!iter.valid()) {
        return Status::Corruption("depth not enough");
      }
      fver_t fver;
      std::string tabname = options().path + "/item.tab.tmp";
      LevelFileWriter writer(tabname);

      fver.flag = kVersionFlag;
      fver.major = kItemTableMajorVersion;
      fver.minor = kItemTableMinorVersion;

      Status status = writer.create(fver);
      if (!status.ok()) {
        return status;
      }
      std::string serialized_str;

      while (iter.hasNext()) {
        if (!serializeTo(iter.key(), iter.value(), serialized_str)) {
          LOG(WARNING) << "";
        } else {
          status = writer.write(serialized_str);
          if (!status.ok()) {
            writer.close();
            return status;
          }
        }
        iter.next();
      }

      return apply();
    }

    // 加载用户表
    Status ItemTable::loadTable()
    {
      fver_t fver;
      std::string tab_name = options().path + "/item.tab";
      LevelFileReader reader(tab_name);

      Status status = reader.open(fver);
      if (!status.ok()) {
        return status;
      }

      if (fver.flag != kVersionFlag) {
        std::ostringstream oss;
        oss<<"Invalid file version flag: expect="<<kVersionFlag
          <<", real="<<fver.flag;
        return Status::IOError(oss.str());
      }
      // 可以做多版本处理
     
      std::string data;
      do {
        status = reader.read(data);
        if (status.ok()) {
          if (parseFrom(data, item_index)) {
          }
        }
      }
      while (status.ok());
      return Status::OK();
    }

    // 淘汰用户，包括已读，不喜欢和推荐信息
    Status ItemTable::eliminate(int32_t hold_time)
    {
      int index = 0;

      pthread_rwlock_wrlock(&window_rwlock_[index%kWindowLockSize]);
      item_list_t::iterator iter = item_window_[index].begin();
      for (; iter != item_window_[index].end(); ++iter) {
        level_table_.erase((*iter)->item_id);
      }
      window_time_ = 0;
      pthread_rwlock_unlock(&window_rwlock_[index%kWindowLockSize]);

      return Status::OK();
    }

    int ItemTable::getWindowIndex(int32_t ctime)
    {
      return (ctime-window_time_)/kSecondPerHour;
    }

    Status ItemTable::addItem(item_info_t* item_info)
    {
      int32_t ctime = time(NULL);

      // 若过来的是两天以前的数据则丢弃 
      if (item_info->publish_time < ctime - kItemMaxAge) {
        std::ostringstream oss;
        oss << "Item too old: itemid=" << std::hex << item_info->item_id 
          << ", publish_time=" << timeToString(item_info->publish_time) 
          << ", item_max_age=" << kItemMaxAge;
        return Status::InvalidArgument(oss.str());
      }
      int index = getWindowIndex(item_info->publish_time);

      pthread_rwlock_wrlock(&window_rwlock_[index%kWindowLockSize]);
      item_list_t::iterator iter = item_window_[index].begin();
      for (; iter != item_window_[index].end(); ++iter) {
        if (item_info->publish_time > (*iter)->publish_time)
          continue;
        item_window_[index].insert(iter, item_info);
        break;
      }
      if (iter == item_window_[index].end())
        item_window_[index].insert(iter, item_info);
      pthread_rwlock_unlock(&window_rwlock_[index%kWindowLockSize]);

      return addItemIndex(item_info);
    }

    Status ItemTable::addItemIndex(item_info_t* item_info)
    {
      item_index_t* item_index = new item_index_t;

      item_index->item_info = item_info;
      item_index->click_count = 1;
      item_index->click_time = time(NULL);

      if (level_table_.add(item_info->item_id, item_index))
        return Status::OK();
      delete item_info;

      std::ostringstream oss;

      oss << "Cann`t add to level table: item_id=" << item_info->item_id;
      return Status::InvalidArgument(oss.str());
    }

    Status ItemTable::updateAction(const action_t& item_action)
    {
      if (level_table_.find(item_action.item_id)) {
        std::ostringstream oss;

        // 点击了已淘汰的数据则不记录用户点击
        oss << "Obsolete item: " << std::hex << item_action.item_id;
        return Status::InvalidArgument(oss.str());
      } 
      ActionUpdater updater(item_action);

      if (level_table_.update(item_action.item_id, updater))
        return Status::OK();

      std::stringstream oss;

      oss << "Cann`t update item action: item_id=" << item_action.item_id;
      return Status::InvalidArgument(oss.str());
    }

    /*
    int ItemTable::syncSlipWindow(const char* fullpath)
    {
      uint32_t crc = 0;
      FILE* wfd = fopen(fullpath, "wb");

      int serlen = 0;
      buffer_t* buffer = newBuffer(1<<20);
      if (NULL == wfd) {
        LOG(ERROR) << "open file " << fullpath << " failed: " << strerror(errno);
        goto FAILED;
      }
      if (!fwrite(&kVersion, sizeof(uint32_t), 1, wfd)) {
        LOG(ERROR) << "write file " << fullpath << " failed: " << strerror(errno);
        goto FAILED;
      }
      crc = crc::extend(crc, (char *)&crc, sizeof(uint32_t));

      for (int i = window_base_; i - window_base_ < window_size_; ++i) {
        int index = i - window_base_;

        pthread_rwlock_wrlock(&window_rwlock_[index%WINDOW_RWLOCK_SIZE]);
        if (0 == item_window_[index].size()) {
          pthread_rwlock_unlock(&window_rwlock_[index%WINDOW_RWLOCK_SIZE]);
          continue;
        }
        item_list_t::iterator iter = item_window_[index].begin();
        for (; iter != item_window_[index].end(); ++iter) {
          serlen = serializeItemInfo(*iter, buffer);
          if (0 == serlen)
            continue;
          if (!fwrite(buffer->buffer, serlen, 1, wfd)) {
            LOG(ERROR) << "write file " << fullpath << " failed: " << strerror(errno);
            pthread_rwlock_unlock(&window_rwlock_[index%WINDOW_RWLOCK_SIZE]);
            goto FAILED;
          }
        }
        pthread_rwlock_unlock(&window_rwlock_[index%WINDOW_RWLOCK_SIZE]);
      }
      freeBuffer(buffer);
      fclose(wfd);
      return 0;
FAILED:
      freeBuffer(buffer);
      if (wfd)
        fclose(wfd);
      return -1;
    }

    int ItemTable::loadSlipWindow(const char* fullpath)
    {
      buffer_t* buffer = newBuffer(1<<20);
      item_info_t* item_info = new item_info_t;
      uint32_t version, valid_crc, crc = 0, total = 0;

      FILE* rfd = fopen(fullpath, "rb");
      if (NULL == rfd) {
        LOG(ERROR) << "open file " << fullpath << " failed: " << strerror(errno);
        goto FAILED;
      }
      if (!fread(&version, sizeof(uint32_t), 1, rfd)) {
        LOG(ERROR) << "read file " << fullpath << " failed: " << strerror(errno);
        goto FAILED;
      }
      // 验证flag
      if ((version&0xFF000000) != (kVersion&0xFF000000)) {
        LOG(ERROR) << std::hex << "file flag failed: expect=" << ((kVersion&0xFF000000)>>24)
          << ", real=" << ((version&0xFF000000)>>24);
        goto FAILED;
      }
      crc = crc::extend(crc, (char*)&crc, sizeof(uint32_t));

      while (fread(&length, sizeof(int32_t), 1, rfd)) {
        crc = crc::extend(crc, (char *)&length, sizeof(int32_t));

        if (length <= 0)
          break;
        if (length > buffer->maxlen)
          buffer = renewBuffer(buffer, length);
        if (!fread(buffer->buffer, length, 1, rfd)) {
          LOG(ERROR) << "read file " << fullpath << " failed: " << strerror(errno);
          goto FAILED;
        }
        crc = crc::extend(crc, buffer->buffer, length);

        item_info_t* parsed_item_info = parseItemInfo(buffer, length, item_info);
        if (NULL == parsed_item_info) {
          LOG(WARNING) << "parse the " << total << " failed.";
        } else {
          if (addItem(parsed_item_info)) {
            LOG(WARNING) << std::hex << "add item: " << parsed_item_info->item_id << " failed.";
          }
        }
        total++;
      }

      if (!fread(&valid_crc, sizeof(uint32_t), 1, rfd)) {
        LOG(ERROR) << "read file " << fullpath << " failed: " << strerror(errno);
        goto FAILED;
      }
      if (valid_crc != crc::mask(crc)) {
        LOG(ERROR) << std::hex << "invalid crc: expect=" << valid_crc 
          << ", real=" << crc::mask(crc);
      }
 
      delete item_info;
      freeBuffer(buffer);
      fclose(rfd);
      return 0;
FAILED:
      delete item_info;
      freeBuffer(buffer);
      if (rfd)
        fclose(rfd);
      return -1;

    }
  */
    Status ItemTable::queryCandidateSet(const query_t& query, candidate_set_t& candset)
    {
      // 由于添加item和淘汰item是并行的
      // 有可能在添加一瞬间window_time发生变更,使得插入的item有可能前移
      // 这里通过使用时间校正因子来弥补该种缺陷
      int start_index = getWindowIndex(query.start_time);
      int end_index = getWindowIndex(query.end_time + kTimeFactor) + 1;

      for (int i = start_index; i < end_index; ++i) {
        pthread_rwlock_wrlock(&window_rwlock_[i%kWindowLockSize]);
        item_list_t::iterator iter = item_window_[i].begin();
        for (; iter != item_window_[i].end(); ++iter) {
          if ((*iter)->publish_time < query.start_time
              || (*iter)->publish_time > query.end_time) {
            continue;
          }
          candidate_t cand;

          cand.item_id = (*iter)->item_id;
          cand.power = (*iter)->primary_power;
          cand.publish_time = (*iter)->publish_time;
          cand.picture_num = (*iter)->picture_num;
          cand.category_id = (*iter)->category_id;
          candset.push_back(cand);
        }
        pthread_rwlock_unlock(&window_rwlock_[i%kWindowLockSize]);
      }
      return Status::OK();
    }
  }; // namespace news
}; // namespace rsys
