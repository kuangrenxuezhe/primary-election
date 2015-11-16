#ifndef RSYS_NEWS_WAL_H
#define RSYS_NEWS_WAL_H

#include <unistd.h>
#include <string>
#include "util/status.h"
#include "util/level_file.h"

namespace rsys {
  namespace news {
    class WALWriter {
      public:
        WALWriter(const std::string& name);
        ~WALWriter();

      public:
        Status create(const fver_t& ver);
        void close();
       
      public:
        Status append(const std::string& data);

      private:
        FileWriter writer_;
    };

    class WALReader {
      public:
        WALReader(const std::string& name);
        ~WALReader();

      public:
        Status open(fver_t& ver);
        void close();

      public:
        Status read(std::string& data);

      private:
        FileReader reader_;
    };

    // 从不完整WAL文件中创建WALWriter
    // 采用从不完整WAL文件中复制正常记录到新创建的WALWriter中的策略
    extern Status recoverWALWriter(const std::string& name, WALWriter* writer);
  } // namespace news
} // namespace rsys
#endif // #define RSYS_NEWS_WAL_H

