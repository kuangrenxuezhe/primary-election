#include "catch.hpp"
#include "util/table_base.h"
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
      : AheadLog(path, fver) {
        table_ = table;
      }
    virtual ~MyAheadLog() {
    }
    // ahead log滚存触发器
    virtual bool trigger();
    // 处理数据回滚
    virtual bool rollback(const std::string& data);
  public:
    MyTableBase* table_;
};

class MyTableBase: public TableBase<uint64_t> {
  public:
    MyTableBase(const std::string& path, const std::string& name, const fver_t& ver, size_t level)
      : TableBase(path, name, ver, level) {
    }
    virtual ~MyTableBase() {
    }

  public:
    LevelTable<uint64_t, uint64_t>* table() {
      return level_table();
    }
    bool deepenTable() {
      return level_table()->deepen();
    }
    bool addKeyValue(uint64_t key, uint64_t* value) {
      return level_table()->add(key, value);
    }
    // 创建value对象 
    virtual uint64_t* newValue() {
      return new uint64_t;
    }
    // 创建AheadLog对象
    virtual AheadLog* createAheadLog() {
      fver_t fver(1, 8);
      return new MyAheadLog(this, ".", fver);
    }

  public:
    // 序列化和反序列话对象
    virtual bool parseFrom(const std::string& data, uint64_t* key, uint64_t* value) {
      const char* p = data.c_str();
      char* pend = NULL;

      //fprintf(stdout, "parse: %s\n", data.c_str());
      *key = strtoul(p, &pend, 10);
      pend++;
      p = pend;
      *value  = strtoul(pend, NULL, 10);
      return true;
    }
    virtual bool serializeTo(uint64_t key, const uint64_t* value,  std::string& data) {
      std::ostringstream oss;
      oss<<key<<"|"<<*value;
      data = oss.str();
      //fprintf(stdout, "serialize: %s\n", data.c_str());
      return true;
    }
};
// ahead log滚存触发器
inline bool MyAheadLog::trigger() {
  return table_->deepenTable();
}
// 处理数据回滚
inline bool MyAheadLog::rollback(const std::string& data) {
  uint64_t key;
  uint64_t* value = new uint64_t;

  if (!table_->parseFrom(data, &key, value))
    return false;

  return table_->addKeyValue(key, value);
}

SCENARIO("TableBase测试", "[base]") {
  GIVEN("给定空的Table") {
    fver_t fver(1,8);
    MyTableBase table(".", "test_table_base", fver, 3);

    WHEN("创建空表") {
      Status status = table.loadTable();
      REQUIRE(status.ok());
      remove("./wal.writing");
    }

    WHEN("写ahead log") {
      Status status = table.loadTable();
      REQUIRE(status.ok());
      status = table.writeAheadLog("1|10");
      REQUIRE(status.ok());
      status = table.writeAheadLog("2|20");
      REQUIRE(status.ok());

      THEN("从ahead log回复数据") {
        Status status = table.loadTable();
        REQUIRE(status.ok());
        REQUIRE(table.table()->find(1));
        REQUIRE(table.table()->find(2));
        //remove("./wal.writing");
      }
    }
  }

  GIVEN("给定已有Table") {
    fver_t fver(1,8);

    WHEN("写入数据") {
      MyTableBase table(".", "test_table_base", fver, 3);

      Status status = table.loadTable();
      REQUIRE(status.ok());
      status = table.writeAheadLog("1|10");
      REQUIRE(status.ok());
      status = table.writeAheadLog("2|20");
      REQUIRE(status.ok());
      THEN("flushTable") {
        status = table.flushTable();
        REQUIRE(status.ok());
        status = table.writeAheadLog("3|30");
        REQUIRE(status.ok());
      }
    }

    WHEN("重新加载数据") {
      MyTableBase table(".", "test_table_base", fver, 3);

      Status status = table.loadTable();
      REQUIRE(status.ok());
      REQUIRE(table.table()->find(1));
      REQUIRE(table.table()->find(2));
      REQUIRE(table.table()->find(3));
      remove("./wal.writing");
      remove("./test_table_base");
    }
  }
}
