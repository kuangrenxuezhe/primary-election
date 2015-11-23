#include "core/user_table.h"

#include <unistd.h>
#include <string.h>

#include "util/util.h"
#include "util/crc32c.h"
#include "glog/logging.h"

#include "proto/record.pb.h"
#include "proto/service.pb.h"

namespace rsys {
  namespace news {
    static const fver_t kUserFver(0, 1);
    static const std::string kUserAheadLog = "wal-user";
    static const std::string kUserTable = "table-user.dat";

    static const char kLogTypeAction    = 'A';
    static const char kLogTypeSubscribe = 'S';
    static const char kLogTypeFeedback  = 'F';

    class UserAheadLog: public AheadLog {
      public:
        UserAheadLog(UserTable* user_table, const std::string& path, 
            const fver_t& fver): AheadLog(path, kUserAheadLog, fver), user_table_(user_table) {
        }

      public:
        // ahead log滚存触发器
        virtual Status trigger() {
          if (user_table_->level_table_->deepen())
            return Status::OK();
          return Status::Corruption("Deepen user table");
        }

        // 处理数据回滚
        virtual Status rollback(const std::string& data) {
          // 数据大小至少一个字节，表示类型
          if (data.length() <= 1) {
            return Status::Corruption("Invalid user info data");
          }
          const char* c_data = data.c_str();

          if (kLogTypeAction == data[0]) {
            Action log_action;
            action_t action;

            if (!log_action.ParseFromArray(c_data + 1, data.length() - 1)) {
              return Status::Corruption("Parse user action");
            }
            glue::structed_action(log_action, action);

            Status status = user_table_->updateAction(log_action.user_id(), action);
            if (status.ok()) {
              return status;
            }
          } else if (kLogTypeSubscribe == data[0]) {
            Subscribe log_subscribe;
            map_str_t subscribe;

            if (!log_subscribe.ParseFromArray(c_data + 1, data.length() - 1)) {
              return Status::Corruption("Parse user subscribe");
            }
            glue::structed_subscribe(log_subscribe, subscribe);
            
            Status status = user_table_->updateUser(log_subscribe.user_id(), subscribe);
            if (status.ok()) {
              return status;
            }
          } else if (kLogTypeFeedback == data[0]) {
            Feedback log_feedback;
            id_set_t feedback;

            if (!log_feedback.ParseFromArray(c_data + 1, data.length() - 1)) {
              return Status::Corruption("Parse recommend feedback");
            }
            glue::structed_feedback(log_feedback, feedback);
 
            Status status = user_table_->updateFeedback(log_feedback.user_id(), feedback);
            if (status.ok()) {
              return status;
            }
          }
          return Status::InvalidArgument("Invalid data");
        }

      private:
        UserTable* user_table_; 
    };

    class UpdaterBase: public LevelTable<uint64_t, user_info_t>::Updater {
      public:
        virtual user_info_t* clone(user_info_t* value) {
          user_info_t* user_info = new user_info_t();

          user_info->ctime = value->ctime;
          user_info->subscribe = user_info->subscribe;
          user_info->dislike = user_info->dislike;
          user_info->readed = user_info->readed;
          user_info->recommended = user_info->recommended;

          return user_info;
        }
    };

    class UserUpdater: public UpdaterBase {
      public:
        UserUpdater(const map_str_t& subscribe)
          : subscribe_(subscribe) {
          }

        virtual bool update(user_info_t* user_info) {
          user_info->subscribe = subscribe_;

          return true;
        }
      private:
        const map_str_t& subscribe_;
    };

    class ActionUpdater: public UpdaterBase {
      public:
        ActionUpdater(const action_t& user_action)
          : user_action_(user_action) {
          }
        virtual bool update(user_info_t* user_info) {
          if (user_action_.action == ACTION_TYPE_CLICK) {
            std::pair<uint64_t, int32_t> id_time = 
              std::make_pair(user_action_.item_id, time(NULL));

            return user_info->readed.insert(id_time).second;
          } else if (user_action_.action == ACTION_TYPE_DISLIKE) {
            const char* seperator;
            type_id_t id;
            
            // 格式：N_XX, N数字表示不喜欢类型, XX表示来源ID/分类ID/圈ID/SRPID
            int type = atoi(user_action_.dislike_reason.c_str());
            if (0 == type) {
              return false;
            }

            id.type_id_component.type = type;
            if (user_action_.dislike_reason.length() > 2) {
              seperator = strchr(user_action_.dislike_reason.c_str(), '_');
              if (NULL != seperator) {
                seperator++;
                id.type_id_component.id = makeID(seperator, strlen(seperator));
              }
            }
            std::pair<uint64_t, std::string> id_str = 
              std::make_pair(id.type_id, user_action_.dislike_reason);

            return user_info->dislike.insert(id_str).second;
          }
          return true;
        }
      private:
        const action_t& user_action_;
    };

    class CandidateUpdater: public UpdaterBase {
      public:
        CandidateUpdater(const id_set_t& id_set)
          : id_set_(id_set) {
          }

        virtual bool update(user_info_t* user_info) {
          int32_t ctime = time(NULL);
          id_set_t::const_iterator iter = id_set_.begin();
          for (; iter != id_set_.end(); ++iter) {
            user_info->recommended.insert(std::make_pair(*iter, ctime));
          }
          return true;
        }
      private:
        const id_set_t& id_set_;
    };

    class UserFetcher: public LevelTable<uint64_t, user_info_t>::Getter {
      public:
        UserFetcher(user_info_t& user_info)
          : user_info_(user_info) { }
        virtual bool copy(user_info_t* user_info) {
          user_info_.ctime = user_info->ctime;
          user_info_.subscribe = user_info->subscribe;
          user_info_.dislike = user_info->dislike;
          user_info_.readed = user_info->readed;
          user_info_.recommended = user_info->recommended;
          return true;
        }
      private:
        user_info_t& user_info_;
    };

    class HistoryFetcher: public UpdaterBase {
      public:
        HistoryFetcher(id_set_t& id_set)
          : id_set_(id_set) {
          }

        virtual bool update(user_info_t* user_info) {
          map_time_t::iterator iter = user_info->readed.begin();
          for (; iter != user_info->readed.end(); ++iter) {
            //TODO: 添加过期判定
            id_set_.insert(iter->first);
          }
          return true;
        }
      private:
        id_set_t& id_set_;
    };

    class CandidateFilter: public UpdaterBase {
      public:
        CandidateFilter(candidate_set_t& cand_set)
          : cand_set_(cand_set) {
          }
        virtual bool update(user_info_t* user_info) {
          candidate_set_t::iterator iter = cand_set_.begin();

          while(iter != cand_set_.end()) {
            if (user_info->readed.find(iter->item_id) != user_info->readed.end()) {
              cand_set_.erase(iter++);
              continue;
            }
            if (user_info->recommended.find(iter->item_id) != user_info->recommended.end()) {
              cand_set_.erase(iter++);
              continue;
            }
            if (user_info->dislike.find(iter->item_id) != user_info->dislike.end()) {
              cand_set_.erase(iter++);
              continue;
            }
            ++iter;
          }
          return true;
        }
      private:
        candidate_set_t& cand_set_;
    };

    class EliminateUpdater: public UpdaterBase {
      public:
        EliminateUpdater() {
        }
        virtual bool update(user_info_t* user_info) {
          user_info->readed.clear();
          user_info->recommended.clear();
          return true;
        }
    };

    UserTable::UserTable(const Options& opts)
      : TableBase(opts.work_path, kUserTable, kUserFver), level_table_(NULL)
    {
      options_ = opts;
      level_table_ = new level_table_t(opts.max_table_level);
    }

    UserTable::~UserTable()
    {
      if (level_table_)
        delete level_table_;
    }
    
    // 增量更新已读新闻
    Status UserTable::updateAction(uint64_t user_id, const action_t& user_action)
    {
      if (!level_table_->find(user_id)) {
        std::ostringstream oss;

        oss<<"user_id=0x"<<std::hex<<user_id;
        return Status::NotFound(oss.str());
      } 
      ActionUpdater updater(user_action);

      if (level_table_->update(user_id, updater)) {
        return Status::OK();
      }
      std::stringstream oss;

      oss<<"Update user action, user_id=0x"<<std::hex<<user_id;
      oss<<", item_id="<<user_action.item_id;
      return Status::Corruption(oss.str());
    }

    // 增量更新已推荐新闻
    Status UserTable::updateFeedback(uint64_t user_id, const id_set_t& id_set)
    {
      if (!level_table_->find(user_id)) {
        std::ostringstream oss;

        oss<<"user_id=0x"<<std::hex<<user_id;
        return Status::NotFound(oss.str());
      } 
      CandidateUpdater updater(id_set);

      if (level_table_->update(user_id, updater)) {
        return Status::OK();
      }
      std::stringstream oss;

      oss<<"Update recommend feedback, user_id=0x"<<std::hex<<user_id;
      return Status::Corruption(oss.str());
    }

    bool UserTable::isObsolete(const user_info_t* user_info)
    {
      int32_t ctime = time(NULL);
      if (ctime - user_info->ctime > options_.user_hold_time)
        return true;
      return false;
    }

    // 淘汰用户，包括已读，不喜欢和推荐信息
    Status UserTable::eliminate()
    {
      Status status = Status::OK();
      LevelTable<uint64_t, user_info_t>::Iterator iter = level_table_->snapshot();
      for (; iter.hasNext(); iter.next()) {
        const user_info_t* user_info = iter.value();

        // 用户是否过期
        if (isObsolete(user_info)) {
          if (user_info->dislike.size() == 0) {
            // 若用户没有不喜欢信息则直接删除
            if (!level_table_->erase(iter.key())) {
              std::ostringstream oss;

              oss<<"Eliminate user failed: id="<<std::hex<<iter.key();
              status = Status::Corruption(oss.str());
            }
          } else {
            EliminateUpdater updater;

            // 若用户有点击过不喜欢信息则只淘汰已读和推荐结果
            if (!level_table_->update(iter.key(), updater)) {
              std::ostringstream oss;

              oss<<"Eliminate user failed: id="<<std::hex<<iter.key();
              status = Status::Corruption(oss.str());
            }
          }
        }
      }
      return status;
    }

    // 全局更新用户订阅信息
    Status UserTable::updateSubscribe(const Subscribe& subscribe)
    {
      std::string serialized_subscribe;

      serialized_subscribe.append(1, kLogTypeSubscribe);
      if (!subscribe.AppendToString(&serialized_subscribe)) {
        std::ostringstream oss;

        oss<<"Serialize user subscribe";
        oss<<", user_id=0x"<<std::hex<<subscribe.user_id();
        return Status::Corruption(oss.str());
      }

      Status status = writeAheadLog(serialized_subscribe);
      if (!status.ok()) {
        return status;
      }
      map_str_t user_subscribe;

      glue::structed_subscribe(subscribe, user_subscribe);
      return updateUser(subscribe.user_id(), user_subscribe);
    }

    // 更新用户候选集合
    Status UserTable::updateFeedback(const Feedback& feedback)
    {
      std::string serialized_feedback;

      serialized_feedback.append(1, kLogTypeFeedback);
      if (feedback.AppendToString(&serialized_feedback)) {
        std::ostringstream oss;

        oss<<"Serialize recommend feedback";
        oss<<", user_id=0x"<<std::hex<<feedback.user_id();
        return Status::Corruption(oss.str());
      }

      Status status = writeAheadLog(serialized_feedback);
      if (!status.ok()) {
        return status;
      }
      id_set_t feedback_set; 

      for (int i = 0; i < feedback.item_id_size(); ++i) {
        feedback_set.insert(feedback.item_id(i));
      }

      return updateFeedback(feedback.user_id(), feedback_set);
    }

    // 用户操作状态更新
    Status UserTable::updateAction(const Action& action, Action& updated)
    {
      std::string serialized_action;

      serialized_action.append(1, kLogTypeAction);
      if (action.AppendToString(&serialized_action)) {
        std::ostringstream oss;

        oss<<"Serialize user action";
        oss<<", user_id=0x"<<std::hex<<action.user_id();
        return Status::Corruption(oss.str());
      }

      Status status = writeAheadLog(serialized_action);
      if (!status.ok()) {
        return status;
      }
      action_t user_action;

      user_action.item_id = action.item_id();
      user_action.action = action.action();
      user_action.action_time = action.click_time();
      user_action.dislike_reason = action.dislike();

      status = updateAction(action.user_id(), user_action);
      if (!status.ok()) {
        return status;
      }
      id_set_t history_set;

      status = queryHistory(action.user_id(), history_set);
      if (!status.ok()) {
        return status;
      }

      // 删除当前item_id
      id_set_t::iterator iter = history_set.find(action.item_id());
      if (iter != history_set.end()) {
        history_set.erase(iter);
      }

      for (iter = history_set.begin(); iter != history_set.end(); ++iter) {
        updated.add_history_id(*iter);
      }

      return Status::OK();
    }

    bool UserTable::findUser(uint64_t user_id)
    {
      return level_table_->find(user_id);
    }

    Status UserTable::queryUser(uint64_t user_id, user_info_t& user_info)
    {
      UserFetcher fetcher(user_info);

      if (level_table_->get(user_id, fetcher)) {
        return Status::OK();
      }
      std::stringstream oss;

      oss<<"user_id=0x"<<std::hex<<user_id;
      return Status::NotFound(oss.str());
    }

    Status UserTable::updateUser(uint64_t user_id, const map_str_t& subscribe)
    {
      if (!level_table_->find(user_id)) {
        user_info_t* user_info = new user_info_t;

        user_info->ctime = time(NULL);
        user_info->subscribe = subscribe;
        if (level_table_->add(user_id, user_info)) {
          return Status::OK();
        } else {
          std::stringstream oss;

          oss<<"Insert user, user_id=0x"<<std::hex<<user_id;
          return Status::Corruption(oss.str());
        }
      } 
      UserUpdater updater(subscribe);

      if (level_table_->update(user_id, updater)) {
        return Status::OK();
      }
      std::stringstream oss;

      oss<<"Update user, user_id=0x"<<std::hex<<user_id;
      return Status::Corruption(oss.str());
    }

    Status UserTable::queryHistory(uint64_t user_id, id_set_t& id_set)
    {
      if (!level_table_->find(user_id)) {
        std::ostringstream oss;

        oss << "Obsolete user: " << std::hex << user_id;
        return Status::InvalidArgument(oss.str());
      } 
      HistoryFetcher fetcher(id_set);

      if (level_table_->update(user_id, fetcher)) {
        return Status::OK();
      }
      std::stringstream oss;

      oss << "Cann`t fetch user history: user_id=" << user_id;
      return Status::InvalidArgument(oss.str());
    }

    Status UserTable::filterCandidateSet(uint64_t user_id, candidate_set_t& candset)
    {
      if (!level_table_->find(user_id)) {
        std::ostringstream oss;

        oss << "Obsolete user: " << std::hex << user_id;
        return Status::InvalidArgument(oss.str());
      } 
      CandidateFilter filter(candset);

      if (level_table_->update(user_id, filter)) {
        return Status::OK();
      }
      std::stringstream oss;

      oss << "Cann`t filter candidate set: user_id=" << user_id;
      return Status::InvalidArgument(oss.str());
    }

    inline AheadLog* UserTable::createAheadLog() {
      return new UserAheadLog(this, options_.work_path, kUserFver);
    }

    Status UserTable::loadData(const std::string& data) 
    {
      proto::UserInfo log_user_info;

      if (!log_user_info.ParseFromString(data)) {
        return Status::Corruption("Parse user info");
      }
      user_info_t* user_info = new user_info_t;

      if (!level_table_->add(log_user_info.user_id(), user_info)) {
        std::ostringstream oss;

        oss<<"Insert level table: key=0x"<<std::hex<<log_user_info.user_id();
        return Status::Corruption(oss.str());
      }
      return Status::OK();
    }

    Status UserTable::onLoadComplete() 
    {
      if (level_table_->deepen()) {
        return Status::OK();
      }
      return Status::Corruption("Deepen level table failed");
    }

    Status UserTable::dumpToFile(const std::string& temp_name) 
    {
      if (level_table_->depth() >= kMinLevel) {
        if (!level_table_->merge()) {
          return Status::Corruption("Table merge failed");
        }
      }

      level_table_t::Iterator iter = level_table_->snapshot();
      if (!iter.valid()) {
        return Status::Corruption("Table depth not enough");
      }
      TableFileWriter writer(temp_name);

      Status status = writer.create(kUserFver);
      if (!status.ok()) {
        return status;
      }
      std::string serialized_data;

      for (; iter.hasNext(); iter.next()) {
        const user_info_t* user_info = iter.value();
        if (NULL == user_info) {
          continue;
        }
        proto::UserInfo log_user_info;

        log_user_info.set_user_id(iter.key());
        glue::proto_userinfo(*user_info, log_user_info);

        std::string data = log_user_info.SerializeAsString();
        status = writer.write(data);
        if (!status.ok()) {
          writer.close();
          return status;
        }
      }
      writer.close();

      return Status::OK();
    }
  } // namespace news
} // namespace rsys
