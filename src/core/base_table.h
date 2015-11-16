#ifndef RSYS_NEWS_BASE_TABLE_H
#define RSYS_NEWS_BASE_TABLE_H

#include "core/options.h"
#include "util/wal.h"
#include "util/level_file.h"

namespace rsys {
  namespace news {
    class BaseTable {
      public:
        BaseTable();
        ~BaseTable();

      public:
        const Options& options() {
          return options_;
        }

      public:
        //与flush采用不同的周期,只保留days天
        Status rollOverLogFile(int32_t expired_days);
        // WAL
        Status writeAheadLog(const std::string& data);

      private:
        Options& options_;
        WALWriter* writer_;
        pthread_mutex_t mutex_; 
    };
  } // namespace news
} // namespace rsys
#endif // #define RSYS_NEWS_BASE_TABLE_H

