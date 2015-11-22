#include "util/table_file.h"
#include "util/crc32c.h"
#include <sstream>

namespace rsys {
  namespace news {
    TableFileWriter::TableFileWriter(const std::string& name)
      : crc_(0), writer_(name)
    {
    }

    TableFileWriter::~TableFileWriter()
    {
    }

    Status TableFileWriter::create(const fver_t& ver)
    {
      Status status = writer_.create();
      if (!status.ok())
        return status;

      return writer_.writeMeta((char*)&ver, sizeof(fver_t));
    }

    Status TableFileWriter::close()
    {
      static std::string nullstr;
      // 写一个空记录表示结尾
      Status status = write(nullstr);

      crc_ = crc::mask(crc_);
      status = writer_.writeMeta((char*)&crc_, sizeof(uint32_t));
      writer_.close();

      return status;
    }

    Status TableFileWriter::write(std::string& data)
    {
      crc_ = crc::extend(crc_, data.c_str(), data.length());
      return writer_.write(data);
    }

    const std::string& TableFileWriter::filename() const 
    {
      return writer_.filename();
    }

    TableFileReader::TableFileReader(const std::string& name)
      : crc_(0), reader_(name)
    {
    }

    TableFileReader::~TableFileReader()
    {
    }

    Status TableFileReader::open(fver_t& ver)
    {
      Status status = reader_.open();
      if (!status.ok())
        return status;

      return reader_.readMeta(sizeof(fver_t), (char*)&ver);
    }

    Status TableFileReader::close()
    {
      uint32_t valid_crc;
      Status status = reader_.readMeta(sizeof(uint32_t), (char*)&valid_crc);
      if (!status.ok()) {
        reader_.close();
        return status;
      }
      
      if (valid_crc != crc::mask(crc_)) {
        std::ostringstream strstream;

        strstream<<"File '"<<reader_.filename()<<"' crc invalid: ";
        strstream<<"expect="<<valid_crc<<", real="<<crc::mask(crc_);
        return Status::IOError(strstream.str());
      }

      return Status::OK();
    }

    Status TableFileReader::read(std::string& data)
    {
      Status status = reader_.read(data);
      if (!status.ok())
        return status;

      crc_ = crc::extend(crc_, data.c_str(), data.length());
      return Status::OK();
    }

    const std::string& TableFileReader::filename() const 
    {
      return reader_.filename();
    }
  } // namespace news
} // namespace rsys
