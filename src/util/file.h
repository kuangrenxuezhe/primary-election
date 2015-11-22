#ifndef RSYS_NEWS_FILE_H
#define RSYS_NEWS_FILE_H

#include <stdint.h>
#include <stdio.h>
#include <sstream>
#include "util/status.h"

namespace rsys {
  namespace news {
    static const uint32_t kVersionFlag = 0xAFU;
    // level file version
    struct fver_ {
      uint32_t   flag:8; // 默认表示：0xAF
      uint32_t major:16; // 主版本号
      uint32_t  minor:8; // 小版本号

      fver_(): flag(kVersionFlag) {
        this->major = this->minor = 0;
      }
      fver_(uint32_t major, uint32_t minor): flag(kVersionFlag) {
        this->major = major; this->minor = minor;
      }
      bool valid(const fver_& fver) {
        return flag == fver.flag && major == fver.major && minor == fver.minor;
      }
      std::string toString() {
        std::ostringstream oss;

        oss << std::hex << "0x" << flag;
        oss << std::dec << "v" << major << "." << minor;
        return oss.str();
      }
    };
    typedef struct fver_ fver_t; 

    class FileWriter {
      public:
        FileWriter(const std::string& name);
        ~FileWriter();

      public:
        Status create();
        void close();

      public:
        // 锁定文件，只允许当前进程写数据
        Status lockfile();
        Status flush();

        // 写入长度+数据
        Status write(const std::string& data);
        // 只写入数据
        Status writeMeta(const char* data, ssize_t len);

      public:
        const std::string& filename() const {
          return name_;
        }

      private:
        int          wfd_;
        std::string name_;
    };

    class FileReader {
      public:
        FileReader(const std::string& name);
        ~FileReader();

      public:
        Status open();
        void close();

      public:
        // 读取长度+数据
        Status read(std::string& buffer);
        // 只读取指定长度的数据, 保证buffer长度大于len
        Status readMeta(ssize_t len, char* buffer);

      public:
        const std::string& filename() const {
          return name_;
        }

      private:
        int          rfd_;
        std::string name_;
    };
  } // namespace news
} // namespace rsys
#endif // #define RSYS_NEWS_FILE_H

