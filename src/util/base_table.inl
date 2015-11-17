#include "util/base_table.h"

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sstream>

namespace rsys {
  namespace news {
    template<typename V>
    inline Status BaseTable<V>::applyTableFile(const std::string& name)
    {
      int i = 0;
      char wal_name[300];
      Status status = Status::OK();

      for (;;) {
        sprintf(wal_name, "%s/wal-%d.dat", workPath().c_str(), i);
        if (::access(wal_name, F_OK))
          break;

        if (::remove(wal_name)) {
          std::ostringstream oss;

          oss<<"Remove file failed: "<<strerror(errno)<<"file="<<wal_name;
          status = Status::IOError(oss.str());
        }
      }
      std::string table_name = workPath() + "/" + tableName();

      if (::access(table_name.c_str(), F_OK)) {
        if (::remove(table_name.c_str())) {
          std::ostringstream oss;

          oss<<"Remove file failed: "<<strerror(errno)<<"file="<<wal_name;
          status = Status::IOError(oss.str());
        }
      }

      if (::rename(name.c_str(), table_name.c_str())) {
        std::ostringstream oss;

        oss<<"Rename file failed: "<<strerror(errno);
        oss<<"oldfile="<<name<<", newfile="<<table_name;
        status = Status::IOError(oss.str());
      }

      return status;
    }

    // 保存用户表
    template<typename V>
      inline Status BaseTable<V>::flushTable() 
      {
        Status status = rollOverLogFile();
        if (!status.ok()) {
          return status;
        }

        typename level_table_t::Iterator iter = level_table_.snapshot();
        if (!iter.valid()) {
          return Status::Corruption("table depth not enough");
        }
        std::string tmpFileName = tableName() + ".tmp";
        LevelFileWriter writer(tmpFileName);

        status = writer.create(tableVersion());
        if (!status.ok()) {
          return status;
        }
        std::string serialized_data;

        for (; iter.hasNext(); iter.next()) {
          if (!serializeTo(iter.key(), iter.value(), serialized_data)) {
            std::ostringstream oss;

            writer.close();
            oss<<"Serialize as string: id=0x"<<std::hex<<iter.key();
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

        return applyTableFile(tmpFileName);
      }

    // 加载用户表
    template<typename V>
      inline Status BaseTable<V>::loadTable()
      {
        fver_t fver;

        std::string table_name = tableName();
        if (::access(table_name.c_str(), F_OK)) {
          return Status::OK(); // 表示空库
        }
        LevelFileReader reader(tableName());

        Status status = reader.open(fver);
        if (!status.ok()) {
          return status;
        }

        // 版本标记号验证
        if (fver.flag != kVersionFlag) {
          std::ostringstream oss;

          oss<<"Invalid file version flag: expect="<<kVersionFlag
            <<", real="<<fver.flag;
          return Status::IOError(oss.str());
        }
        std::string serialized_data;

        do {
          status = reader.read(serialized_data);
          if (!status.ok()) {
            reader.close();
            return status;
          }

          if (serialized_data.length() <= 0) {
            break;
          }
          uint64_t key;

          value_t* value = newValue();
          if (!parseFrom(serialized_data, &key, value)) {
            return Status::Corruption("Parse from string.");
          } else {
            if (level_table_.add(key, value)) {
              std::ostringstream oss;

              oss<<"Insert level table: key=0x"<<std::hex<<key;
              reader.close();
              return Status::Corruption(oss.str());
            }
          }
        } 
        while(serialized_data.length() > 0);

        reader.close();
        level_table_.deepen();

        return recoverAheadLog();
      }

    template<typename V>
      inline Status BaseTable<V>::recoveryAheadLog(const std::string& name, WALWriter* writer)
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

          if (serialized_data.length() > 0) {
            if (rollback(serialized_data)) {
              status = writer->append(serialized_data);
              if (!status.ok()) {
                reader.close();
                return status;
              }
            }
          }
        }
        while (serialized_data.length() > 0);
        reader.close();

        return Status::OK();
      }

    template<typename V>
      inline Status BaseTable<V>::recoverAheadLog() {
        std::string recovery_name = workPath() + "/wal.recovery";
        std::string writing_name = workPath() + "/wal.writing";

        if (access(recovery_name.c_str(), F_OK)) {
          if (NULL == wal_)
            wal_ = new WALWriter(writing_name);

          return recoveryAheadLog(recovery_name, wal_);
        } else {
          if (access(writing_name.c_str(), F_OK)) {
            if (NULL == wal_)
              wal_ = new WALWriter(writing_name);

            return wal_->create(tableVersion());
          } else {
            if (rename(writing_name.c_str(), recovery_name.c_str())) {
              std::ostringstream oss;

              oss<<"Rename file failed: "<<strerror(errno);
              oss<<"old="<<writing_name<<",new="<<recovery_name;
              return Status::IOError(oss.str());
            }

            if (NULL == wal_)
              wal_ = new WALWriter(writing_name);

            return recoveryAheadLog(recovery_name, wal_);
          }
        }
      }

    //与flush采用不同的周期,只保留days天
    template<typename V>
      inline Status BaseTable<V>::rollOverLogFile() {
        int i = 0;
        char last_name[300];

        pthread_mutex_lock(&mutex_);
        level_table_.deepen();
        wal_->close();

        for (;;) {
          sprintf(last_name, "%s/wal-%d.dat", workPath().c_str(), i);
          if (access(last_name, F_OK)) {
            continue;
          }

          if (rename(wal_->filename().c_str(), last_name)) {
            std::ostringstream oss;

            oss<<"Rename file failed: "<< strerror(errno)
              <<", oldfile: "<<wal_->filename()<<", newfile: "<<last_name;
            pthread_mutex_unlock(&mutex_);
            return Status::IOError(oss.str());
          }
          break;
        }
        Status status = wal_->create(tableVersion());
        if (!status.ok()) {
          pthread_mutex_unlock(&mutex_);
          return status; 
        }
        pthread_mutex_unlock(&mutex_);

        return Status::OK();
      }

    // WAL
    template<typename V>
      Status BaseTable<V>::writeAheadLog(const std::string& data) {
        pthread_mutex_lock(&mutex_);
        Status status = wal_->append(data);
        if (status.ok()) {
          pthread_mutex_unlock(&mutex_);
          return status;
        }
        pthread_mutex_unlock(&mutex_);

        return Status::OK();
      }
  } // namespace news
} // namespace rsys
