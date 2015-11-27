#include "core/candidate_db.h"
#include "core/core_type.h"
#include "glog/logging.h"

namespace rsys {
  namespace news {
    static const fver_t kSingletonVer(1,0);
    static const std::string kSingletonName = "/._candb_lock_";

    Status CandidateDB::openDB(const Options& opts, CandidateDB** dbptr)
    {
      CandidateDB* c_dbptr = new CandidateDB(opts);
      
      Status status = c_dbptr->lock();
      if (!status.ok()) {
        delete c_dbptr; 
        return status;
      }
      *dbptr = c_dbptr;

      return (*dbptr)->reload();
    }

    CandidateDB::CandidateDB(const Options& opts)
      : singleton_(opts.work_path + kSingletonName), user_table_(NULL), item_table_(NULL)
    {
      options_ = opts;

      user_table_ = new UserTable(opts);
      item_table_ = new ItemTable(opts);
    }

    CandidateDB::~CandidateDB() 
    {
      singleton_.close();
      if (user_table_)
        delete user_table_;
      if (item_table_)
        delete item_table_;
    }

 // 单进程锁定
    Status CandidateDB::lock()
    {
      Status status = singleton_.create();
      if (!status.ok()) {
        return status;
      }

      status = singleton_.lockfile();
      if (!status.ok()) {
        return status;
      }

      return Status::OK();
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
        return status;
      }

      status = item_table_->eliminate();
      if (!status.ok()) {
        LOG(WARNING)<<status.toString();
      }
      status = item_table_->flushTable();
      if (!status.ok()) {
        return status;
      }

      return Status::OK();
    }

    Status CandidateDB::reload()
    {
      Status status = user_table_->loadTable();
      if (!status.ok()) {
        return status;
      }

      status = item_table_->loadTable();
      if (!status.ok()) {
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
      if (!status.ok()) {
        return status;
      } 
      return item_table_->updateAction(action);
    }

    Status CandidateDB::queryCandidateSet(const Recommend& recmd, CandidateSet& cset)
    {
      query_t query;
      candidate_set_t candidate_set;

      glue::structed_query(recmd, query);
      // 修正时间
      if (recmd.beg_time() <= 0) {
        query.end_time = time(NULL);
        query.start_time = query.end_time - options_.interval_recommendation;
      } else {
        query.end_time = recmd.beg_time();
        query.start_time = query.end_time - options_.item_hold_time;
      }
      Status status = item_table_->queryCandidateSet(query, candidate_set);
      if (!status.ok()) {
        return status;
      }

      status = user_table_->filterCandidateSet(recmd.user_id(), candidate_set);
      if (!status.ok() && !status.isNotFound()) {
        return status;
      }
      id_set_t history_set;

      status = user_table_->queryHistory(recmd.user_id(), history_set);
      if (!status.ok() && !status.isNotFound()) {
        return status;
      }

      for (id_set_t::iterator iter = history_set.begin(); 
          iter != history_set.end(); ++iter) {
        cset.mutable_base()->add_history_id(*iter);
      }
      cset.mutable_base()->set_user_id(recmd.user_id());

      candidate_set_t::iterator iter = candidate_set.begin();
      for (; iter != candidate_set.end(); ++iter) {
        cset.mutable_base()->add_item_id(iter->item_id);
        cset.mutable_payload()->add_power(iter->power);
        cset.mutable_payload()->add_publish_time(iter->publish_time);
        cset.mutable_payload()->add_type(iter->item_type);
        cset.mutable_payload()->add_picture_num(iter->picture_num);
        cset.mutable_payload()->add_category_id(iter->category_id);
      }

      return Status::OK();
    }

    // 查询用户是否在用户表中
    Status CandidateDB::queryUserInfo(const proto::UserQuery& query, proto::UserInfo& user_info)
    {
      user_info_t user;

      Status status = user_table_->queryUser(query.user_id(), user);
      if (!status.ok()) {
        return status;
      }
      user_info.set_user_id(query.user_id());
      glue::proto_user_info(user, user_info);

      return Status::OK();
    }

    // 查询用户是否在用户表中
    Status CandidateDB::queryItemInfo(const proto::ItemQuery& query, proto::ItemInfo& user_info)
    {
      item_info_t item;
      
      Status status = item_table_->queryItem(query.item_id(), item);
      if (!status.ok()) {
        return status;
      }
      glue::proto_item_info(item, user_info);

      return Status::OK();
    }
  } // namespace news
} // namespace rsys

