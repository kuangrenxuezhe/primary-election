#ifndef RSYS_NEWS_BASE_TABLE_H
#define RSYS_NEWS_BASE_TABLE_H

#include "util/status.h"
#include "util/ahead_log.h"
#include "util/table_file.h"
#include "util/level_table.h"

namespace rsys {
  namespace news {
      class TableBase {
        public:
          TableBase(const std::string& path, const std::string& name, const fver_t& ver);
          virtual ~TableBase();

        public:
          // 加载用户表
          Status loadTable();
          // 保存用户表
          Status flushTable();
          // write-ahead log
          Status writeAheadLog(const std::string& data);

        public:
          fver_t file_version() { return fver_; }
          // 创建value对象 
          //virtual value_t* newValue() = 0;
          // 创建AheadLog对象
          virtual AheadLog* createAheadLog() = 0;

        public:
          virtual Status onPreLoad() { return Status::OK(); }
          virtual Status loadData(const std::string& data) {
            return Status::OK();
          }
          virtual Status onLoadComplete() { return Status::OK(); }

          virtual Status onPreDump() { return Status::OK(); }
          virtual Status dumpToFile(const std::string& temp_name) {
            return Status::OK();
          }
          virtual Status onDumpComplete() { return Status::OK(); }

          // 序列化和反序列话对象
          //virtual bool parseFrom(const std::string& data, uint64_t* key, value_t* value) = 0;
          //virtual bool serializeTo(uint64_t key, const value_t* value,  std::string& data) = 0;

        protected:
          // 库文件生效
          Status applyFile(const std::string& name);

        private:
          fver_t                fver_;
          std::string           path_;
          std::string           name_;
          AheadLog*        ahead_log_;
      };
  } // namespace news
} // namespace rsys
#endif // #define RSYS_NEWS_BASE_TABLE_H

