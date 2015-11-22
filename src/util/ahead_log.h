#ifndef RSYS_NEWS_AHEAD_LOG_H
#define RSYS_NEWS_AHEAD_LOG_H

#include <pthread.h>
#include "util/wal.h"

namespace rsys {
  namespace news {
    class AheadLog {
      public:
        AheadLog(const std::string& path, const std::string& name, const fver_t& fver);
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
        virtual Status trigger() {
          return Status::OK();
        }
        // 处理数据回滚
        virtual Status rollback(const std::string& data) {
          return Status::OK();
        }

      protected:
        int stat_file_numb();
        Status recovery(const std::string& name);
        Status recovery(const std::string& name, WALWriter* writer);

      private:
        fver_t           fver_; // 文件版本号
        std::string      path_; // 文件保存路径
        std::string      name_; // ahead-log名称
        WALWriter*     writer_; // ahead-log writer
        pthread_mutex_t mutex_; // 互斥器
    };
  } // namespace news
} // namespace rsys
#endif // #define RSYS_NEWS_AHEAD_LOG_H


