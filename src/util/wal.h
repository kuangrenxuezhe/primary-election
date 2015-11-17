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
        const std::string& filename() const;

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
        const std::string& filename() const;

      public:
        Status read(std::string& data);

      private:
        FileReader reader_;
    };
  } // namespace news
} // namespace rsys
#endif // #define RSYS_NEWS_WAL_H

