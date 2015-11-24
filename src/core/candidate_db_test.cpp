#include "catch.hpp"
#include "core/options.h"
#include "core/candidate_db.h"

using namespace rsys::news;
SCENARIO("CandidateDB测试", "[base]") {
  GIVEN("创建空的CandidateDB") {
    Options options;
    CandidateDB* candb = NULL;

    WHEN("创建空候选集库") {
      Status status = CandidateDB::openDB(options, &candb);
      THEN("CandidateDB创建成功") {
        REQUIRE(status.ok());
      }
    }
    delete candb;
  }

  GIVEN("创建空的CandidateDB") {
    Options options;
    CandidateDB* candb = NULL;
    Status status = CandidateDB::openDB(options, &candb);
    REQUIRE(status.ok());
    WHEN("二次打开候选集库") {
      status = CandidateDB::openDB(options, &candb);
      THEN("CandidateDB打开失败") {
        REQUIRE(!status.ok());
      }
    }
    delete candb;
  }

  GIVEN("Candidate空库") {
    Options options;
    CandidateDB* candb = NULL;
    Status status = CandidateDB::openDB(options, &candb);
    REQUIRE(status.ok());

    WHEN("添加item数据") {
      Item item;
      item.set_item_id(1);
      item.set_publish_time(time(NULL));

      status = candb->addItem(item);
      REQUIRE(status.ok());

      THEN("数据添加成功") {
        proto::ItemQuery query;
        query.set_item_id(1);
        proto::ItemInfo item_info;
        status = candb->queryItemInfo(query, item_info);
        REQUIRE(status.ok());
        REQUIRE(item_info.item_id() == item.item_id());
        REQUIRE(item_info.publish_time() == item.publish_time());
      }
    }

    WHEN("添加subscribe数据") {
      Subscribe subscribe;
      subscribe.set_user_id(1);
      subscribe.add_srp_id("test");

      status = candb->updateSubscribe(subscribe);
      REQUIRE(status.ok());

      THEN("数据更新成功") {
        proto::UserInfo user_info;
        proto::UserQuery query;
        query.set_user_id(1);
        status = candb->queryUserInfo(query, user_info);
        REQUIRE(status.ok());
        REQUIRE(user_info.user_id() == query.user_id());
        REQUIRE(user_info.subscribe_size() == 1);
        REQUIRE(user_info.subscribe(0).str() == subscribe.srp_id(0));
      }
    }

    WHEN("用户操作更新") {
      Item item;
      item.set_item_id(1);
      item.set_publish_time(time(NULL));

      status = candb->addItem(item);
      REQUIRE(status.ok());

      Subscribe subscribe;
      subscribe.set_user_id(1);
      subscribe.add_srp_id("test");

      status = candb->updateSubscribe(subscribe);
      REQUIRE(status.ok());

      Action action;
      action.set_user_id(1);
      action.set_item_id(1);
      action.set_click_time(time(NULL));
      action.set_action(ACTION_TYPE_CLICK);

      status = candb->updateAction(action, action);
      REQUIRE(status.ok());

      THEN("数据状态更新") {
        proto::UserInfo user_info;
        proto::UserQuery query;
        query.set_user_id(1);
        status = candb->queryUserInfo(query, user_info);
        REQUIRE(status.ok());
        REQUIRE(user_info.readed_size() == 1);

        proto::ItemQuery query1;
        query1.set_item_id(1);
        proto::ItemInfo item_info;
        status = candb->queryItemInfo(query1, item_info);
        REQUIRE(status.ok());
        REQUIRE(item_info.click_count() == 2); 
      }
    }

    WHEN("推荐反馈") {
      Subscribe subscribe;
      subscribe.set_user_id(1);
      subscribe.add_srp_id("test");

      status = candb->updateSubscribe(subscribe);
      REQUIRE(status.ok());

      Feedback feedback;
      feedback.set_user_id(1);
      feedback.add_item_id(11);

      status = candb->updateFeedback(feedback);
      REQUIRE(status.ok());

      THEN("数据状态更新") {
        proto::UserInfo user_info;
        proto::UserQuery query;
        query.set_user_id(1);
        status = candb->queryUserInfo(query, user_info);
        REQUIRE(status.ok());
        REQUIRE(user_info.recommended_size() == 1);
      }
    }

    WHEN("添加数据获取推荐") {
      Subscribe subscribe;
      subscribe.set_user_id(1);
      subscribe.add_srp_id("test");

      status = candb->updateSubscribe(subscribe);
      REQUIRE(status.ok());

      Item item;
      item.set_item_id(1);
      item.set_publish_time(time(NULL));

      status = candb->addItem(item);
      REQUIRE(status.ok());

      item.set_item_id(2);
      item.set_publish_time(time(NULL));

      status = candb->addItem(item);
      REQUIRE(status.ok());


      Feedback feedback;
      feedback.set_user_id(1);
      feedback.add_item_id(1);

      status = candb->updateFeedback(feedback);
      REQUIRE(status.ok());

      THEN("数据状态更新") {
        Recommend recmd;
        CandidateSet candset;

        recmd.set_user_id(1);
        recmd.set_beg_time(time(NULL) - 86400);
        recmd.set_end_time(time(NULL));
        status = candb->queryCandidateSet(recmd, candset);
        REQUIRE(status.ok());
      }
    }
    delete candb;
  }
}

