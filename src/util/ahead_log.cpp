#include "util/ahead_log.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sstream>

namespace rsys {
  namespace news {
    static const std::string kRecoveryName = "/wal.recovery";
    static const std::string kWritingName = "/wal.writing";

    AheadLog::AheadLog(const std::string& path, const fver_t& fver)
      : path_(path), fver_(fver.major, fver.minor), writer_(NULL)
    {
      writer_ = new WALWriter(path_ + "/wal.writing");
      pthread_mutex_init(&mutex_, NULL);
    }

    AheadLog::~AheadLog()
    {
      if (writer_)
        delete writer_;
      pthread_mutex_destroy(&mutex_);
    }

    int AheadLog::stat_file_numb() 
    {
      char name[300];
      int max_no = 0;

      // 计算最大文件序号
      for (;; ++max_no) {
        sprintf(name, "%s/wal-%d.dat", path_.c_str(), max_no);
        if (::access(name, F_OK))
          break;
      }
      return max_no;
    }

    // 丢弃已生效的ahead log
    Status AheadLog::apply(int32_t expired)
    {
      int i = 0;
      char name[300];

      Status status = Status::OK();
      if (expired <= 0) { // 表示删除所有的ahead log
        for (i = 0;; ++i) {
          sprintf(name, "%s/wal-%d.dat", path_.c_str(), i);
          if (::access(name, F_OK))
            break;

          if (::remove(name)) {
            std::ostringstream oss;

            oss<<strerror(errno)<<", file="<<name;
            status = Status::IOError(oss.str());
          }
        }
        return status;
      }
      int max_no = stat_file_numb();

      int32_t ctime = time(NULL);

      for (; max_no > 0; --max_no) {
        struct stat stbuf;

        sprintf(name, "%s/wal-%d.dat", path_.c_str(), max_no - 1);
        if (::stat(name, &stbuf)) {
          std::ostringstream oss;

          oss<<strerror(errno)<<", file="<<name;
          status = Status::IOError(oss.str());
          continue;
        }

        if (stbuf.st_mtime < ctime - expired)
          continue;

        // 删除过期
        if (::remove(name)) {
          std::ostringstream oss;

          oss<<strerror(errno)<<", file="<<name;
          status = Status::IOError(oss.str());
        }
      }
      return status;
    }

    //滚存Log文件
    Status AheadLog::rollover()
    {
      char name[300];

      int max_no = 0;
      Status status = Status::OK();

      pthread_mutex_lock(&mutex_);
      writer_->close();
      if (!trigger()) {
        status = Status::Corruption("Trigger failed");
      }
      char newname[300];
 
      max_no = stat_file_numb();
      for (; max_no > 0; --max_no) {
        sprintf(name, "%s/wal-%d.dat", path_.c_str(), max_no - 1);
        sprintf(newname, "%s/wal-%d.dat", path_.c_str(), max_no);

        if (::rename(name, newname)) {
          std::ostringstream oss;

          oss<<strerror(errno)
            <<", oldfile="<<name<<", newfile="<<newname;
          status = Status::IOError(oss.str());
        }
      }

      sprintf(newname, "%s/wal-%d.dat", path_.c_str(), max_no);
      if (::rename(writer_->filename().c_str(), newname)) {
        std::ostringstream oss;

        oss<<strerror(errno)
          <<", oldfile="<<name<<", newfile="<<newname;
        status = Status::IOError(oss.str());
      }

      Status wrs = writer_->create(fver_);
      if (!wrs.ok()) {
        status = wrs;
      }
      pthread_mutex_unlock(&mutex_);

      return status;
    }

    // Write-Ahead Log
    Status AheadLog::write(const std::string& data)
    {
      pthread_mutex_lock(&mutex_);
      Status status = writer_->append(data);
      if (!status.ok()) {
        pthread_mutex_unlock(&mutex_);
        return status;
      }
      pthread_mutex_unlock(&mutex_);

      return Status::OK();
    }

    Status AheadLog::open()
    {
      char name[300];

      int max_no = stat_file_numb();
      for (; max_no > 0; --max_no) {
        sprintf(name, "%s/wal-%d.dat", path_.c_str(), max_no - 1);
        Status status = recovery(name);
        if (!status.ok())
          return status;
      }
      std::string recovery_name = path_ + "/wal.recovery";
      std::string writing_name = path_ + "/wal.writing";

      assert(NULL != writer_);
      if (!access(recovery_name.c_str(), F_OK)) {
        Status status = writer_->create(fver_);
        if (!status.ok()) {
          return status;
        }
        return recovery(recovery_name, writer_);
      } else {
        if (access(writing_name.c_str(), F_OK)) {
          return writer_->create(fver_);
        } else {
          if (rename(writing_name.c_str(), recovery_name.c_str())) {
            std::ostringstream oss;

            oss<<strerror(errno);
            oss<<", oldfile="<<writing_name<<", newfile="<<recovery_name;
            return Status::IOError(oss.str());
          }

          Status status = writer_->create(fver_);
          if (!status.ok()) {
            return status;
          }
          return recovery(recovery_name, writer_);
        }
      }
    }

    Status AheadLog::recovery(const std::string& name)
    {
      fver_t ver;
      WALReader reader(name);

      Status status = reader.open(ver);
      if (!status.ok()) {
        return status;
      }
      std::string serialized_data;

      do {
        status = reader.read(serialized_data);
        if (!status.ok())
          break;

        if (serialized_data.length() <= 0)
          break;
      } while (rollback(serialized_data));

      reader.close();
      return Status::OK();
    }

    Status AheadLog::recovery(const std::string& name, WALWriter* writer)
    {
      fver_t ver;
      WALReader reader(name);

      Status status = reader.open(ver);
      if (!status.ok()) {
        return status;
      }
      std::string serialized_data;

      do {
        status = reader.read(serialized_data);
        if (!status.ok())
          break;

        if (serialized_data.length() <= 0)
          break;

        if (rollback(serialized_data)) {
          status = writer->append(serialized_data);
          if (!status.ok()) {
            reader.close();
            return status;
          }
        }
      } while (true);
      reader.close();

      if (::remove(name.c_str())) {
        std::ostringstream oss;

        oss<<strerror(errno)<<", file="<<name;
        status = Status::IOError(oss.str());
      }
      return Status::OK();
    } 

    void AheadLog::close() 
    {
      writer_->close();
    }
  } // namespace news
} // namespace rsys
