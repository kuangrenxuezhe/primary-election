#include "catch.hpp"

#include <stdio.h>
#include <unistd.h>

#include "util/file.h"

using namespace rsys::news;
SCENARIO("测试文件reader/writer", "[base]") {
  GIVEN("给定一个文件名") {
    std::string filename = "test_file_xdfe";
    if (!access(filename.c_str(), F_OK))
      remove(filename.c_str());

    WHEN("创建一个空文件") {
      FileWriter writer(filename);
      Status status = writer.create();

      THEN("文件被创建") {
        REQUIRE(status.ok());
        REQUIRE(0 == access(filename.c_str(), F_OK));
        writer.close();
      }
      remove(filename.c_str());
    }

    WHEN("当文件存在时") {
      FileWriter touch(filename);
      Status status = touch.create();
      if (status.ok())
        touch.close();
      REQUIRE(0 == access(filename.c_str(), F_OK));

      FileWriter writer(filename);
      status = writer.create();

      THEN("文件能被打开") {
        REQUIRE(status.ok());
        writer.close();
      }
      remove(filename.c_str());
    }

    WHEN("文件被锁定") {
      FileWriter writer(filename);
      Status status = writer.create();
      REQUIRE(status.ok());
      status = writer.lockfile();
      REQUIRE(status.ok());

      THEN("文件不可再被锁定") {
        FileWriter try_lock(filename);
        status = try_lock.create();
        REQUIRE(status.ok());
        status = try_lock.lockfile();
        REQUIRE(!status.ok());
        try_lock.close();
      }
      writer.close();
      remove(filename.c_str());
    }

    WHEN("文件不存在时") {
      FileReader reader(filename);
      if (!access(filename.c_str(), F_OK))
        remove(filename.c_str());
      THEN("文件打开失败") {
        Status status = reader.open();
        REQUIRE(!status.ok());
      }
    }

    WHEN("存在空文件") {
      FileWriter writer(filename);
      Status status = writer.create();
      REQUIRE(status.ok());
      writer.close();

      THEN("可正常打开但无数据") {
        FileReader reader(filename);
        status = reader.open();
        REQUIRE(status.ok());

        std::string data;
        status = reader.read(data);
        REQUIRE(!status.ok());
        reader.close();
      }
      remove(filename.c_str());
    }

    WHEN("存在正常的数据文件") {
      FileWriter writer(filename);
      Status status = writer.create();
      REQUIRE(status.ok());

      std::string data = "test";
      status = writer.write(data);
      REQUIRE(status.ok());
      writer.close();

      THEN("可正常读取数据") {
        FileReader reader(filename);
        status = reader.open();
        REQUIRE(status.ok());

        std::string rdata;
        status = reader.read(rdata);
        REQUIRE(status.ok());
        REQUIRE(rdata == data);

        status = reader.read(rdata);
        REQUIRE(!status.ok());
        reader.close();
      }
      remove(filename.c_str());
    }

    WHEN("存在正常的数据文件") {
      FileWriter writer(filename);
      Status status = writer.create();
      REQUIRE(status.ok());

      std::string data = "test";
      status = writer.write(data);
      REQUIRE(status.ok());
      writer.close();

      FileWriter rewriter(filename);
      status = rewriter.create();
      REQUIRE(status.ok());

      std::string redata = "re";
      status = rewriter.write(redata);
      REQUIRE(status.ok());
      rewriter.close();

      THEN("覆盖原始的文件") {
        FileReader reader(filename);
        status = reader.open();
        REQUIRE(status.ok());

        std::string rdata;
        status = reader.read(rdata);
        REQUIRE(status.ok());
        REQUIRE(redata== rdata);

        status = reader.read(rdata);
        REQUIRE(!status.ok());
        reader.close();
      }
      remove(filename.c_str());
    }
  }
}

