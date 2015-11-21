#include "util/table_base.h"

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sstream>

namespace rsys {
  namespace news {
    template<typename V>
      TableBase<V>::TableBase(const std::string& path, const std::string& name, const fver_t& ver, size_t level)
        : path_(path), name_(name), ahead_log_(NULL), level_table_(NULL) {
        fver_.flag = ver.flag;
        fver_.major = ver.major;
        fver_.minor = ver.minor;
        level_table_ = new level_table_t(level);
      }

    template<typename V>
      TableBase<V>::~TableBase() {
        //if (ahead_log_) {
        //  delete ahead_log_;
        //}
        if (level_table_) {
          delete level_table_;
        }
      }

    template<typename V>
      inline Status TableBase<V>::applyFile(const std::string& name) {
        Status status = ahead_log_->apply(-1);
        std::string table_name = path_ + "/" + name_;

        if (0 == ::access(table_name.c_str(), F_OK)) {
          if (::remove(table_name.c_str())) {
            std::ostringstream oss;

            oss<<strerror(errno)<<", file="<<table_name;
            status = Status::IOError(oss.str());
          }
        }

        if (::rename(name.c_str(), table_name.c_str())) {
          std::ostringstream oss;

          oss<<strerror(errno)<<", oldfile="<<name<<", newfile="<<table_name;
          status = Status::IOError(oss.str());
        }

        return status;
      }

    // 保存用户表
    template<typename V>
      inline Status TableBase<V>::flushTable() {
        Status status = ahead_log_->rollover();
        if (!status.ok()) {
          return status;
        }

        if (level_table_->depth() >= kMinLevel) {
          if (!level_table_->merge()) {
            return Status::Corruption("Table merge failed");
          }
        }

        typename level_table_t::Iterator iter = level_table_->snapshot();
        if (!iter.valid()) {
          return Status::Corruption("Table depth not enough");
        }
        std::string temp_name = name_ + ".tmp";
        LevelFileWriter writer(temp_name);

        status = writer.create(fver_);
        if (!status.ok()) {
          return status;
        }
        std::string serialized_data;

        for (; iter.hasNext(); iter.next()) {
          if (!serializeTo(iter.key(), iter.value(), serialized_data)) {
            std::ostringstream oss;

            writer.close();
            oss<<"Serialize as string, key=0x"<<std::hex<<iter.key();
            return Status::Corruption(oss.str());
          } else {
            status = writer.write(serialized_data);
            if (!status.ok()) {
              writer.close();
              return status;
            }
          }
        }
        writer.close();

        return applyFile(temp_name);
      }

    // 加载用户表
    template<typename V>
      inline Status TableBase<V>::loadTable() {
        fver_t fver;

        if (NULL == ahead_log_)
          ahead_log_ = createAheadLog();

        if (::access(name_.c_str(), F_OK)) { // 表示空库
          // 若存在ahead log则从ahead log恢复
          return ahead_log_->open();
        }
        LevelFileReader reader(name_);

        // 从当前level file中加载table
        Status status = reader.open(fver);
        if (!status.ok()) {
          return status;
        }

        // 版本标记号验证
        if (!fver_.valid(fver)) {
          std::ostringstream oss;

          oss<<"Invalid version, expect="<<fver_.toString()<<", real="<<fver.toString();
          return Status::IOError(oss.str());
        }
        std::string serialized_data;

        do {
          status = reader.read(serialized_data);
          if (!status.ok()) {
            reader.close();
            return status;
          }

          // 长度为空内容表示文件结尾
          if (serialized_data.length() <= 0) {
            break;
          }
          uint64_t key;

          value_t* value = newValue();
          if (!parseFrom(serialized_data, &key, value)) {
            return Status::Corruption("Parse from string.");
          } else {
            if (!level_table_->add(key, value)) {
              std::ostringstream oss;

              oss<<"Insert level table: key=0x"<<std::hex<<key;
              reader.close();
              return Status::Corruption(oss.str());
            }
          }
        } while(serialized_data.length() > 0);

        reader.close();
        level_table_->deepen();

        return ahead_log_->open(); // 从ahead log恢复
      }

    // WAL
    template<typename V>
      inline Status TableBase<V>::writeAheadLog(const std::string& data) {
        return ahead_log_->write(data);
      }
  } // namespace news
} // namespace rsys
