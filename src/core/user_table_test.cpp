#include "catch.hpp"
#include "core/user_table.h"
#include "proto/service.pb.h"

using namespace rsys::news;

SCENARIO("测试User表", "[base]") {
  GIVEN("空用户表") {
    Options opts;
    UserTable table(opts);

    WHEN("添加用户信息") {
      user_info_t* user_info = new user_info_t;

      user_info->ctime = 10;
      user_info->dislike.insert(std::make_pair(1, "test"));
      Status status = table.updateUser(1, user_info);
      REQUIRE(status.ok());
      THEN("用户存在") {
        REQUIRE(table.findUser(1));
        REQUIRE(!table.findUser(2));

        user_info_t* user_info = new user_info_t;
        status = table.getUser(1, user_info);
        REQUIRE(status.ok());
        REQUIRE(user_info->ctime == 10);
        REQUIRE(user_info->dislike.size() == 1);
        REQUIRE(user_info->dislike[1] == std::string("test"));
      }
    }
  }

  GIVEN("给定用户") {
    Options opts;
    UserTable table(opts);
    user_info_t* user_info = new user_info_t;

    user_info->ctime = 10;
    Status status = table.updateUser(1, user_info);
    REQUIRE(status.ok());

    WHEN("用户点击更新") {
      action_t click;
      click.item_id = 1;
      click.action = ACTION_TYPE_CLICK;

      Status status = table.updateAction(1, click);
      REQUIRE(status.ok());
      
      THEN("状态已读列表变化") {
        user_info_t* user_info = new user_info_t;
        status = table.getUser(1, user_info);
        REQUIRE(status.ok());
        REQUIRE(user_info->readed.size() == 1);

        map_time_t::iterator iter = user_info->readed.begin();
        REQUIRE(iter->first == 1);
      }
    }

    WHEN("用户不喜欢更新") {
      action_t click;
      click.item_id = 1;
      click.action = ACTION_TYPE_DISLIKE;
      click.dislike_reason = "1_说明";

      Status status = table.updateAction(1, click);
      REQUIRE(status.ok());

      THEN("状态已读列表变化") {
        user_info_t* user_info = new user_info_t;
        status = table.getUser(1, user_info);
        REQUIRE(status.ok());
        REQUIRE(user_info->dislike.size() == 1);

        map_str_t::iterator iter = user_info->dislike.begin();
        REQUIRE(iter->second == click.dislike_reason);
      }
    }

    WHEN("用户候选集合更新") {
      id_set_t id_set;
      id_set.insert(1);

      Status status = table.updateCandidateSet(1, id_set);
      REQUIRE(status.ok());

      THEN("候选集合列表变化") {
        user_info_t* user_info = new user_info_t;
        status = table.getUser(1, user_info);
        REQUIRE(status.ok());
        REQUIRE(user_info->recommended.size() == 1);
      }
    }
  }

  GIVEN("带有已读和推荐数据用户") {
    Options opts;
    UserTable table(opts);

    user_info_t* user_info = new user_info_t;

    user_info->ctime = 10;
    user_info->readed.insert(std::make_pair(1, 10));
    user_info->recommended.insert(std::make_pair(2, 20));
    user_info->dislike.insert(std::make_pair(1, "test"));
    Status status = table.updateUser(1, user_info);
    REQUIRE(status.ok());

    WHEN("获取用户已读历史") {
      id_set_t id_set;
      status = table.queryHistory(1, id_set);
      REQUIRE(status.ok());
      THEN("") {
        REQUIRE(id_set.size() == 1);
        REQUIRE(*(id_set.begin()) == 1);
      }
    }

    WHEN("过滤已推荐记录") {
      candidate_set_t cand_set;
      candidate_t cand;
      cand.item_id = 1;
      cand_set.push_back(cand);
      cand.item_id = 2;
      cand_set.push_back(cand);
      cand.item_id = 3;
      cand_set.push_back(cand);
      status = table.filterCandidateSet(1, cand_set);
      REQUIRE(status.ok());
      THEN("已推荐数据被过滤掉") {
        REQUIRE(cand_set.size() == 1);
        candidate_set_t::iterator iter = cand_set.begin();
        REQUIRE(iter->item_id == 3);
      }
    }
  }
}

