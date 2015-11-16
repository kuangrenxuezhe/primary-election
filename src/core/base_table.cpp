#include "core/base_table.h"

namespace rsys {
  namespace news {
    // 保存用户表
    Status BaseTable::flushTable()
    {
    }

    // 加载用户表
    Status BaseTable::loadTable()
    {
    }

    // 淘汰用户，包括已读，不喜欢和推荐信息
    Status BaseTable::eliminate(int32_t hold_time)
    {
    }

    //与flush采用不同的周期,只保留days天
    Status BaseTable::rollOverLogFile(int32_t expired_days)
    {
      char today[300], yesterday[300];

      pthread_mutex_lock(&mutex_);
      for (int i = expired_days; i > 0; --i) {
        sprintf(today, "%s/wal.day-%d", options_.path.c_str(), i);
        if (access(today, F_OK))
          continue;
        if (i == expired_days) {
          if (remove(today)) {
            LOG(FATAL) << "remove file failed: " << strerror(errno)
              << ", file: " << today;
            pthread_mutex_unlock(&mutex_);
            return Status::IOError(strerror(errno));
          }
        } else {
          sprintf(yesterday, "%s/wal.day-%d", options_.path.c_str(), i);
          if (rename(today, yesterday)) {
            LOG(FATAL) << "rename file failed: " << strerror(errno)
              << ", oldfile: " << today << ", newfile: " << yesterday;
            pthread_mutex_unlock(&mutex_);
            return Status::IOError(strerror(errno));
          }
        }
      }
      writer_->close();

      sprintf(today, "%s/wal.day-0", options_.path.c_str());
      sprintf(yesterday, "%s/wal.day-1", options_.path.c_str());
      if (rename(today, yesterday)) {
        LOG(FATAL) << "rename file failed: " << strerror(errno)
          << ", oldfile: " << today << ", newfile: " << yesterday;
        pthread_mutex_unlock(&mutex_);
        return Status::IOError(strerror(errno));
      }
      writer_ = new WALWriter(today);
      pthread_mutex_unlock(&mutex_);

      return Status::OK();
    }
    // WAL
    Status BaseTable::writeAheadLog(const std::string& data)
    {
      return Status::OK();
    }
  } // namespace news
} // namespace rsys
