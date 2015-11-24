#include "util/wal.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <string.h>
#include "glog/logging.h"

namespace rsys {
  namespace news {
    WALWriter::WALWriter(const std::string& name)
      : writer_(name)
    {
    }

    WALWriter::~WALWriter()
    {
    }
    
    Status WALWriter::create(const fver_t& ver)
    {
      Status status = writer_.create();
      if (!status.ok())
        return status;

      return writer_.writeMeta((char*)&ver, sizeof(fver_t));
    }
 
    void WALWriter::close()
    {
      writer_.close();
    }

    Status WALWriter::append(const std::string& data)
    {
      Status status = writer_.write(data);
      if (!status.ok())
        return status;
      return writer_.flush();
    }

    const std::string& WALWriter::filename() const 
    {
      return writer_.filename();
    }
 
    WALReader::WALReader(const std::string& name)
      : reader_(name)
    {
    }

    WALReader::~WALReader()
    {
    }

    Status WALReader::open(fver_t& ver)
    {
      Status status = reader_.open();
      if (!status.ok())
        return status;

      return reader_.readMeta(sizeof(fver_t), (char*)&ver);
    }

    Status WALReader::read(std::string& data)
    {
      return reader_.read(data);
    }

    const std::string& WALReader::filename() const 
    {
      return reader_.filename();
    }
       
    void WALReader::close()
    {
      reader_.close();
    }
  } // namespace news
} // namespace rsys
