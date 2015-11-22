#ifndef RSYS_NEWS_LEVEL_FILE_H
#define RSYS_NEWS_LEVEL_FILE_H

#include <stdint.h>
#include <sstream>
#include "util/status.h"
#include "util/file.h"

namespace rsys {
  namespace news {
        // 不允许写入空记录，空记录作为TableFile结尾标记
    class TableFileWriter {
      public:
        TableFileWriter(const std::string& name);
        ~TableFileWriter();

      public:
        Status create(const fver_t& ver);
        // 文件关闭时，写入CRC
        Status close();
        const std::string& filename() const;

      public:
        Status write(std::string& data);

      private:
        uint32_t      crc_;
        FileWriter writer_;
    }; 

    // 空记录表示TableFile文件结尾
    class TableFileReader {
      public:
        TableFileReader(const std::string& name);
        ~TableFileReader();

      public:
        Status open(fver_t& ver);
        // 关闭文件时，验证CRC
        Status close();
        const std::string& filename() const;

      public:
        Status read(std::string& data);

      private:
        uint32_t      crc_;
        FileReader reader_;
    };
  } // namespace news
} // namespace rsys
#endif // #define RSYS_NEWS_LEVEL_FILE_H

