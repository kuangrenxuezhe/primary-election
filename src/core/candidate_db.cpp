#include "core/candidate_db.h"
#include <sys/time.h>
#include "utils/util.h"
#include "utils/duration.h"
#include "core/core_type.h"
#include "glog/logging.h"

namespace souyue {
  namespace recmd {
    static const fver_t kSingletonVer(1,0);
    static const std::string kSingletonName = "/._candb_lock_";

    class DurationLogger: public AutoDuration {
      public:
        template<typename... Args>
          DurationLogger(Duration::Unit unit, Args... args)
          : AutoDuration(unit, args...) {
          }

        virtual ~DurationLogger() {
          if (duration().count() < 100) {
            LOG(INFO) << info() << ", used: " << duration().count() << "ms";
          } else {
            LOG(WARNING) << info() << ", used: " << duration().count() << "ms";
          }
        }
    };
    Status CandidateDB::openDB(const ModelOptions& opts, CandidateDB** dbptr)
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

    CandidateDB::CandidateDB(const ModelOptions& opts)
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

    Status CandidateDB::queryCandidateSet(const Recommend& recmd, CandidateSet& candidate_set)
    {
      query_t query;

      glue::structed_query(recmd, query);
      // 修正时间
      if (recmd.beg_time() <= 0) {
        query.end_time = time(NULL);
      } else {
        query.end_time = recmd.beg_time();
      }
      query.start_time = query.end_time - options_.interval_recommendation;
 
      if (options_.service_type != 0)
        return querySubscriptionCandidateSet(recmd, query, candidate_set);

      return queryRecommendationCandidateSet(recmd, query, candidate_set);
    }

    Status CandidateDB::querySubscriptionCandidateSet(const Recommend& recmd, query_t& query, CandidateSet& cset)
    {
      DurationLogger duration(Duration::kMilliSeconds, "SubscriptionCandidateSet: user_id=", recmd.user_id());
      Status status = Status::OK();
      candidate_set_t candidate_set;

      query.region_id = kInvalidRegionID;
      // 获取普通新闻数据
      query.item_type = kNormalItem;
      status = item_table_->queryCandidateSet(query, candidate_set);
      if (!status.ok()) {
        return status;
      }
      if (candidate_set.size() > 0) {
        status = user_table_->filterCandidateSet(recmd.user_id(), candidate_set);
        if (!status.ok() && !status.isNotFound()) {
          return status;
        }
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

      int total = (int)candidate_set.size();
      // 修正返回结果总个数
      if (recmd.recommend_num() > 0 && total > recmd.recommend_num()) {
        total = recmd.recommend_num();
      }
      if (total > options_.max_candidate_set_size) {
        total = options_.max_candidate_set_size;
      }

      cset.mutable_base()->mutable_item_id()->Reserve(total);
      cset.mutable_payload()->mutable_power()->Reserve(total);
      cset.mutable_payload()->mutable_publish_time()->Reserve(total);
      cset.mutable_payload()->mutable_category_id()->Reserve(total);
      cset.mutable_payload()->mutable_picture_num()->Reserve(total);
      cset.mutable_payload()->mutable_type()->Reserve(total);

      float total_weight = 0.0f;
      candidate_set_t::iterator iter = candidate_set.begin();
      for (int i = 0; i < total && iter != candidate_set.end(); ++iter) {
        // 过滤掉用户非订阅数据
        if (iter->candidate_type == kTopCandidate || 
            iter->candidate_type == kPartialTopCandidate ||
            iter->candidate_type == kSubscribeCandidate) {
          i++;
          glue::copy_to_proto(*iter, cset);
          glue::remedy_candidate_weight(*iter, cset);

          if (cset.payload().power(cset.payload().power_size() - 1) != -100000)
            total_weight += cset.payload().power(cset.payload().power_size() - 1);
        }
     }

      // 候选集权重归一
      if (total_weight <= 0) total_weight = 1.0f;
      for (int i = 0; i < cset.payload().power_size(); ++i) {
        if (cset.payload().power(i) == -100000)
          continue;
        cset.mutable_payload()->set_power(i, cset.payload().power(i)/total_weight);
      }
      duration.appendInfo(", candidate set=", total);

      return Status::OK();
    }

    Status CandidateDB::queryRecommendationCandidateSet(const Recommend& recmd, query_t& query, CandidateSet& cset)
    {
      DurationLogger duration(Duration::kMilliSeconds, "RecommendationCandidateSet: user_id=", recmd.user_id());
      uint64_t region_id[2];
      Status status = Status::OK();
      candidate_set_t candidate_set, candidate_video_set, candidate_region_set;

      query.region_id = kInvalidRegionID;
      // 获取普通新闻数据
      query.item_type = kNormalItem;
      status = item_table_->queryCandidateSet(query, candidate_set);
      if (!status.ok()) {
        return status;
      }
      if (candidate_set.size() > 0) {
        status = user_table_->filterCandidateSet(recmd.user_id(), candidate_set);
        if (!status.ok() && !status.isNotFound()) {
          return status;
        }
      }

      if (recmd.network() == RECOMMEND_NETWORK_WIFI) {
        // 获取视频数据 
        query.item_type = kVideoItem;
        item_table_->queryCandidateSet(query, candidate_video_set);
        if (candidate_video_set.size() > 0)
          user_table_->filterCandidateSet(recmd.user_id(), candidate_video_set);
      }

      if (recmd.zone().length() > 0) {
        int region_num = glue::zone_to_region_id(recmd.zone().c_str(), region_id);
        if (region_num > 0) {
          query.item_type = kRegionItem;
          // 获取地域数据
          query.region_id = region_id[0];
          item_table_->queryCandidateSet(query, candidate_region_set);
          // 对于直辖市只处理城市，因为省份和城市是相同的
          if (region_num > 1 && candidate_region_set.size() <= 0 && region_id[0] != region_id[1])  {
            // 若城市不存在结果则回退到省份
            query.region_id = region_id[1];
            item_table_->queryCandidateSet(query, candidate_region_set);
          }
          if (candidate_region_set.size() > 0)
            user_table_->filterCandidateSet(recmd.user_id(), candidate_region_set);
        }
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

      // 撮合普通，视频和地域数据
      int total = options_.max_candidate_set_size;

      cset.mutable_base()->mutable_item_id()->Reserve(total);
      cset.mutable_payload()->mutable_power()->Reserve(total);
      cset.mutable_payload()->mutable_publish_time()->Reserve(total);
      cset.mutable_payload()->mutable_category_id()->Reserve(total);
      cset.mutable_payload()->mutable_picture_num()->Reserve(total);
      cset.mutable_payload()->mutable_type()->Reserve(total);

      int total_video = candidate_video_set.size();
      if (recmd.recommend_num() > 0 && total_video > recmd.recommend_num()) {
        total_video = recmd.recommend_num();
      }
      if (total_video > options_.max_candidate_video_size)
        total_video = options_.max_candidate_video_size;

      float total_weight = 0.0f;
      candidate_set_t::iterator iter = candidate_video_set.begin();
      for (int i = 0; i < total_video && iter != candidate_video_set.end(); ++iter, ++i) {
        glue::copy_to_proto(*iter, cset);
        glue::remedy_candidate_weight(*iter, cset);
        if (cset.payload().power(cset.payload().power_size() - 1) != -100000)
          total_weight += cset.payload().power(cset.payload().power_size() - 1);
      }

      int total_region = candidate_region_set.size();
      if (recmd.recommend_num() > 0 && total_region > recmd.recommend_num()) {
        total_region = recmd.recommend_num();
      }
      if (total_region > options_.max_candidate_region_size)
        total_region = options_.max_candidate_region_size;

      iter = candidate_region_set.begin();
      for (int i = 0; i < total_region && iter != candidate_region_set.end(); ++iter, ++i) {
        glue::copy_to_proto(*iter, cset);
        glue::remedy_candidate_weight(*iter, cset);
        if (cset.payload().power(cset.payload().power_size() - 1) != -100000)
          total_weight += cset.payload().power(cset.payload().power_size() - 1);
      }

      int total_normal = total - total_region - total_video;
      if (recmd.recommend_num() > 0 && total_normal > recmd.recommend_num()) {
        total_normal = recmd.recommend_num();
      }
      if (candidate_set.size() < total_normal)
        total_normal = candidate_set.size();

      iter = candidate_set.begin();
      for (int i = 0; i < total_normal && iter != candidate_set.end(); ++iter, ++i) {
        glue::copy_to_proto(*iter, cset);
        glue::remedy_candidate_weight(*iter, cset);

        if (cset.payload().power(cset.payload().power_size() - 1) != -100000)
          total_weight += cset.payload().power(cset.payload().power_size() - 1);
      }

      // 候选集权重归一
      if (total_weight <= 0) total_weight = 1.0f;
      for (int i = 0; i < cset.payload().power_size(); ++i) {
        if (cset.payload().power(i) == -100000)
          continue;
        cset.mutable_payload()->set_power(i, cset.payload().power(i)/total_weight);
      }
      duration.appendInfo(", candidate set normal=", total_normal, ", video=", total_video, ", region=", total_region);

      return Status::OK();
    }

    // 查询用户是否在用户表中
    Status CandidateDB::queryUserInfo(const UserQuery& query, UserInfo& user_info)
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
    Status CandidateDB::queryItemInfo(const ItemQuery& query, ItemInfo& user_info)
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
  } // namespace recmd
} // namespace souyue

