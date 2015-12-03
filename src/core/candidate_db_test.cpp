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
        if (!status.ok())
          FAIL(status.toString());
      }
    }
    delete candb;
  }

  GIVEN("创建空的CandidateDB") {
    Options options;
    CandidateDB* candb = NULL;
    Status status = CandidateDB::openDB(options, &candb);
    if (!status.ok())
      FAIL(status.toString());
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
    if (!status.ok())
      FAIL(status.ok());

    WHEN("添加item数据") {
      Item item;
      item.set_item_id(1);
      item.set_publish_time(time(NULL));

      status = candb->addItem(item);
      if (!status.ok())
        FAIL(status.ok());

      THEN("数据添加成功") {
        proto::ItemQuery query;
        query.set_item_id(1);
        proto::ItemInfo item_info;
        status = candb->queryItemInfo(query, item_info);
        if (!status.ok())
          FAIL(status.ok());

        REQUIRE(item_info.item_id() == item.item_id());
        REQUIRE(item_info.publish_time() == item.publish_time());
      }
    }

    WHEN("添加subscribe数据") {
      Subscribe subscribe;
      subscribe.set_user_id(1);
      subscribe.add_srp_id("test");

      status = candb->updateSubscribe(subscribe);
      if (!status.ok())
        FAIL(status.ok());

      THEN("数据更新成功") {
        proto::UserInfo user_info;
        proto::UserQuery query;
        query.set_user_id(1);
        status = candb->queryUserInfo(query, user_info);
        if (!status.ok())
          FAIL(status.ok());

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
      if (!status.ok())
        FAIL(status.toString());

      Subscribe subscribe;
      subscribe.set_user_id(1);
      subscribe.add_srp_id("test");

      status = candb->updateSubscribe(subscribe);
      if (!status.ok())
        FAIL(status.toString());

      Action action;
      action.set_user_id(1);
      action.set_item_id(1);
      action.set_click_time(time(NULL));
      action.set_action(ACTION_TYPE_CLICK);

      status = candb->updateAction(action, action);
      if (!status.ok())
        FAIL(status.toString());

      THEN("数据状态更新") {
        proto::UserInfo user_info;
        proto::UserQuery query;
        query.set_user_id(1);
        status = candb->queryUserInfo(query, user_info);
        if (!status.ok())
          FAIL(status.toString());

        REQUIRE(user_info.readed_size() == 1);

        proto::ItemQuery query1;
        query1.set_item_id(1);
        proto::ItemInfo item_info;
        status = candb->queryItemInfo(query1, item_info);
        if (!status.ok())
          FAIL(status.toString());

        REQUIRE(item_info.click_count() == 2); 
      }
    }

    WHEN("推荐反馈") {
      Subscribe subscribe;
      subscribe.set_user_id(1);
      subscribe.add_srp_id("test");

      status = candb->updateSubscribe(subscribe);
      if (!status.ok())
        FAIL(status.toString());

      Feedback feedback;
      feedback.set_user_id(1);
      feedback.add_item_id(11);

      status = candb->updateFeedback(feedback);
      if (!status.ok())
        FAIL(status.toString());

      THEN("数据状态更新") {
        proto::UserInfo user_info;
        proto::UserQuery query;
        query.set_user_id(1);
        status = candb->queryUserInfo(query, user_info);
        if (!status.ok())
          FAIL(status.toString());

        REQUIRE(user_info.recommended_size() == 1);
      }
    }

    WHEN("添加数据获取推荐") {
      Subscribe subscribe;
      subscribe.set_user_id(1);
      subscribe.add_srp_id("test");

      status = candb->updateSubscribe(subscribe);
      if (!status.ok())
        FAIL(status.toString());

      Item item;
      item.set_item_id(1);
      item.set_publish_time(time(NULL));

      status = candb->addItem(item);
      if (!status.ok())
        FAIL(status.toString());

      item.set_item_id(2);
      item.set_publish_time(time(NULL));

      status = candb->addItem(item);
      if (!status.ok())
        FAIL(status.toString());

      Feedback feedback;
      feedback.set_user_id(1);
      feedback.add_item_id(1);

      status = candb->updateFeedback(feedback);
      if (!status.ok())
        FAIL(status.toString());

      THEN("数据状态更新") {
        Recommend recmd;
        CandidateSet candset;

        recmd.set_user_id(1);
        recmd.set_beg_time(time(NULL) - 86400);
        recmd.set_end_time(time(NULL));
        status = candb->queryCandidateSet(recmd, candset);
        if (!status.ok())
          FAIL(status.toString());

      }
    }
    delete candb;
    remove("./wal-item.writing");
    remove("./wal-user.writing");
  }
}

TEST_CASE("CandidateDB操作逻辑测试", "[base]") {
  Options opts;
  CandidateDB* candb;
  Status status = CandidateDB::openDB(opts, &candb);
  if (!status.ok())
    FAIL(status.toString()); 

  SECTION("findUser") {
    User user;
    user.set_user_id(1);
    Status status = candb->findUser(user);
    if (!status.isNotFound())
      FAIL(status.toString());

    user.set_user_id(2);
    status = candb->findUser(user);
    if (!status.isNotFound())
      FAIL(status.toString());

    user.set_user_id(3);
    status = candb->findUser(user);
    if (!status.isNotFound())
      FAIL(status.toString());

    Subscribe subscribe;
    subscribe.set_user_id(1);
    status = candb->updateSubscribe(subscribe);
    if (!status.ok())
      FAIL(status.toString());

    Action action;
    action.set_user_id(2);
    action.set_item_id(1);
    status = candb->updateAction(action, action);
    if (!status.isNotFound()) // item没有添加
      FAIL(status.toString());

    Recommend query;
    query.set_user_id(3);
    CandidateSet candset;
    status = candb->queryCandidateSet(query, candset);
    if (!status.ok())
      FAIL(status.toString());

    user.set_user_id(1);
    status = candb->findUser(user);
    if (!status.ok())
      FAIL(status.toString());

    user.set_user_id(2);
    status = candb->findUser(user);
    if (!status.ok())
      FAIL(status.toString());

    user.set_user_id(3);
    status = candb->findUser(user);
    if (!status.isNotFound())
      FAIL(status.toString());
  }

  SECTION("addItem") {
    Item item;
    item.set_item_id(1);
    item.set_publish_time(time(NULL));
    Status status = candb->addItem(item);
    if (!status.ok())
      FAIL(status.toString());

    item.set_item_id(2);
    item.set_publish_time(time(NULL) - opts.item_hold_time);
    status = candb->addItem(item);
    if (!status.isInvalidData())
      FAIL(status.toString());

    item.set_item_id(3);
    item.set_publish_time(time(NULL) + 3701);
    status = candb->addItem(item);
    if (!status.isInvalidData())
      FAIL(status.toString());
  }
  SECTION("updateSubscribe") {
    Subscribe subscribe;
    subscribe.set_user_id(10);
    status = candb->updateSubscribe(subscribe);
    if (!status.ok())
      FAIL(status.toString());
    User user;
    user.set_user_id(10);
    status = candb->findUser(user);
    if (!status.ok())
      FAIL(status.toString());
  }
  SECTION("updateAction") {
    Item item;
    item.set_item_id(21);
    item.set_publish_time(time(NULL));
    Status status = candb->addItem(item);
    if (!status.ok())
      FAIL(status.toString());

    Action action;
    action.set_user_id(20);
    action.set_item_id(21);
    action.set_action(ACTION_TYPE_CLICK);
    status = candb->updateAction(action, action);
    if (!status.ok())
      FAIL(status.toString());
    REQUIRE(action.history_id_size() == 0);

    item.set_item_id(22);
    item.set_publish_time(time(NULL));
    status = candb->addItem(item);
    if (!status.ok())
      FAIL(status.toString());

    action.Clear();
    action.set_user_id(20);
    action.set_item_id(22);
    action.set_action(ACTION_TYPE_CLICK);
    status = candb->updateAction(action, action);
    if (!status.ok())
      FAIL(status.toString());
    REQUIRE(action.history_id_size() == 1);
  }
  SECTION("queryCandidateSet") {
    int32_t ctime = time(NULL);
    Item item;
    item.set_item_id(31);
    item.set_publish_time(ctime - 3);
    item.set_item_type(ITEM_TYPE_NEWS);
    //item.add_zone("河北:石家庄");
    Status status = candb->addItem(item);
    if (!status.ok())
      FAIL(status.toString());

    item.set_item_id(32);
    item.set_publish_time(ctime - 2);
    item.set_item_type(ITEM_TYPE_VIDEO);
    item.clear_zone();
    //item.add_zone("河北:邢台");
    status = candb->addItem(item);
    if (!status.ok())
      FAIL(status.toString());

    item.set_item_id(33);
    item.set_publish_time(ctime - 1);
    item.set_item_type(ITEM_TYPE_NEWS);
    item.clear_zone();
    //item.add_zone("北京:北京");
    status = candb->addItem(item);
    if (!status.ok())
      FAIL(status.toString());

    Recommend recmd;
    CandidateSet candset;

    recmd.set_user_id(1);
    recmd.set_network(RECOMMEND_NETWORK_WIFI);
    status = candb->queryCandidateSet(recmd, candset);
    if (!status.ok())
      FAIL(status.toString());
    REQUIRE(candset.base().item_id_size() == 3);
    REQUIRE(candset.base().history_id_size() == 0);

    candset.Clear();
    recmd.set_user_id(1);
    recmd.set_network(RECOMMEND_NETWORK_MOBILE);
    status = candb->queryCandidateSet(recmd, candset);
    if (!status.ok())
      FAIL(status.toString());
    REQUIRE(candset.base().item_id_size() == 2);
    REQUIRE(candset.base().history_id_size() == 0);

    candset.Clear();
    recmd.set_user_id(1);
    recmd.set_network(RECOMMEND_NETWORK_WIFI);
    recmd.set_zone("河北:石家庄");
    status = candb->queryCandidateSet(recmd, candset);
    if (!status.ok())
      FAIL(status.toString());
    REQUIRE(candset.base().item_id_size() == 3);
    REQUIRE(candset.base().history_id_size() == 0);

    item.set_item_id(34);
    item.set_publish_time(ctime);
    item.set_item_type(ITEM_TYPE_NEWS);
    item.clear_zone();
    item.add_zone("北京:北京");
    item.mutable_top_info()->set_top_type(TOP_TYPE_GLOBAL);
    status = candb->addItem(item);
    if (!status.ok())
      FAIL(status.toString());

    item.set_item_id(35);
    item.set_publish_time(ctime);
    item.set_item_type(ITEM_TYPE_NEWS);
    item.clear_zone();
    item.add_zone("北京:北京");
    item.mutable_top_info()->set_top_type(TOP_TYPE_PARTIAL);
    item.mutable_top_info()->add_top_srp_id("1111");
    status = candb->addItem(item);
    if (!status.ok())
      FAIL(status.toString());

    recmd.Clear();
    candset.Clear();
    recmd.set_user_id(1);
    recmd.set_network(RECOMMEND_NETWORK_WIFI);
    status = candb->queryCandidateSet(recmd, candset);
    if (!status.ok())
      FAIL(status.toString());
    REQUIRE(candset.base().item_id_size() == 4);
    REQUIRE(candset.base().history_id_size() == 0);

    Subscribe subscribe;
    subscribe.set_user_id(1);
    subscribe.add_srp_id("1111");
    status = candb->updateSubscribe(subscribe);
    if (!status.ok())
      FAIL(status.toString());

    recmd.Clear();
    candset.Clear();
    recmd.set_user_id(1);
    recmd.set_network(RECOMMEND_NETWORK_WIFI);
    status = candb->queryCandidateSet(recmd, candset);
    if (!status.ok())
      FAIL(status.toString());
    REQUIRE(candset.base().item_id_size() == 5);
    REQUIRE(candset.base().history_id_size() == 0);

    Feedback feedback;
    feedback.set_user_id(1);
    feedback.add_item_id(31);
    feedback.add_item_id(32);
    status = candb->updateFeedback(feedback);
    if (!status.ok())
      FAIL(status.toString());

    recmd.Clear();
    candset.Clear();
    recmd.set_user_id(1);
    recmd.set_network(RECOMMEND_NETWORK_WIFI);
    status = candb->queryCandidateSet(recmd, candset);
    if (!status.ok())
      FAIL(status.toString());
    REQUIRE(candset.base().item_id_size() == 3);
    REQUIRE(candset.base().history_id_size() == 0);
  }
  delete candb;
  remove("./wal-item.writing");
  remove("./wal-user.writing");
}

