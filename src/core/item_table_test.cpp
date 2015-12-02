#include "catch.hpp"
#include "core/item_table.h"
#include "proto/service.pb.h"

using namespace rsys::news;

SCENARIO("测试item表", "[base]") {
  GIVEN("给定空的item表") {
    Options opts;
    ItemTable table(opts);
    Status status = table.loadTable();
    REQUIRE(status.ok());

    WHEN("添加新item时") {
      item_info_t* item = new item_info_t;
      item->item_type = kNormalItem;
      item->item_id = 1;
      item->publish_time = time(NULL);
      status = table.addItem(item);
      REQUIRE(status.ok());

      THEN("候选集合只有一个item") {
        query_t query;
        candidate_set_t cand_set;

        query.item_type = kNormalItem;
        query.region_id = 0UL;
        query.start_time = time(NULL) - 10;
        query.end_time = time(NULL);

        status = table.queryCandidateSet(query, cand_set);
        REQUIRE(status.ok());
        REQUIRE(cand_set.size() == 1);
        REQUIRE(cand_set.begin()->item_id == 1);

        item_info_t item_info;
        status = table.queryItem(1, item_info);
        REQUIRE(item_info.publish_time == item->publish_time);
      }
    }

    WHEN("更新item点击") {
      item_info_t* item = new item_info_t;
      item->item_id = 1;
      item->publish_time = time(NULL);
      item->click_count = 0;
      item->click_time = 1;
      Status status = table.addItem(item);
      REQUIRE(status.ok());

      action_t action;

      action.item_id = 1;
      status = table.updateAction(1, action);
      REQUIRE(status.ok());

      THEN("状态变化") {
        item_info_t item_info;
        status = table.queryItem(1, item_info);
        REQUIRE(status.ok());
        REQUIRE(item_info.click_count == 1);
        REQUIRE(item_info.click_time == 1);
      }
    }
    status = table.flushTable();
    REQUIRE(status.ok());
  }

  GIVEN("给定item表") {
    Options opts;
    ItemTable table(opts);

    WHEN("加载item表") {
      Status status = table.loadTable();
      REQUIRE(status.ok());

      THEN("加载成功") {
        item_info_t item_info;
        status = table.queryItem(1, item_info);
        REQUIRE(status.ok());
        REQUIRE(item_info.click_count == 1);
        REQUIRE(item_info.click_time == 1);
      }
      remove("./wal-item.writing");
      remove("./table-item.dat");
    }
  }
}

