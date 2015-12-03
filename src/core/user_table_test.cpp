#include "catch.hpp"
#include "core/user_table.h"
#include "proto/service.pb.h"

using namespace rsys::news;

SCENARIO("测试User表", "[base]") {
  GIVEN("空用户表") {
    Options opts;
    UserTable table(opts);
    Status status = table.loadTable();
    REQUIRE(status.ok());

    WHEN("添加用户信息") {
      map_str_t subscribe;
      subscribe.insert(std::make_pair(1, "test"));

      Status status = table.updateUser(1, subscribe);
      REQUIRE(status.ok());
      THEN("用户存在") {
        REQUIRE(table.findUser(1));
        REQUIRE(!table.findUser(2));

        user_info_t user_info;
        status = table.queryUser(1, user_info);
        REQUIRE(status.ok());
        REQUIRE(user_info.subscribe.size() == 1);
        REQUIRE(user_info.subscribe[1] == std::string("test"));
      }
    }
    remove("./wal-user.writing");
  }

  GIVEN("给定用户") {
    Options opts;
    UserTable table(opts);
    Status status = table.loadTable();
    REQUIRE(status.ok());

    map_str_t subscribe;
    subscribe.insert(std::make_pair(1, "test"));

    status = table.updateUser(1, subscribe);
    REQUIRE(status.ok());

    WHEN("用户点击更新") {
      action_t click;
      click.item_id = 1;
      click.action = ACTION_TYPE_CLICK;

      Status status = table.updateAction(1, click);
      REQUIRE(status.ok());
      
      THEN("状态已读列表变化") {
        user_info_t user_info;
        status = table.queryUser(1, user_info);
        REQUIRE(status.ok());

        REQUIRE(user_info.subscribe.size() == 1);
        REQUIRE(user_info.subscribe[1] == std::string("test"));

        REQUIRE(user_info.readed.size() == 1);
        map_time_t::iterator iter = user_info.readed.begin();
        REQUIRE(iter->first == 1);
      }
    }

    WHEN("用户不喜欢更新") {
      action_t click;
      click.item_id = 1;
      click.action = ACTION_TYPE_DISLIKE;
      click.dislike_reason = "\"1_id\":\"说明\",\"1\":\"重复内容\"";

      Status status = table.updateAction(1, click);
      REQUIRE(status.ok());

      THEN("状态已读列表变化") {
        user_info_t user_info;
        status = table.queryUser(1, user_info);
        REQUIRE(status.ok());
        REQUIRE(user_info.dislike.size() == 2);
      }
    }

    WHEN("用户候选集合更新") {
      id_set_t id_set;
      id_set.insert(1);

      Status status = table.updateFeedback(1, id_set);
      REQUIRE(status.ok());

      THEN("候选集合列表变化") {
        user_info_t user_info;
        status = table.queryUser(1, user_info);
        REQUIRE(status.ok());
        REQUIRE(user_info.recommended.size() == 1);
      }
    }
    remove("./wal-user.writing");
  }

  GIVEN("带有已读和推荐数据用户") {
    Options opts;
    UserTable table(opts);
    Status status = table.loadTable();
    REQUIRE(status.ok());

    map_str_t subscribe;
    subscribe.insert(std::make_pair(1, "test"));

    status = table.updateUser(1, subscribe);
    REQUIRE(status.ok());

    WHEN("获取用户已读历史") {
      action_t click;
      click.item_id = 1;
      click.action = ACTION_TYPE_CLICK;

      Status status = table.updateAction(1, click);
      REQUIRE(status.ok());
      THEN("已读历史") {
        id_set_t id_set;
        status = table.queryHistory(1, id_set);
        REQUIRE(status.ok());
        REQUIRE(id_set.size() == 1);
        REQUIRE(*(id_set.begin()) == 1);
      }
    }

    WHEN("过滤已推荐记录") {
      action_t click;
      click.item_id = 1;
      click.action = ACTION_TYPE_CLICK;

      Status status = table.updateAction(1, click);
      REQUIRE(status.ok());

      id_set_t id_set;
      id_set.insert(2);

      status = table.updateFeedback(1, id_set);
      REQUIRE(status.ok());

      THEN("已推荐数据被过滤掉") {
        candidate_set_t cand_set;
        item_info_t cand;
        cand.item_id = 1;
        cand_set.push_back(cand);
        cand.item_id = 2;
        cand_set.push_back(cand);
        cand.item_id = 3;
        cand_set.push_back(cand);
        status = table.filterCandidateSet(1, cand_set);
        REQUIRE(status.ok());

        REQUIRE(cand_set.size() == 1);
        candidate_set_t::iterator iter = cand_set.begin();
        REQUIRE(iter->item_info.item_id == 3);
      }
    }
    remove("./wal-user.writing");
  }

  GIVEN("正常写入用户表") {
    Options opts;
    UserTable table(opts);
    Status status = table.loadTable();
    REQUIRE(status.ok()); 

    WHEN("添加新用户") {
      map_str_t subscribe;
      subscribe.insert(std::make_pair(1, "test"));

      status = table.updateUser(1, subscribe);
      REQUIRE(status.ok());

      THEN("正常写入") {
        status = table.flushTable();
        REQUIRE(status.ok());
      }
    }
  }

  GIVEN("正常读取用户表") {
    Options opts;
    UserTable table(opts);
    Status status = table.loadTable();
    REQUIRE(status.ok()); 

    WHEN("获取新用户") {
      user_info_t user_info;
      status = table.queryUser(1, user_info);
      REQUIRE(status.ok());

      THEN("正常写入") {
        REQUIRE(user_info.subscribe.size() == 1);
        map_str_t::iterator iter = user_info.subscribe.begin();
        REQUIRE(iter->first == 1);
        REQUIRE(iter->second == std::string("test"));
      }
    }
    remove("./table-user.dat");
    remove("./wal-user.writing");
  }

  GIVEN("给定已有用户表") {
    Options opts;
    opts.user_hold_time = 2;
    UserTable table(opts);
    Status status = table.loadTable();
    REQUIRE(status.ok()); 
    
    map_str_t subscribe;
    status = table.updateUser(1, subscribe);
    REQUIRE(status.ok());

    subscribe.insert(std::make_pair(1, "test"));
    status = table.updateUser(2, subscribe);
    REQUIRE(status.ok());
    
    action_t action;
    action.action = ACTION_TYPE_DISLIKE;
    action.dislike_reason = "\"1_xx\":\"tt\"";
    status = table.updateAction(2, action);
    REQUIRE(status.ok());

    sleep(3);

    map_str_t subscribe1;
    status = table.updateUser(3, subscribe1);
    REQUIRE(status.ok());

    status = table.flushTable();
    REQUIRE(status.ok());

    WHEN("淘汰过期用户") {
      table.eliminate();

      THEN("剩余用户") {
        REQUIRE(table.findUser(2));
        REQUIRE(table.findUser(3));
        REQUIRE(!table.findUser(1));
      }
    }
    remove("./table-user.dat");
    remove("./wal-user.writing");
  }
}

