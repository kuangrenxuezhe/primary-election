#include "catch.hpp"
#include "util/ahead_log.h"

using namespace rsys::news;
class MockAheadLog: public AheadLog {
  public:
    MockAheadLog(const std::string& path, const fver_t& fver)
      : AheadLog(path, "wal", fver) {
    }
    virtual ~MockAheadLog() {
    }
    // ahead log滚存触发器
    virtual Status trigger() {
      return Status::OK();
    }
    // 处理数据回滚
    virtual Status rollback(const std::string& data) {
      logs_.push_back(data);
      return Status::OK();
    }
  public:
    std::vector<std::string> logs_;
};

SCENARIO("ahead日志测试", "[base]") {
  GIVEN("给定空ahead日志") {
    fver_t fver(1,8);

    WHEN("recovery空文件时") {
      if (0 == access("./wal.writing", F_OK))
        remove("./wal.writing");
      MockAheadLog mylog(".", fver);
      Status status = mylog.open();
      REQUIRE(0 == mylog.logs_.size());
      REQUIRE(status.ok());
      status = mylog.write("test1");
      REQUIRE(status.ok());
      status = mylog.write("test2");
      REQUIRE(status.ok());
      mylog.close();
    }

    WHEN("recovery上次文件") {
      MockAheadLog mylog1(".", fver);
      Status status = mylog1.open();
      REQUIRE(status.ok()); 
      REQUIRE(2 == mylog1.logs_.size());
      REQUIRE(mylog1.logs_[0] == "test1");
      REQUIRE(mylog1.logs_[1] == "test2");
      remove("./wal.writing");
    }

    WHEN("写入多个连续log文件") {
      MockAheadLog mylog(".", fver);
      Status status = mylog.open();
      REQUIRE(0 == mylog.logs_.size());
      REQUIRE(status.ok());
      status = mylog.write("test1");
      REQUIRE(status.ok());
      status = mylog.rollover();
      REQUIRE(status.ok());
      status = mylog.write("test2");
      REQUIRE(status.ok());
      status = mylog.rollover();
      REQUIRE(status.ok());
      status = mylog.write("test3");
      REQUIRE(status.ok());
      mylog.close();

      MockAheadLog mylog1(".", fver);
      status = mylog1.open();
      REQUIRE(status.ok()); 
      REQUIRE(3 == mylog1.logs_.size());
      REQUIRE(mylog1.logs_[0] == "test1");
      REQUIRE(mylog1.logs_[1] == "test2");
      REQUIRE(mylog1.logs_[2] == "test3");
      remove("./wal.writing");
      remove("./wal-0.dat");
      remove("./wal-1.dat");
    }
  }
}

