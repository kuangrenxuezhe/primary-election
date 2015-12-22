#include "core/user_table.h"

#include <unistd.h>
#include <string.h>

#include "utils/util.h"
#include "utils/crc32c.h"
#include "utils/ahead_log.h"
#include "glog/logging.h"
#include "proto/supplement.pb.h"
#include "proto/service.pb.h"

namespace souyue {
  namespace recmd {
    static const fver_t kUserFver(0, 1);
    static const std::string kUserAheadLog = "wal-user";
    static const std::string kUserTable = "table-user.dat";

    static const char kLogTypeAction    = 'A';
    static const char kLogTypeSubscribe = 'S';
    static const char kLogTypeFeedback  = 'F';

    class UserAheadLog: public AheadLog {
      public:
        UserAheadLog(UserTable* user_table, const std::string& path)
          : AheadLog(path, kUserAheadLog, kUserFver), user_table_(user_table) {
        }

      public:
        virtual Status trigger() {
          if (user_table_->level_table_->deepen())
            return Status::OK();
          return Status::Corruption("Deepen user table");
        }

        virtual Status rollback(const std::string& data) {
          if (data.length() <= 1) { // 数据大小至少一个字节，表示类型
            return Status::Corruption("Invalid user info data");
          }
          const char* c_data = data.c_str();

          if (kLogTypeAction == data[0]) {
            Action log_action;

            if (!log_action.ParseFromArray(c_data + 1, data.length() - 1)) {
              return Status::Corruption("Parse user action");
            }
            action_t action;

            glue::structed_action(log_action, action);
            return user_table_->updateAction(log_action.user_id(), action);
          } else if (kLogTypeSubscribe == data[0]) {
            Subscribe log_subscribe;

            if (!log_subscribe.ParseFromArray(c_data + 1, data.length() - 1)) {
              return Status::Corruption("Parse user subscribe");
            }
            map_str_t subscribe;

            glue::structed_subscribe(log_subscribe, subscribe);
            return user_table_->updateUser(log_subscribe.user_id(), subscribe);
          } else if (kLogTypeFeedback == data[0]) {
            Feedback log_feedback;

            if (!log_feedback.ParseFromArray(c_data + 1, data.length() - 1)) {
              return Status::Corruption("Parse recommend feedback");
            }
            id_set_t feedback;

            glue::structed_feedback(log_feedback, feedback);
            return user_table_->updateFeedback(log_feedback.user_id(), feedback);
          }
          return Status::InvalidData("Invalid user data type, type=", data[0]);
        }

      private:
        UserTable* user_table_; 
    };

    class UpdaterBase: public UserTable::level_table_t::Updater {
      public:
        virtual user_info_t* clone(user_info_t* value) {
          return new user_info_t(*value);
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
            std::vector<std::string> tokens;

            splitString(user_action_.dislike_reason, ',', tokens);
            for (size_t i = 0; i < tokens.size(); ++i) {
              const char* seperator;
              type_id_t id;
              std::vector<std::string> key_value;

              splitString(tokens[i], ':', key_value);
              if (key_value.size() != 2) {
                LOG(WARNING) << "Invalid dislike data: " << tokens[i];
                continue;
              }

              trimString(key_value[0], "\"");
              trimString(key_value[1], "\"");

              // 格式：N_XX, N数字表示不喜欢类型, XX表示来源ID/分类ID/圈ID/SRPID
              int type = atoi(key_value[0].c_str());
              if (0 == type) {
                LOG(WARNING) << "Invalid dislike data: " << tokens[i];
                continue;
              }

              id.type_id_component.type = type;
              id.type_id_component.id = 0UL;
              if (key_value[0].length() > 2) {
                seperator = strchr(key_value[0].c_str(), '_');
                if (NULL != seperator) {
                  seperator++;
                  id.type_id_component.id = makeID(seperator, strlen(seperator));
                }
              }

              if (!user_info->dislike.insert(std::make_pair(id.type_id, tokens[i])).second)
                return false;
            }
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

    class UserFetcher: public UserTable::level_table_t::Getter {
      public:
        UserFetcher(user_info_t& user_info)
          : user_info_(user_info) { }
        virtual bool copy(user_info_t* user_info) {
          user_info_ = *user_info;
          return true;
        }
      private:
        user_info_t& user_info_;
    };

    class HistoryFetcher: public UserTable::level_table_t::Getter {
      public:
        HistoryFetcher(id_set_t& id_set)
          : id_set_(id_set) {
          }

        virtual bool copy(user_info_t* user_info) {
          map_time_t::iterator iter = user_info->readed.begin();
          for (; iter != user_info->readed.end(); ++iter) {
            id_set_.insert(iter->first);
          }
          return true;
        }
      private:
        id_set_t& id_set_;
    };

    class CandidateFilter: public UserTable::level_table_t::Getter {
      public:
        CandidateFilter(candidate_set_t& cand_set)
          : cand_set_(cand_set) { }

        virtual bool copy(user_info_t* user_info) {
          candidate_set_t::iterator iter = cand_set_.begin();

          while(iter != cand_set_.end()) {
            if (iter->item_info.top.size() > 0) {
              int top_type = kNormalCandidate;
              map_pair_t::iterator iter_find = iter->item_info.top.begin();

              for (; iter_find != iter->item_info.top.end(); ++iter_find) {
                type_id_t type_id;
                type_id.type_id = iter_find->first;

                if (type_id.type_id_component.type == IDTYPE_TOP) {
                  top_type = kTopCandidate; // 全局推荐
                } else {
                  // 部分推荐，判定是否用户订阅的SRP&圈子
                  if (user_info->subscribe.find(iter_find->first) != user_info->subscribe.end()) {
                    top_type = kPartialTopCandidate;
                  }
                }
              }
              if (top_type == kNormalCandidate) {
                cand_set_.erase(iter++);
              } else {
                iter->candidate_type = top_type;
                iter++;
              }
            } else {
              if (user_info->readed.find(iter->item_info.item_id) != user_info->readed.end()) {
                cand_set_.erase(iter++);
                continue;
              }
              if (user_info->recommended.find(iter->item_info.item_id) != user_info->recommended.end()) {
                cand_set_.erase(iter++);
                continue;
              }
              bool is_dislike = false, is_subscribe = false;
              map_pair_t::iterator iter_find = iter->item_info.belongs_to.begin();

              for (; iter_find != iter->item_info.belongs_to.end(); ++iter_find) {
                if (user_info->dislike.find(iter_find->first) != user_info->dislike.end()) {
                  is_dislike = true;
                }
                if (user_info->subscribe.find(iter_find->first) != user_info->subscribe.end()) {
                  is_subscribe = true;
                }
              }
              if (is_dislike) {
                cand_set_.erase(iter++);
              } else {
                if (is_subscribe) {
                  iter->candidate_type = kSubscribeCandidate;
                }
                iter++;
              }
            }
          }
          return true;
        }
      private:
        candidate_set_t& cand_set_;
    };

    class EliminationUpdater: public UpdaterBase {
      public:
        EliminationUpdater() {
        }
        virtual bool update(user_info_t* user_info) {
          user_info->readed.clear();
          user_info->recommended.clear();
          return true;
        }
    };

    UserTable::UserTable(const ModelOptions& opts)
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

    // 添加user, user_info由外部分配内存, 且不写ahead-log
    Status UserTable::addUser(uint64_t user_id, user_info_t* user_info)
    {
      user_info->last_modified = time(NULL);
      if (level_table_->add(user_id, user_info)) {
        return Status::OK();
      }
      return Status::Corruption("Insert user, user_id=", user_id);
    }

    // 增量更新已读新闻
    Status UserTable::updateAction(uint64_t user_id, const action_t& user_action)
    {
      if (!level_table_->find(user_id)) {
        user_info_t* user_info = new user_info_t;
        Status status = addUser(user_id, user_info);
        if (!status.ok()) {
          delete user_info;
          return status;
        }
      } 
      ActionUpdater updater(user_action);

      if (level_table_->update(user_id, updater)) {
        return Status::OK();
      }
      return Status::Corruption("Update user action, user_id=", user_id, ", item_id=", user_action.item_id);
    }

    // 增量更新已推荐新闻
    Status UserTable::updateFeedback(uint64_t user_id, const id_set_t& id_set)
    {
      if (!level_table_->find(user_id)) {
        return Status::NotFound("Update feedback, user_id=", user_id);
      } 
      CandidateUpdater updater(id_set);

      if (level_table_->update(user_id, updater)) {
        return Status::OK();
      }
      return Status::Corruption("Update feedback, user_id=", user_id);
    }

    bool UserTable::isObsolete(int32_t last_modified, int32_t ctime)
    {
      return last_modified < ctime - options_.user_hold_time ? true:false;
    }

    // 淘汰用户，包括已读，不喜欢和推荐信息
    Status UserTable::eliminate()
    {
      level_table_t::Iterator iter = level_table_->snapshot();
      if (!iter.valid()) {
        return Status::Corruption("Table depth not enough");
      }
      int32_t ctime = time(NULL); 
      Status status = Status::OK();

      for (; iter.hasNext(); iter.next()) {
        const user_info_t* user_info = iter.value();

        // 用户是否过期
        if (!isObsolete(user_info->last_modified, ctime)) {
          continue;
        }

        if (user_info->dislike.size() == 0) {
          // 若用户没有不喜欢信息则直接删除
          if (!level_table_->erase(iter.key())) {
            status = Status::Corruption("Eliminate user, user_id=", iter.key());
          }
        } else {
          EliminationUpdater elimination;

          // 若用户有点击过不喜欢信息则只淘汰已读和推荐结果
          if (!level_table_->update(iter.key(), elimination)) {
            status = Status::Corruption("Eliminate user readed&recommendation, user_id=", iter.key());
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
        return Status::Corruption("Serialize user subscribe, user_id=", subscribe.user_id());
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
      if (!feedback.AppendToString(&serialized_feedback)) {
        return Status::Corruption("Serialize recommend feedback, user_id=", feedback.user_id());
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
      if (!action.AppendToString(&serialized_action)) {
        return Status::Corruption("Serialize user action, user_id=", action.user_id());
      }

      Status status = writeAheadLog(serialized_action);
      if (!status.ok()) {
        return status;
      }
      action_t user_action;

      glue::structed_action(action, user_action);
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

      updated.mutable_history_id()->Reserve(history_set.size());
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
      return Status::NotFound("Query user, user_id=", user_id);
    }

    Status UserTable::updateUser(uint64_t user_id, const map_str_t& subscribe)
    {
      if (!level_table_->find(user_id)) {
        user_info_t* user_info = new user_info_t;
        user_info->subscribe = subscribe;

        Status status = addUser(user_id, user_info);
        if (!status.ok()) {
          delete user_info;
          return status;
        }
      } 
      UserUpdater updater(subscribe);

      if (level_table_->update(user_id, updater)) {
        return Status::OK();
      }
      return Status::Corruption("Update user, user_id=", user_id);
    }

    Status UserTable::queryHistory(uint64_t user_id, id_set_t& id_set)
    {
      if (!level_table_->find(user_id)) {
        return Status::NotFound("Obsolete user: ", user_id);
      } 
      HistoryFetcher fetcher(id_set);

      if (level_table_->get(user_id, fetcher)) {
        return Status::OK();
      }

      return Status::NotFound("User history, user_id=", user_id);
    }

    Status UserTable::filterCandidateSet(uint64_t user_id, candidate_set_t& candset)
    {
      if (!level_table_->find(user_id)) {
        user_info_t* user_info = new user_info_t;
        Status status = addUser(user_id, user_info);
        if (!status.ok()) {
          delete user_info;
        }
        return status;
      } 
      CandidateFilter filter(candset);

      if (level_table_->get(user_id, filter)) {
        return Status::OK();
      }
      return Status::NotFound("Filter candidate set, user_id=", user_id);
    }

    inline AheadLog* UserTable::createAheadLog() {
      return new UserAheadLog(this, options_.work_path);
    }

    Status UserTable::loadData(const std::string& data) 
    {
      UserInfo log_user_info;

      if (!log_user_info.ParseFromString(data)) {
        return Status::Corruption("Parse user info");
      }
      user_info_t* user_info = new user_info_t;

      glue::structed_user_info(log_user_info, *user_info);
      if (!level_table_->add(log_user_info.user_id(), user_info)) {
        delete user_info;
        return Status::Corruption("Insert level table: key=", log_user_info.user_id());
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
        UserInfo log_user_info;

        log_user_info.set_user_id(iter.key());
        glue::proto_user_info(*user_info, log_user_info);

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
  } // namespace recmd
} // namespace souyue
