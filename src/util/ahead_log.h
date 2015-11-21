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
        // 创建ahead log
        Status open();
        // 滚存ahead log文件
        Status rollover();
        // 丢弃已生效的ahead log
        Status apply(int32_t expired);
        // 写入ahead log
        Status write(const std::string& data);
        // 关闭Ahead log
        void close();

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
        int stat_file_numb();
        Status recovery(const std::string& name);
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


