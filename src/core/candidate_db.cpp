#include "core/candidate_db.h"
#include <sys/time.h>
#include "util.h"
#include "duration.h"
#include "core/core_type.h"
#include "glog/logging.h"

namespace rsys {
  namespace news {
    static const fver_t kSingletonVer(1,0);
    static const std::string kSingletonName = "/._candb_lock_";

    class DurationLogger: public AutoDuration {
      public:
        template<typename... Args>
          DurationLogger(Duration::Unit unit, Args... args)
          : AutoDuration(unit, args...) {
          }

        virtual ~DurationLogger() {
          LOG(INFO) << info() << ", used: " << duration().count() << "ms";
        }
    };
    Status CandidateDB::openDB(const Options& opts, CandidateDB** dbptr)
    {
      DurationLogger duration(Duration::kMilliSeconds, "OpenDB");
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
      singleton_.unlock();
      singleton_.close();
      if (user_table_)
        delete user_table_;
      if (item_table_)
        delete item_table_;
      LOG(INFO) << "CloseDB";
    }

    // 单进程锁定
    Status CandidateDB::lock()
    {
      Status status = singleton_.create();
      if (!status.ok()) {
        return status;
      }

      status = singleton_.lock();
      if (!status.ok()) {
        return status;
      }

      return Status::OK();
    }

    // 可异步方式，线程安全
    Status CandidateDB::flush()
    {
      DurationLogger duration(Duration::kMilliSeconds, "Flush");
      Status status = user_table_->eliminate();
      if (!status.ok()) {
        LOG(WARNING)<<status.toString();
      }
      status = user_table_->flushTable();
      if (!status.ok()) {
        return status;
      }

      status = item_table_->flushTable();
      if (!status.ok()) {
        return status;
      }

      return Status::OK();
    }

    Status CandidateDB::reload()
    {
      DurationLogger duration(Duration::kMilliSeconds, "Reload");
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
      DurationLogger duration(Duration::kMilliSeconds, "FindUser: user_id=", user.user_id());

      if (user_table_->findUser(user.user_id())) {
        return Status::OK();
      }
      return Status::NotFound("user_id=", user.user_id());
    }

    Status CandidateDB::addItem(const Item& item)
    {
      DurationLogger duration(Duration::kMilliSeconds, "AddItem: item_id=", item.item_id());
      return item_table_->addItem(item);
    }

    Status CandidateDB::updateSubscribe(const Subscribe& subscribe)
    {
      DurationLogger duration(Duration::kMilliSeconds, "UpdateSubscribe: user_id=", subscribe.user_id());
      return user_table_->updateSubscribe(subscribe);
    }

    Status CandidateDB::updateFeedback(const Feedback& feedback)
    {
      DurationLogger duration(Duration::kMilliSeconds, "UpdateFeedback: user_id=", feedback.user_id());
      return user_table_->updateFeedback(feedback);
    }

    Status CandidateDB::updateAction(const Action& action, Action& updated)
    {
      DurationLogger duration(Duration::kMilliSeconds, "UpdateAction: user_id=", action.user_id(), ", item_id=", action.item_id());
      Status status = user_table_->updateAction(action, updated);
      if (!status.ok()) {
        return status;
      } 
      return item_table_->updateAction(action);
    }

    Status CandidateDB::queryCandidateSet(const Recommend& recmd, CandidateSet& cset)
    {
      DurationLogger duration(Duration::kMilliSeconds, "QueryCandidateSet: user_id=", recmd.user_id());
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
      uint64_t region_id[2];
      Status status = Status::OK();

      if (!glue::zone_to_region_id(recmd.zone().c_str(), region_id)) {
        query.region_id = kInvalidRegionID;
        status = item_table_->queryCandidateSet(query, candidate_set);
      } else {
        query.region_id = region_id[1];
        status = item_table_->queryCandidateSet(query, candidate_set);
        if (!status.ok()) {
          return status;
        }
        // 对于直辖市只处理城市，因为省份和城市是相同的
        if (candidate_set.size() <= 0 && region_id[0] != region_id[1])  {
          // 若城市不存在结果则回退到省份
          query.region_id = region_id[0];
          status = item_table_->queryCandidateSet(query, candidate_set);
        }
      }

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

      cset.mutable_base()->mutable_history_id()->Reserve(history_set.size());
      for (id_set_t::iterator iter = history_set.begin(); 
          iter != history_set.end(); ++iter) {
        cset.mutable_base()->add_history_id(*iter);
      }
      cset.mutable_base()->set_user_id(recmd.user_id());

      int total = candidate_set.size();
      if (options_.max_candidate_set_size < (int)candidate_set.size())
        total = options_.max_candidate_set_size;

      cset.mutable_base()->mutable_item_id()->Reserve(total);
      cset.mutable_payload()->mutable_power()->Reserve(total);
      cset.mutable_payload()->mutable_publish_time()->Reserve(total);
      cset.mutable_payload()->mutable_category_id()->Reserve(total);
      cset.mutable_payload()->mutable_picture_num()->Reserve(total);
      cset.mutable_payload()->mutable_type()->Reserve(total);
      candidate_set_t::iterator iter = candidate_set.begin();
      for (int i = 0; iter != candidate_set.end() && i < total; ++iter, ++i) {
        cset.mutable_base()->add_item_id(iter->item_id);
        cset.mutable_payload()->add_power(iter->power);
        cset.mutable_payload()->add_publish_time(iter->publish_time);
        cset.mutable_payload()->add_type((CandidateType)iter->item_type);
        cset.mutable_payload()->add_picture_num(iter->picture_num);
        cset.mutable_payload()->add_category_id(iter->category_id);
      }
      duration.appendInfo(", candidate_set_size=", total);

      return Status::OK();
    }

    // 查询用户是否在用户表中
    Status CandidateDB::queryUserInfo(const proto::UserQuery& query, proto::UserInfo& user_info)
    {
      DurationLogger duration(Duration::kMilliSeconds, "QueryUserInfo: user_id=", query.user_id());
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
      DurationLogger duration(Duration::kMilliSeconds, "QueryItemInfo: item_id=", query.item_id());
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

