#include "core/candidate_db.h"

#include "core/core_type.h"
#include "glog/logging.h"
#include "proto/record.pb.h"

namespace rsys {
  namespace news {
    // 默认推荐时间范围
    static const int32_t kDefaultRecommendGap = 1*24*60*60;
    static const fver_t kWALVersion(0, 1);

    const char kCategoryName[][16] = {
      "\xbb\xa5\xc1\xaa\xcd\xf8\xd0\xc2\xce\xc5", // 互联网新闻 GBK编码
      "\xb9\xfa\xc4\xda\xd0\xc2\xce\xc5", // 国内新闻 GBK编码
      "\xb9\xfa\xbc\xca\xd0\xc2\xce\xc5", // 国际新闻 GBK编码
      "\xc9\xe7\xbb\xe1\xd0\xc2\xce\xc5", // 社会新闻 GBK编码
      "\xd3\xe9\xc0\xd6\xd0\xc2\xce\xc5", // 娱乐新闻 GBK编码
      "\xbe\xfc\xca\xc2\xd0\xc2\xce\xc5", // 军事新闻 GBK编码
      "\xcc\xe5\xd3\xfd\xd0\xc2\xce\xc5", // 体育新闻 GBK编码
      "\xc6\xfb\xb3\xb5\xd0\xc2\xce\xc5", // 汽车新闻 GBK编码
      "\xbf\xc6\xbc\xbc\xd0\xc2\xce\xc5", // 科技新闻 GBK编码
      "\xb2\xc6\xbe\xad\xd0\xc2\xce\xc5", // 财经新闻 GBK编码
      "\xbd\xcc\xd3\xfd\xd0\xc2\xce\xc5", // 教育新闻 GBK编码
      "\xb7\xbf\xb2\xfa\xd0\xc2\xce\xc5", // 房产新闻 GBK编码
      "\xc5\xae\xd0\xd4\xd0\xc2\xce\xc5", // 女性新闻 GBK编码
    };

    class CandidateAheadLog: public AheadLog {
      public:
        CandidateAheadLog(const std::string& path, const fver_t& fver)
          : AheadLog(path, fver) {
        }
        virtual ~CandidateAheadLog() {
        }

      public:
        // ahead log滚存触发器
        virtual bool trigger() {
          return true;
        }
        // 处理数据回滚
        virtual bool rollback(const std::string& data) {
          return true;
        }
    };

    Status CandidateDB::openDB(const Options& opts, CandidateDB** dbptr)
    {
      *dbptr = new CandidateDB(opts);
      return (*dbptr)->reload();
    }

    CandidateDB::CandidateDB(const Options& opts)
      : ahead_log_(NULL), user_table_(NULL), item_table_(NULL)
    {
      options_ = opts;

      user_table_ = new UserTable(opts);
      item_table_ = new ItemTable(opts);

      ahead_log_ = new CandidateAheadLog(opts.work_path, kWALVersion);
    }

    CandidateDB::~CandidateDB() 
    {
      delete ahead_log_;
      delete user_table_;
      delete item_table_;
    }

    Status CandidateDB::rollover(int32_t expired) 
    {
      return ahead_log_->apply(expired);
    }

    // 可异步方式，线程安全
    Status CandidateDB::flush()
    {
      Status status = user_table_->eliminate(options_.user_hold_time);
      if (!status.ok()) {
        LOG(WARNING) << status.toString();
      }
      status = user_table_->flushTable();
      if (!status.ok()) {
        LOG(ERROR) << status.toString();
        return status;
      }

      status = item_table_->eliminate(options_.item_hold_time);
      if (!status.ok()) {
        LOG(WARNING) << status.toString();
      }
      status = item_table_->flushTable();
      if (!status.ok()) {
        LOG(ERROR) << status.toString();
        return status;
      }

      return Status::OK();
    }

    Status CandidateDB::reload()
    {
      Status status = user_table_->loadTable();
      if (!status.ok()) {
        LOG(ERROR) << status.toString();
        return status;
      }

      status = item_table_->loadTable();
      if (!status.ok()) {
        LOG(ERROR) << status.toString();
        return status;
      }

      return ahead_log_->recovery();
    }

    bool CandidateDB::findUser(uint64_t user_id)
    {
      return user_table_->findUser(user_id);
    }

    Status CandidateDB::addItem(const ItemInfo& item)
    {
      LogRecord log_record;
      std::string serialize_str;

      // write ahead-log
      log_record.mutable_record()->PackFrom(item);
      serialize_str = log_record.SerializeAsString();

      Status status = ahead_log_->write(serialize_str);
      if (!status.ok()) {
        LOG(WARNING) << status.toString() << std::hex << ", item_id=" << item.item_id();
      }

      // convert ItemInfo to item_info_t
      item_info_t* item_info = new item_info_t;
      item_info->item_id = item.item_id();
      item_info->publish_time = item.publish_time();
      item_info->primary_power = item.power();
      item_info->item_type = item.item_type();
      
      // 匹配分类
      for (int i = 0; i < item.category_size(); ++i) {
        for (size_t j = 0; j < sizeof(kCategoryName)/sizeof(char[16]); ++j) {
          if (strcasecmp(item.category(i).c_str(), kCategoryName[j]))
            continue;
          item_info->category_id = j;
          break;
        }
      }
      item_info->region_id = 0; //TODO: 下一个版本补入

      for (int i = 0; i < item.srp_size(); ++i) {
        const Srp& srp = item.srp(i);
        item_info->circle_and_srp.insert(srp.srp_id());
      }

      return item_table_->addItem(item_info);
    }

    Status CandidateDB::queryHistory(uint64_t user_id, IdSet& history)
    {
      id_set_t id_set;
      Status status = user_table_->queryHistory(user_id, id_set);
      if (status.ok()) {
        id_set_t::iterator iter = id_set.begin();
        for (; iter != id_set.end(); ++iter) {
          history.add_id(*iter);
        }
      }
      return status;
    }

    Status CandidateDB::queryCandidateSet(const CandidateQuery& query, CandidateSet& cset)
    {
      query_t cand_query;
      candidate_set_t cand_set;   

      if (query.end_time() >= query.start_time()) {
        cand_query.end_time = time(NULL);
        cand_query.start_time = cand_query.end_time - kDefaultRecommendGap;
      } else {
        cand_query.start_time = query.start_time();
        cand_query.end_time = query.end_time();
      }

      Status status = item_table_->queryCandidateSet(cand_query, cand_set);
      if (!status.ok()) {
        LOG(ERROR) << "query candidate set failed: " << status.toString()
          << ", begin time=" << cand_query.start_time << ", end time=" << cand_query.end_time;
        return status;
      }

      status = user_table_->filterCandidateSet(query.user_id(), cand_set);
      if (!status.ok()) {
        LOG(ERROR) << "filter candidate set failed: " << status.toString()
          << " user id=" << query.user_id();
        return status;
      }

      id_set_t id_set;
      status = user_table_->queryHistory(query.user_id(), id_set);
      if (!status.ok()) {
        LOG(ERROR) << "query history failed: " << status.toString()
          << " user id=" << query.user_id();
        return status;
      }

      for (candidate_set_t::iterator iter = cand_set.begin(); 
          iter != cand_set.end(); ++iter) {
        CandidateSet_Candidate* cand = cset.add_candidate();

        cand->set_item_id(iter->item_id);
        cand->set_power(iter->power);
        cand->set_publish_time(iter->publish_time);
        cand->set_category_id(iter->category_id);
        cand->set_picture_num(iter->picture_num);
      }

      for (id_set_t::iterator iter = id_set.begin(); iter != id_set.end(); ++iter) {
        cset.add_history(*iter);
      }

      return Status::OK();
    }

    Status CandidateDB::updateAction(const UserAction& action)
    {
      LogRecord log_record;
      std::string serialize_str;

      log_record.mutable_record()->PackFrom(action);
      serialize_str = log_record.SerializeAsString();

      Status status = ahead_log_->write(serialize_str);
      if (!status.ok()) {
        LOG(WARNING) << "write action log failed: " << status.toString()
          << std::hex << ", user id=" << action.user_id() << ", item id=" << action.item_id();
      }
      action_t item_action;

      item_action.item_id = action.item_id();
      item_action.action = action.action();
      item_action.action_time = action.click_time();
      status = item_table_->updateAction(item_action);
      if (!status.ok()) {
        LOG(WARNING) << "add action failed: " << status.toString()
          << std::hex << ", user id=" << action.user_id() << ", item id=" << action.item_id();
        return status;
      }
      action_t user_action;

      user_action.item_id = action.item_id();
      user_action.action = action.action();
      user_action.action_time = action.click_time();
      status = user_table_->updateReaded(action.user_id(), user_action);
      if (!status.ok()) {
        LOG(WARNING) << "add action failed: " << status.toString()
          << std::hex << ", user id=" << action.user_id() << ", item id=" << action.item_id();
      }

      return status;
    }

    Status CandidateDB::updateUser(const UserSubscribe& subscribe)
    {
      LogRecord log_record;
      std::string serialize_str;

      log_record.mutable_record()->PackFrom(subscribe);
      serialize_str = log_record.SerializeAsString();

      Status status = ahead_log_->write(serialize_str);
      if (!status.ok()) {
        LOG(WARNING) << "write subscribe log failed: " << status.toString()
          << std::hex << ", user id=" << subscribe.user_id();
      }
      user_info_t* user_info = new user_info_t;

      for (int i = 0; i < subscribe.srp_id_size(); ++i) {
        user_info->srp.insert(subscribe.srp_id(i));
      }

      for (int i = 0; i < subscribe.circle_id_size(); ++i) {
        user_info->circle.insert(subscribe.circle_id(i));
      }
      return user_table_->updateUser(subscribe.user_id(), user_info);
    }

    Status CandidateDB::updateCandidateSet(uint64_t user_id, const IdSet& recommend_set)
    {
      LogRecord log_record;
      std::string serialize_str;

      log_record.mutable_record()->PackFrom(recommend_set);
      serialize_str = log_record.SerializeAsString();

      Status status = ahead_log_->write(serialize_str);
      if (!status.ok()) {
        LOG(WARNING) << "write recommend set log failed: " << status.toString()
          << std::hex << ", user id=" << user_id;
      }
      id_set_t id_set;

      for (int i = 0; i < recommend_set.id_size(); ++i) {
        id_set.insert(recommend_set.id(i));
      }
      return user_table_->updateCandidateSet(user_id, id_set);
    }
  } // namespace news
} // namespace rsys

