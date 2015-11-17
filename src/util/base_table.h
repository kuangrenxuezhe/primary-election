#ifndef RSYS_NEWS_BASE_TABLE_H
#define RSYS_NEWS_BASE_TABLE_H

#include "util/status.h"
#include "util/wal.h"
#include "util/level_file.h"
#include "util/level_table.h"

namespace rsys {
  namespace news {
    template<typename V>
      class BaseTable {
        public:
          typedef V value_t;
          typedef LevelTable<uint64_t, value_t> level_table_t;

        public:
          BaseTable() {
          }
          ~BaseTable() {
          }

        public:
          level_table_t& level_table() {
            return level_table_;
          }

        public:
          // 写入表的版本号
          virtual fver_t tableVersion() const = 0;

          virtual const std::string tableName() const = 0;
          virtual const std::string workPath() const = 0;

        public:
          // 保存用户表
          Status flushTable();
          // 加载用户表
          Status loadTable();
          // 库文件生效
          Status applyTableFile(const std::string& name);

          //与flush采用不同的周期,只保留days天
          Status rollOverLogFile();
          Status recoverAheadLog();

          // WAL
          Status writeAheadLog(const std::string& data);
          Status recoveryAheadLog(const std::string& name, WALWriter* writer);

        public:
          virtual value_t* newValue() = 0;
          // 淘汰用户，包括已读，不喜欢和推荐信息
          virtual Status eliminate(int32_t hold_time) = 0;

          virtual bool rollback(const std::string& data) = 0;
          virtual bool parseFrom(const std::string& data, uint64_t* key, value_t* value) = 0;
          virtual bool serializeTo(uint64_t key, const value_t* value,  std::string& data) = 0;

        private:
          WALWriter* wal_;
          pthread_mutex_t mutex_; 

          level_table_t level_table_;
      };
  } // namespace news
} // namespace rsys
#include "util/base_table.inl"
#endif // #define RSYS_NEWS_BASE_TABLE_H

