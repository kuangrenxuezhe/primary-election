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

  GIVEN("给定一个已打开的文件") {
    FileWriter writer("wal-test-lock");
    Status status = writer.create();
    REQUIRE(status.ok());
    status = writer.lockfile();
    REQUIRE(status.ok());

    WHEN("当文件被lock") {
      FileWriter rewriter("wal-test-lock");
      Status status = rewriter.create();
      REQUIRE(status.ok());
      THEN("文件无法被再次lock") {
        status = rewriter.lockfile();
        REQUIRE(!status.ok());
      }
      rewriter.close();
    }
    writer.close();
    remove("./wal-test-lock");
  }

  GIVEN("创建一个lock文件") {
    FileWriter writer("wal-test-trylock");
    Status status = writer.create();
    REQUIRE(status.ok());
    status = writer.lockfile();
    REQUIRE(status.ok());
    writer.close();

    WHEN("当文件被再次打开") {
      FileWriter rewriter("wal-test-trylock");
      Status status = rewriter.create();
      REQUIRE(status.ok());
      THEN("文件再次lock正常") {
        status = rewriter.lockfile();
        REQUIRE(status.ok());
      }
      rewriter.close();
    }
    remove("./wal-test-trylock");
  }
}

