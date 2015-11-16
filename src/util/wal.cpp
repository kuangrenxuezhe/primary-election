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
      
      status = writer_.lockfile();
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
      return writer_.write(data);
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

    void WALReader::close()
    {
      reader_.close();
    }

    Status recoverWALWriter(const std::string& name, WALWriter* writer)
    {
      fver_t ver;
      WALReader reader(name);

      Status status = reader.open(ver);
      if (!status.ok()) {
        return status;
      }
      std::string data;

      status = reader.read(data);
      while (status.ok()) {
        status = writer->append(data);
        if (!status.ok()) {
          reader.close();
          return status;
        }
        data.clear();
        status = reader.read(data);
      }
      reader.close();

      return Status::OK();
    }
  } // namespace news
} // namespace rsys
