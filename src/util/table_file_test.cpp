#include "catch.hpp"

#include <unistd.h>
#include <stdio.h>
#include "util/table_file.h"

using namespace rsys::news;
SCENARIO("测试level file的reader/writer", "[base]") {
  GIVEN("给定level file") {
    std::string level_file = "test_level_file_dfef";
    if (!access(level_file.c_str(), F_OK))
      remove(level_file.c_str());

    fver_t fver(1, 9);
    TableFileWriter writer(level_file);
    Status status = writer.create(fver);
    REQUIRE(status.ok());
    REQUIRE(writer.filename() == level_file);

    std::string data = "test";
    status = writer.write(data);
    REQUIRE(status.ok());
    status = writer.close();
    REQUIRE(status.ok());

    THEN("正常读取level file") {
      TableFileReader reader(level_file);
      fver_t rfver;
      status = reader.open(rfver);
      REQUIRE(status.ok());
      REQUIRE(rfver.valid(fver));

      std::string rdata;
      status = reader.read(rdata);
      REQUIRE(status.ok());
      REQUIRE(rdata == data);

      //文件结尾
      status = reader.read(rdata);
      REQUIRE(status.ok());
      REQUIRE(0 == rdata.length());
      status = reader.close();
      REQUIRE(status.ok());
    }
    remove(level_file.c_str());
  }
}
