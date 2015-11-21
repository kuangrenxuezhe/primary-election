#include "catch.hpp"
#include "util/level_table.h"

using namespace rsys::news;

class MyUpdater: public LevelTable<uint64_t, uint64_t>::Updater {
  public:
    MyUpdater(uint64_t new_value) {
      new_value_ = new_value;
    }
    virtual ~MyUpdater() {
    }
  public:
    virtual bool update(uint64_t* value) {
      *value = new_value_;
      return true;
    }
    virtual uint64_t* clone(uint64_t* value) {
      return new uint64_t(*value);
    }
  private:
    uint64_t new_value_;
};
class MyVistor: public LevelTable<uint64_t, uint64_t>::Updater {
  public:
    MyVistor(uint64_t valid) {
      valid_value_ = valid;
    }
    virtual ~MyVistor() {
    }
  public:
    virtual bool update(uint64_t* value) {
      REQUIRE(valid_value_ == *value);
      return true;
    }
    virtual uint64_t* clone(uint64_t* value) {
      return new uint64_t(*value);
    }
  private:
    uint64_t valid_value_;
};
typedef LevelTable<uint64_t, uint64_t>::Iterator LTIterator;
SCENARIO("测试level table", "[base]") {
  GIVEN("给定一个空的level table") {
    LevelTable<uint64_t, uint64_t> table(3);

    THEN("空操作") {
      REQUIRE(!table.find(1));
      REQUIRE(!table.erase(1));
      REQUIRE(1 == table.depth());
      REQUIRE(!table.merge());

      LTIterator iter = table.snapshot();
      REQUIRE(!iter.valid());
    }

    THEN("增删改操作") {
      uint64_t* v = new uint64_t;
      *v = 10;
      REQUIRE(table.add(1, v));
      REQUIRE(table.find(1));
      MyUpdater my(20);
      REQUIRE(table.update(1, my));
      MyVistor vistor(20);
      REQUIRE(table.update(1, vistor));
      LTIterator iter = table.snapshot();
      REQUIRE(!iter.valid());

      REQUIRE(table.erase(1));
      REQUIRE(!table.find(1));
    }

    THEN("层级合并") {
      uint64_t* v = new uint64_t;
      *v = 10;
      REQUIRE(table.add(1, v));
      v = new uint64_t;
      *v = 20;
      REQUIRE(table.add(2, v));

      REQUIRE(table.deepen());
      REQUIRE(2 == table.depth());
      MyVistor vistor(10);
      REQUIRE(table.update(1, vistor));
      MyVistor vistor1(20);
      REQUIRE(table.update(2, vistor1));

      v = new uint64_t;
      *v = 30;
      REQUIRE(table.add(3, v));  

      v = new uint64_t;
      *v = 40;
      REQUIRE(table.add(2, v));  
      MyVistor vistor2(40);
      REQUIRE(table.update(2, vistor2));

      LTIterator iter = table.snapshot();
      REQUIRE(iter.valid());
      REQUIRE(1 == iter.key());
      REQUIRE(10 == *iter.value());
      iter.next();
      REQUIRE(2 == iter.key());
      REQUIRE(20 == *iter.value());
      iter.next();
      REQUIRE(!iter.hasNext());

      REQUIRE(table.deepen());
      REQUIRE(3 == table.depth());
      REQUIRE(table.find(1));
      REQUIRE(table.find(2));
      REQUIRE(table.find(3));
      REQUIRE(!table.deepen());

      REQUIRE(table.merge());
      REQUIRE(2 == table.depth());

      LTIterator iter1 = table.snapshot();
      REQUIRE(iter1.valid());
      REQUIRE(1 == iter1.key());
      iter1.next();
      REQUIRE(2 == iter1.key());
      iter1.next();
      REQUIRE(3 == iter1.key());
      iter1.next();
      REQUIRE(!iter1.hasNext());
    }
  }
}
