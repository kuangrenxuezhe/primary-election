#include "core/candidate_db.h"

#include "core/core_type.h"
#include "glog/logging.h"
#include "proto/record.pb.h"

namespace rsys {
  namespace news {
    Status CandidateDB::openDB(const Options& opts, CandidateDB** dbptr)
    {
      *dbptr = new CandidateDB(opts);
      return (*dbptr)->reload();
    }

    CandidateDB::CandidateDB(const Options& opts)
      : user_table_(NULL), item_table_(NULL)
    {
      options_ = opts;

      user_table_ = new UserTable(opts);
      item_table_ = new ItemTable(opts);
    }

    CandidateDB::~CandidateDB() 
    {
      if (user_table_)
        delete user_table_;
      if (item_table_)
        delete item_table_;
    }

    // 可异步方式，线程安全
    Status CandidateDB::flush()
    {
      Status status = user_table_->eliminate();
      if (!status.ok()) {
        LOG(WARNING)<<status.toString();
      }
      status = user_table_->flushTable();
      if (!status.ok()) {
        LOG(ERROR)<<status.toString();
        return status;
      }

      status = item_table_->eliminate();
      if (!status.ok()) {
        LOG(WARNING)<<status.toString();
      }
      status = item_table_->flushTable();
      if (!status.ok()) {
        LOG(ERROR)<<status.toString();
        return status;
      }

      return Status::OK();
    }

    Status CandidateDB::reload()
    {
      Status status = user_table_->loadTable();
      if (!status.ok()) {
        LOG(ERROR)<<status.toString();
        return status;
      }

      status = item_table_->loadTable();
      if (!status.ok()) {
        LOG(ERROR)<<status.toString();
        return status;
      }

      return Status::OK();
    }

    Status CandidateDB::findUser(const User& user)
    {
      if (user_table_->findUser(user.user_id())) {
        return Status::OK();
      }
      std::ostringstream oss;

      oss<<"user_id="<<std::hex<<user.user_id();
      return Status::NotFound(oss.str());
    }

    Status CandidateDB::addItem(const Item& item)
    {
      return item_table_->addItem(item);
    }

    Status CandidateDB::updateSubscribe(const Subscribe& subscribe)
    {
      return user_table_->updateSubscribe(subscribe);
    }

    Status CandidateDB::updateFeedback(const Feedback& feedback)
    {
      return user_table_->updateFeedback(feedback);
    }

    Status CandidateDB::updateAction(const Action& action, Action& updated)
    {
      Status status = user_table_->updateAction(action, updated);
      if (status.ok()) {
        return status;
      } 
      return item_table_->updateAction(action);
    }

    Status CandidateDB::queryCandidateSet(const Recommend& query, CandidateSet& cset)
    {
      query_t table_query;
      candidate_set_t candidate_set;

      table_query.request_num = query.request_num();
      table_query.start_time = query.beg_time();
      table_query.end_time = query.end_time();
      table_query.network = query.network();

      Status status = item_table_->queryCandidateSet(table_query, candidate_set);
      if (!status.ok()) {
        return status;
      }

      status = user_table_->filterCandidateSet(query.user_id(), candidate_set);
      if (!status.ok()) {
        return status;
      }
      id_set_t history_set;

      status = user_table_->queryHistory(query.user_id(), history_set);
      if (!status.ok()) {
        return status;
      }

      for (id_set_t::iterator iter = history_set.begin(); 
          iter != history_set.end(); ++iter) {
        cset.mutable_base()->add_history_id(*iter);
      }
      cset.mutable_base()->set_user_id(query.user_id());

      candidate_set_t::iterator iter = candidate_set.begin();
      for (; iter != candidate_set.end(); ++iter) {
        cset.mutable_base()->add_item_id(iter->item_id);
        cset.mutable_payload()->add_power(iter->power);
        cset.mutable_payload()->add_publish_time(iter->publish_time);
        //cset.mutable_payload()->add_type(iter->
        cset.mutable_payload()->add_picture_num(iter->picture_num);
        cset.mutable_payload()->add_category_id(iter->category_id);
      }

      return Status::OK();
    }

    // 查询用户是否在用户表中
    Status CandidateDB::queryUserInfo(const UserQuery& query, UserInfo& user_info)
    {
      user_info_t user;

      Status status = user_table_->queryUser(user_info.user_id(), &user);
      if (!status.ok()) {
        return status;
      }

      user_info.set_ctime(user.ctime);
      for (subscribe_t::iterator iter = user.subscribe.begin(); 
          iter != user.subscribe.end(); ++iter) {
        KeyStr* pair = user_info.add_subscribe();

        pair->set_key(iter->first);
        pair->set_str(iter->second);
      }
      for (map_str_t::iterator iter = user.dislike.begin();
          iter != user.dislike.end(); ++iter) {
        KeyStr* pair = user_info.add_dislike();

        pair->set_key(iter->first);
        pair->set_str(iter->second);
      }
      for (map_time_t::iterator iter = user.readed.begin();
          iter != user.readed.end(); ++iter) {
        KeyTime* pair = user_info.add_readed();

        pair->set_key(iter->first);
        pair->set_ctime(iter->second);
      }
      for (map_str_t::iterator iter = user.recommended.begin();
          iter != user.recommended.end(); ++iter) {
        KeyTime* pair = user_info.add_recommended();

        pair->set_key(iter->first);
        pair->set_ctime(iter->second);
      }
      return Status::OK();
    }

    // 查询用户是否在用户表中
    Status CandidateDB::queryItemInfo(const ItemQuery& query, ItemInfo& user_info)
    {
      return item_table_->queryItemInfo(query, user_info);
    }
  } // namespace news
} // namespace rsys

