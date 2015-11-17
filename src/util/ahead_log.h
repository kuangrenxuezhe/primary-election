#ifndef RSYS_NEWS_AHEAD_LOG_H
#define RSYS_NEWS_AHEAD_LOG_H

#include <pthread.h>
#include "util/wal.h"

namespace rsys {
  namespace news {
    class AheadLog {
      public:
        AheadLog(const std::string& path, const fver_t& fver);
        virtual ~AheadLog();

      public:
        //滚存Log文件
        Status rollover();
        Status recovery();

        // 丢弃已生效的ahead log
        Status apply(int32_t expired);

        // Write-Ahead Log
        Status write(const std::string& data);

      protected:
        // ahead log滚存触发器
        virtual bool trigger() {
          return true;
        }
        // 处理数据回滚
        virtual bool rollback(const std::string& data) {
          return true;
        }

      protected:
        Status recovery(const std::string& name, WALWriter* writer);

      private:
        fver_t fver_;
        std::string path_;
        WALWriter* writer_;
        pthread_mutex_t mutex_; 
    };
  } // namespace news
} // namespace rsys
#endif // #define RSYS_NEWS_AHEAD_LOG_H


