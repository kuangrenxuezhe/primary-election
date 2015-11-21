#ifndef RSYS_NEWS_LEVEL_FILE_H
#define RSYS_NEWS_LEVEL_FILE_H

#include <stdint.h>
#include <sstream>
#include "util/status.h"
#include "util/file.h"

namespace rsys {
  namespace news {
    static const uint32_t kVersionFlag = 0xAFU;
    // level file version
    struct fver_ {
      uint32_t flag:8; // 默认表示：0xAF
      uint32_t major:16; // 主版本号
      uint32_t minor:8; // 小版本号

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

    // 不允许写入空记录，空记录作为LevelFile结尾标记
    class LevelFileWriter {
      public:
        LevelFileWriter(const std::string& name);
        ~LevelFileWriter();

      public:
        Status create(const fver_t& ver);
        // 文件关闭时，写入CRC
        Status close();
        const std::string& filename() const;

      public:
        Status write(std::string& data);

      private:
        uint32_t crc_;
        FileWriter writer_;
    }; 

    // 空记录表示LevelFile文件结尾
    class LevelFileReader {
      public:
        LevelFileReader(const std::string& name);
        ~LevelFileReader();

      public:
        Status open(fver_t& ver);
        // 关闭文件时，验证CRC
        Status close();
        const std::string& filename() const;

      public:
        Status read(std::string& data);

      private:
        uint32_t crc_;
        FileReader reader_;
    };
  } // namespace news
} // namespace rsys
#endif // #define RSYS_NEWS_LEVEL_FILE_H

