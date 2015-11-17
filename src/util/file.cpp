#include "util/file.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <sstream>

namespace rsys {
  namespace news {
    static const ssize_t kBufSize = 4096;

    FileWriter::FileWriter(const std::string& name)
      : wfd_(-1), name_(name)
    {
    }

    FileWriter::~FileWriter()
    {
      close();
    }

    Status FileWriter::create()
    {
      wfd_ = ::open(name_.c_str(), O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
      if (wfd_ <= 0) {
        std::ostringstream strstream;

        strstream<<"Open '"<<name_<<"' failed: "<<strerror(errno);
        return Status::IOError(strstream.str());
      }
      return Status::OK(); 
    }

    void FileWriter::close()
    {
      if (wfd_ > 0)
        ::close(wfd_);
      wfd_ = -1;
    }

    // 锁定文件，只允许当前进程写数据
    Status FileWriter::lockfile()
    {
      if (flock(wfd_, LOCK_EX|LOCK_NB)) {
        std::ostringstream strstream;

        strstream<<"Lock '"<<name_<<"' failed: "<<strerror(errno);
        return Status::IOError(strstream.str());
      }
      return Status::OK();
    }

    Status FileWriter::flush()
    {
      if (fsync(wfd_)) {
        std::ostringstream strstream;

        strstream<<"Flush '"<<name_<<"' failed: "<<strerror(errno);
        return Status::IOError(strstream.str());
      }
      return Status::OK();
    }

    Status FileWriter::write(const std::string& data)
    {
      ssize_t len = data.length();

      Status status = writeMeta((char*)&len, sizeof(len));
      if (!status.ok())
        return status;

      return writeMeta(data.c_str(), data.length());
    }

    Status FileWriter::writeMeta(const char* data, ssize_t len)
    {
      if (len > 0) {
        if (len != ::write(wfd_, data, len)) {
          std::ostringstream strstream;

          strstream<<"Write to '"<<name_<<"' failed: "<<strerror(errno);
          return Status::IOError(strstream.str());
        }
      }

      return Status::OK(); 
    }

    /*
    Status FileWriter::fileSize(off_t& size)
    {
      struct stat stbuf;

      if (fstat(wfd_, &stbuf)) {
        std::ostringstream strstream;

        strstream<<"Stat '"<<name_<<"' failed: "<<strerror(errno);
        return Status::IOError(strstream.str());
      }
      size = stbuf.st_size;

      return Status::OK();
    }
    */

    FileReader::FileReader(const std::string& name)
      : rfd_(-1), name_(name)
    {
    }

    FileReader::~FileReader()
    {
      close();
    }

    Status FileReader::open()
    {
      rfd_ = ::open(name_.c_str(), O_RDONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
      if (rfd_ <= 0) {
        std::stringstream strstream;

        strstream<<strerror(errno)<<", file="<<name_;
        return Status::IOError(strstream.str());
      }
      return Status::OK(); 
    }

    void FileReader::close()
    {
      if (rfd_ > 0)
        ::close(rfd_);
      rfd_ = -1;
    }

        // 读取长度+数据
    Status FileReader::read(std::string& data)
    {
      ssize_t total = 0, slen = 0;

      Status status = readMeta(sizeof(ssize_t), (char*)&total);
      if (!status.ok())
        return status;

      data.clear();
      data.reserve(total);

      char buf[kBufSize];
      while (slen < total) {
        ssize_t rlen = total - slen > kBufSize ? kBufSize : total - slen;

        status = readMeta(rlen, buf);
        if (!status.ok())
          return status;

        slen += rlen;
        data.append(buf, rlen);
      }

      return Status::OK();
    }

    // 只读取指定长度的数据, 保证buffer长度大于len
    Status FileReader::readMeta(ssize_t len, char* buffer)
    {
      if (len > 0) {
        ssize_t rlen = ::read(rfd_, buffer, len);
        if (rlen <= 0) {
          std::stringstream strstream;

          strstream<<"Read '"<<name_<<"' failed: "<<strerror(errno);
          return Status::IOError(strstream.str());
        }

        if (rlen != len) {
          std::stringstream strstream;

          strstream<<"File '"<<name_<<"' not enough data.";
          return Status::IOError(strstream.str());
        }
      }

      return Status::OK();
    }
  } // namespace news
} // namespace rsys
