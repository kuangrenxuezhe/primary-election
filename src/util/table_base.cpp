#include "util/table_base.h"

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sstream>

namespace rsys {
  namespace news {
    TableBase::TableBase(const std::string& path, const std::string& name, const fver_t& ver)
      : path_(path), name_(name), ahead_log_(NULL) {
        fver_ = ver;
      }

    TableBase::~TableBase() {
      if (ahead_log_) {
        delete ahead_log_;
      }
    }

    Status TableBase::applyFile(const std::string& name) {
      char fullpath[300];

      Status status = ahead_log_->apply(-1);
      
      sprintf(fullpath, "%s/%s", path_.c_str(), name_.c_str());
      std::string table_name(fullpath);

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
    Status TableBase::flushTable() {
      Status status = onPreDump();
      if (!status.ok()) {
        return status;
      }

      if (NULL == ahead_log_) {
        ahead_log_ = createAheadLog();
      } else {
        status = ahead_log_->rollover();
        if (!status.ok()) {
          return status;
        }
      }
      char fullpath[300];

      sprintf(fullpath, "%s/%s.tmp", path_.c_str(), name_.c_str());
      std::string temp_name(fullpath);

      status = dumpToFile(temp_name);
      if (!status.ok()) {
        return status;
      }

      status = applyFile(temp_name);
      if (!status.ok()) {
        return status;
      }

      return onDumpComplete();
    }

    // 加载用户表
    Status TableBase::loadTable() {
      fver_t fver;

      Status status = onPreLoad();
      if (!status.ok()) {
        return status;
      }

      if (NULL == ahead_log_)
        ahead_log_ = createAheadLog();

      if (::access(name_.c_str(), F_OK)) { // 表示空库
        status = onLoadComplete();
        if (!status.ok()) {
          return status;
        }
        // 若存在ahead log则从ahead log恢复
        return ahead_log_->open();
      }
      TableFileReader reader(name_);

      // 从当前level file中加载table
      status = reader.open(fver);
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
          goto FAILED;
        }

        // 长度为空内容表示文件结尾
        if (serialized_data.length() <= 0) {
          break;
        }

        status = loadData(serialized_data);
        if (!status.ok()) {
          goto FAILED;
        }
      } while(serialized_data.length() > 0);

      reader.close();
      status = onLoadComplete();
      if (!status.ok()) {
        return status;
      }

      return ahead_log_->open(); // 从ahead log恢复

FAILED:
      reader.close();
      return status;
    }

    // WAL
    Status TableBase::writeAheadLog(const std::string& data) {
      return ahead_log_->write(data);
    }
  } // namespace news
} // namespace rsys
