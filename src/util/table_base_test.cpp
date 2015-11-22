#include "catch.hpp"
#include "util/table_base.h"

#include <list>
#include <string>
#include <iostream>

using namespace rsys::news;
namespace Catch {
  std::string toString(Status const& value ) {
    return value.toString();
  }
  template<> struct StringMaker<Status> {
    static std::string convert( Status const& value ) {
      return value.toString();
    } 
  }; 
}

class MyTableBase;
class MyAheadLog: public AheadLog {
  public:
    MyAheadLog(MyTableBase* table, const std::string& path, const fver_t& fver)
      : AheadLog(path, "wal", fver) {
        table_ = table;
      }
    virtual ~MyAheadLog() {
    }
    // ahead log滚存触发器
    virtual Status trigger();
    // 处理数据回滚
    virtual Status rollback(const std::string& data);
  public:
    MyTableBase* table_;
};

class MyTableBase: public TableBase {
  public:
    MyTableBase(const std::string& path, const std::string& name, const fver_t& ver)
      : TableBase(path, name, ver) {
    }
    virtual ~MyTableBase() {
    }

  public:
    void addData(const std::string& data) {
      table_.push_back(data);
    }
    // 创建AheadLog对象
    virtual AheadLog* createAheadLog() {
      fver_t fver(1, 8);
      return new MyAheadLog(this, ".", fver);
    }
    virtual Status loadData(const std::string& data) {
      table_.push_back(data);
      return Status::OK();
    }
    virtual Status dumpToFile(const std::string& name) {
      TableFileWriter writer(name);
      Status status = writer.create(file_version());
      if (!status.ok()) 
        return status;
      std::list<std::string>::iterator iter = table_.begin();
      for (; iter != table_.end(); ++iter) {
        status = writer.write(*iter);
        if (!status.ok()) {
          writer.close();
          return status;
        }
      }
      writer.close();
      return Status::OK();
    }

  public:
    std::list<std::string> table_;
};
// ahead log滚存触发器
inline Status MyAheadLog::trigger() {
  return Status::OK();
}
// 处理数据回滚
inline Status MyAheadLog::rollback(const std::string& data) {
  table_->addData(data);
  return Status::OK();
}

SCENARIO("TableBase测试", "[base]") {
  fver_t fver(1,8);

  GIVEN("给定空的Table") {
    MyTableBase table(".", "test_table_base", fver);

    WHEN("创建空表") {
      Status status = table.loadTable();
      REQUIRE(status.ok());
      THEN("确认空表") {
        REQUIRE(table.table_.size() == 0);
      }
      remove("./wal.writing");
    }

    WHEN("写ahead log") {
      Status status = table.loadTable();
      REQUIRE(status.ok());

      status = table.writeAheadLog("1|10");
      REQUIRE(status.ok());
      status = table.writeAheadLog("2|20");
      REQUIRE(status.ok());
    }
  }

  GIVEN("给定已有Table") {
    MyTableBase table(".", "test_table_base", fver);

    WHEN("从ahead log恢复") {
      Status status = table.loadTable();
      REQUIRE(status.ok());

      THEN("确认恢复数据") {
        REQUIRE(table.table_.size() == 2);
      }
      status = table.flushTable();
      REQUIRE(status.ok());
    }
  }

  GIVEN("给定已有Table数据") {
    MyTableBase table(".", "test_table_base", fver);

    WHEN("重新加载数据") {
      Status status = table.loadTable();
      REQUIRE(status.ok());
      THEN("确认恢复数据") {
        REQUIRE(table.table_.size() == 2);
      }
      remove("./wal.writing");
      remove("./test_table_base");
    }
  }
}

