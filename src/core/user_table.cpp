#include "core/user_table.h"

#include <unistd.h>
#include <string.h>
#include <openssl/md5.h>

#include "util/crc32c.h"
#include "glog/logging.h"

#include "proto/record.pb.h"
#include "proto/service.pb.h"

namespace rsys {
  namespace news {
    static const fver_t kUserFver(0, 1);
    static const std::string kUserAheadLog = "wal-user";
    static const std::string kUserTable = "table-user.dat";

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
          return Status::Corruption("Trigger");
        }
        // 处理数据回滚
        virtual Status rollback(const std::string& data) {
          LogRecord record;
          if (!record.ParseFromString(data)) {
            return Status::Corruption("Parse user info");
          }

          if (record.record().Is<LogAction>()) {
            LogAction log_action;
            action_t action;

            record.record().UnpackTo(&log_action);
            action.item_id = log_action.item_id();
            action.action = log_action.action();
            action.action_time = log_action.action_time();
            action.dislike_reason = log_action.dislike_reason();

            Status status = user_table_->updateAction(log_action.user_id(), action);
            if (status.ok())
              return status;
          } else if (record.record().Is<LogSubscribe>()) {
            LogSubscribe log_subscribe;
            subscribe_t subscribe;

            record.record().UnpackTo(&log_subscribe);
            for (int i = 0; i < log_subscribe.keystr_size(); ++i) {
              const KeyStr& pair = log_subscribe.keystr(i);
              subscribe.insert(std::make_pair(pair.key(), pair.str()));
            }

            Status status = user_table_->updateUser(log_subscribe.user_id(), subscribe);
            if (status.ok())
              return status;
          } else if (record.record().Is<LogCandidateSet>()) {
            LogCandidateSet log_cand_set;
            id_set_t cand_set;

            record.record().UnpackTo(&log_cand_set);
            for (int i = 0; i < log_cand_set.cand_id_size(); ++i) {
              cand_set.insert(log_cand_set.cand_id(i));
            }

            Status status = user_table_->updateCandidateSet(log_cand_set.user_id(), cand_set);
            if (status.ok())
              return status;
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
        UserUpdater(const subscribe_t& subscribe)
          : subscribe_(subscribe) {
          }
        virtual bool update(user_info_t* user_info) {
          user_info->subscribe = subscribe_;
          return true;
        }
      private:
        const subscribe_t& subscribe_;
    };

    class ActionUpdater: public UpdaterBase {
      public:
        ActionUpdater(const action_t& user_action)
          : user_action_(user_action) {
          }
        // 创建60位的id
        uint64_t make_id(const char* str, size_t len) {
          uint8_t digest[16];
          MD5_CTX ctx;

          MD5_Init(&ctx);
          MD5_Update(&ctx, str, len);
          MD5_Final(digest, &ctx);
          return (*(uint64_t*)digest)&0xFFFFFFFFFFFFFFFUL;
        }
        virtual bool update(user_info_t* user_info) {
          if (user_action_.action == ACTION_TYPE_CLICK) {
            std::pair<uint64_t, int32_t> id_time = 
              std::make_pair(user_action_.item_id, time(NULL));
            return user_info->readed.insert(id_time).second;
          } else if (user_action_.action == ACTION_TYPE_DISLIKE) {
            const char* seperator;
            subscribe_id_t sid;
            
            // 格式：N_XX, N数字表示不喜欢类型, XX表示来源ID/分类ID/圈ID/SRPID
            sid.subscribe_id_component.type = atoi(user_action_.dislike_reason.c_str());
            if (user_action_.dislike_reason.length() > 2) {
              seperator = strchr(user_action_.dislike_reason.c_str(), '_');
              if (NULL != seperator) {
                sid.subscribe_id_component.id = make_id(seperator + 1, strlen(seperator));
              }
            }
            std::pair<uint64_t, std::string> id_str = 
              std::make_pair(sid.subscribe_id, user_action_.dislike_reason);
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
          id_set_t::iterator iter = id_set_.begin();
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
        UserFetcher(user_info_t* user_info)
          : user_info_(user_info) { }
        virtual bool copy(user_info_t* user_info) {
          user_info_->ctime = user_info->ctime;
          user_info_->subscribe = user_info->subscribe;
          user_info_->dislike = user_info->dislike;
          user_info_->readed = user_info->readed;
          user_info_->recommended = user_info->recommended;
          return true;
        }
      private:
        user_info_t* user_info_;
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
      LogRecord record;
      LogAction log_action;

      log_action.set_user_id(user_id);
      log_action.set_item_id(user_action.item_id);
      log_action.set_action(user_action.action);
      log_action.set_action_time(user_action.action_time);
      log_action.set_dislike_reason(user_action.dislike_reason);
      record.mutable_record()->PackFrom(log_action);

      std::string serialize_record = record.SerializeAsString();
      Status status = writeAheadLog(serialize_record);
      if (!status.ok()) {
        return status;
      }

      if (!level_table_->find(user_id)) {
        std::ostringstream oss;

        // 点击了已淘汰的数据则不记录用户点击
        oss<<"Obsolete user: id="<<std::hex<<user_id;
        return Status::InvalidArgument(oss.str());
      } 
      ActionUpdater updater(user_action);

      if (level_table_->update(user_id, updater)) {
        return Status::OK();
      }
      std::stringstream oss;

      oss << "Cann`t update user action: user_id=" << user_id
        << ", item_id=" << user_action.item_id;
      return Status::InvalidArgument(oss.str());
    }

    // 增量更新已推荐新闻
    Status UserTable::updateCandidateSet(uint64_t user_id, const id_set_t& id_set)
    {
      LogRecord record;
      LogCandidateSet log_cand_set;

      id_set_t::iterator iter = id_set.begin();
      for (; iter != id_set.end(); ++iter) {
        log_cand_set.add_cand_id(*iter);
      }
      log_cand_set.set_user_id(user_id);
      record.mutable_record()->PackFrom(log_cand_set);

      std::string serialize_id_set = record.SerializeAsString();
      Status status = writeAheadLog(serialize_id_set);
      if (!status.ok()) {
        return status;
      }

      if (!level_table_->find(user_id)) {
        std::ostringstream oss;

        // 点击了已淘汰的数据则不记录用户点击
        oss << "Obsolete user: " << std::hex << user_id;
        return Status::InvalidArgument(oss.str());
      } 
      CandidateUpdater updater(id_set);

      if (level_table_->update(user_id, updater)) {
        return Status::OK();
      }
      std::stringstream oss;

      oss << "Cann`t update user candidate set: user_id=" << user_id;
      return Status::InvalidArgument(oss.str());
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

    bool UserTable::findUser(uint64_t user_id)
    {
      return level_table_->find(user_id);
    }

    Status UserTable::getUser(uint64_t user_id, user_info_t* user_info)
    {
      UserFetcher fetcher(user_info);

      if (level_table_->get(user_id, fetcher)) {
        return Status::OK();
      }
      std::stringstream oss;

      oss<<"Not found user: id=0x"<<std::hex<<user_id;
      return Status::InvalidArgument(oss.str());
    }

    Status UserTable::updateUser(uint64_t user_id, const subscribe_t& subscribe)
    {
      LogRecord record;
      LogSubscribe log_subscribe;

      map_str_t::const_iterator iter = subscribe.begin();
      for (; iter != subscribe.end(); ++iter) {
        KeyStr* keystr = log_subscribe.add_keystr();

        keystr->set_key(iter->first);
        keystr->set_str(iter->second);
      }
      log_subscribe.set_user_id(user_id);
      record.mutable_record()->PackFrom(log_subscribe);

      std::string serialize_subscribe = record.SerializeAsString();
      Status status = writeAheadLog(serialize_subscribe);
      if (!status.ok()) {
        return status;
      }

      if (!level_table_->find(user_id)) {
        user_info_t* user_info = new user_info_t;

        user_info->ctime = time(NULL);
        user_info->subscribe = subscribe;
        if (level_table_->add(user_id, user_info)) {
          return Status::OK();
        } else {
          std::stringstream oss;

          oss<<"Cann`t insert user: id="<<std::hex<<user_id;
          return Status::InvalidArgument(oss.str());
        }
      } 
      UserUpdater updater(subscribe);

      if (level_table_->update(user_id, updater)) {
        return Status::OK();
      }
      std::stringstream oss;

      oss<<"Cann`t update user info: id="<<std::hex<<user_id;
      return Status::InvalidArgument(oss.str());
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
      LogUserInfo log_user_info;
      if (!log_user_info.ParseFromString(data)) {
        return Status::Corruption("Parse user_info failed");
      }
      user_info_t* user_info = new user_info_t;

      user_info->ctime = log_user_info.ctime();
      for (int i = 0; i < log_user_info.subscribe_size(); ++i) {
        const KeyStr& pair = log_user_info.subscribe(i);
        user_info->subscribe.insert(std::make_pair(pair.key(), pair.str()));
      }
      for (int i = 0; i < log_user_info.dislike_size(); ++i) {
        const KeyStr& pair = log_user_info.dislike(i);
        user_info->dislike.insert(std::make_pair(pair.key(), pair.str()));
      }
      for (int i = 0; i < log_user_info.readed_size(); ++i) {
        const KeyTime& pair  = log_user_info.readed(i);
        user_info->readed.insert(std::make_pair(pair.key(), pair.ctime()));
      }
      for (int i = 0; i < log_user_info.recommended_size(); ++i) {
        const KeyTime& pair  = log_user_info.recommended(i);
        user_info->recommended.insert(std::make_pair(pair.key(), pair.ctime()));
      }

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
        LogUserInfo log_user_info;

        log_user_info.set_user_id(iter.key());
        for (subscribe_t::const_iterator citer = user_info->subscribe.begin();
            citer != user_info->subscribe.end(); ++citer) {
          KeyStr* pair = log_user_info.add_subscribe();
          pair->set_key(citer->first);
          pair->set_str(citer->second);
        }
        for (map_str_t::const_iterator citer = user_info->dislike.begin();
            citer != user_info->dislike.end(); ++citer) {
          KeyStr* pair = log_user_info.add_dislike();
          pair->set_key(citer->first);
          pair->set_str(citer->second);
        }
        for (map_time_t::const_iterator citer = user_info->readed.begin();
            citer != user_info->readed.end(); ++citer) {
          KeyTime* pair = log_user_info.add_readed();
          pair->set_key(citer->first);
          pair->set_ctime(citer->second);
        }
        for (map_time_t::const_iterator citer = user_info->recommended.begin();
            citer != user_info->recommended.end(); ++citer) {
          KeyTime* pair = log_user_info.add_recommended();
          pair->set_key(citer->first);
          pair->set_ctime(citer->second);
        }
        std::string serialized_data = log_user_info.SerializeAsString();

        status = writer.write(serialized_data);
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
