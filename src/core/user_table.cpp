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

    class UserAheadLog: public AheadLog {
      public:
        UserAheadLog(UserTable* user_table, const std::string& path, 
            const fver_t& fver): AheadLog(path, fver), user_table_(user_table) {
        }

      public:
        // ahead log滚存触发器
        virtual bool trigger() {
          return user_table_->level_table()->deepen();
        }
        // 处理数据回滚
        virtual bool rollback(const std::string& data) {
          uint64_t user_id;

          user_info_t* user_info = user_table_->newValue();
          if (!user_table_->parseFrom(data, &user_id, user_info))
            return false;

          if (!user_table_->level_table()->add(user_id, user_info)) {
            delete user_info;
            return false;
          }
          return true;
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
        UserUpdater(user_info_t* user_info)
          : user_info_(user_info) {
          }
        virtual bool update(user_info_t* user_info) {
          user_info->subscribe = user_info_->subscribe;
          return true;
        }
      private:
        user_info_t* user_info_;
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

    class UserFetcher: public UpdaterBase {
      public:
        UserFetcher(user_info_t* user_info)
          : user_info_(user_info) { }
        virtual bool update(user_info_t* user_info) {
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
        virtual ~EliminateUpdater() {
        }
        virtual bool update(user_info_t* user_info) {
          return true;
        }
    };

    UserTable::UserTable(const Options& opts)
      : TableBase(opts.work_path, opts.table_name, kUserFver, opts.max_table_level)
    {
      options_ = opts;
    }

    UserTable::~UserTable()
    {
    }

    bool UserTable::parseFrom(const std::string& data, uint64_t* user_id, user_info_t* user_info)
    {
      UserRecord user_record;
      if (!user_record.ParseFromString(data))
        return false;

      *user_id = user_record.user_id();
      user_info->ctime = user_record.ctime();
      for (int i = 0; i < user_record.srp_id_size(); ++i) {
        //user_info->srp.insert(user_record.srp_id(i));
      }
      for (int i = 0; i < user_record.circle_id_size(); ++i) {
        //user_info->circle.insert(user_record.circle_id(i));
      }
      for (int i = 0; i < user_record.dislike_size(); ++i) {
        //const ItemPair& pair  = user_record.dislike(i);
        //user_info->dislike.insert(std::make_pair(pair.item_id(), pair.ctime()));
      }
      for (int i = 0; i < user_record.readed_size(); ++i) {
        const ItemPair& pair  = user_record.readed(i);
        user_info->readed.insert(std::make_pair(pair.item_id(), pair.ctime()));
      }
      for (int i = 0; i < user_record.recommended_size(); ++i) {
        const ItemPair& pair  = user_record.recommended(i);
        user_info->recommended.insert(std::make_pair(pair.item_id(), pair.ctime()));
      }
      return true;
    }

    bool UserTable::serializeTo(uint64_t user_id, const user_info_t* user_info, std::string& data)
    {
      UserRecord user_record;

      user_record.set_user_id(user_id);
      //for (id_set_t::const_iterator citer = user_info->srp.begin();
      //    citer != user_info->srp.end(); ++citer) {
      //  user_record.add_srp_id(*citer);
      //}
      //for (id_set_t::const_iterator citer = user_info->circle.begin();
      //    citer != user_info->circle.end(); ++citer) {
      //  user_record.add_circle_id(*citer);
      //}
      //for (itemid_map_t::const_iterator citer = user_info->dislike.begin();
      //    citer != user_info->dislike.end(); ++citer) {
      //  ItemPair* pair = user_record.add_dislike();
      //  pair->set_item_id(citer->first);
      //  pair->set_ctime(citer->second);
      //}
      //for (itemid_map_t::const_iterator citer = user_info->readed.begin();
      //    citer != user_info->readed.end(); ++citer) {
      //  ItemPair* pair = user_record.add_readed();
      //  pair->set_item_id(citer->first);
      //  pair->set_ctime(citer->second);
      //}
      //for (itemid_map_t::const_iterator citer = user_info->recommended.begin();
      //    citer != user_info->recommended.end(); ++citer) {
      //  ItemPair* pair = user_record.add_recommended();
      //  pair->set_item_id(citer->first);
      //  pair->set_ctime(citer->second);
      //}
      data = user_record.SerializeAsString();
      return true;
    }

    // 增量更新已读新闻
    Status UserTable::updateAction(uint64_t user_id, const action_t& user_action)
    {
      if (!level_table()->find(user_id)) {
        std::ostringstream oss;

        // 点击了已淘汰的数据则不记录用户点击
        oss<<"Obsolete user: id="<<std::hex<<user_id;
        return Status::InvalidArgument(oss.str());
      } 
      ActionUpdater updater(user_action);

      if (level_table()->update(user_id, updater)) {
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
      if (!level_table()->find(user_id)) {
        std::ostringstream oss;

        // 点击了已淘汰的数据则不记录用户点击
        oss << "Obsolete user: " << std::hex << user_id;
        return Status::InvalidArgument(oss.str());
      } 
      CandidateUpdater updater(id_set);

      if (level_table()->update(user_id, updater)) {
        return Status::OK();
      }
      std::stringstream oss;

      oss << "Cann`t update user candidate set: user_id=" << user_id;
      return Status::InvalidArgument(oss.str());
    }

    bool UserTable::isObsoleteUser(const user_info_t* user_info, int32_t hold_time)
    {
      return false;
    }

    bool UserTable::isObsoleteUserInfo(const user_info_t* user_info, int32_t hold_time)
    {
      //itemid_map_t::const_iterator iter = user_info->readed.begin();
      //for (; iter != user_info->readed.end(); ++iter) {

      //}

      //iter = user_info->dislike.begin();
      //for (; iter != user_info->dislike.end(); ++iter) {

      //}

      //iter = user_info->recommended.begin();
      //for (; iter != user_info->recommended.end(); ++iter) {

      //}
      return true;
    }

    // 淘汰用户，包括已读，不喜欢和推荐信息
    Status UserTable::eliminate(int32_t hold_time)
    {
      // LevelTable<uint64_t, user_info_t>::Iterator 
      //   iter = level_table().snapshot();

      // while (iter.hasNext()) {
      //   user_info_t* user_info = iter.value();

      //   if (isObsoleteUser(user_info, hold_time))
      //     level_table().erase(iter.key());

      //   // 淘汰用户
      //   if (isObsoleteUserInfo(user_info, hold_time))  {
      //     EliminateUpdater updater;

      //     level_table().update(iter.key(), updater);
      //   }
      //   iter.next();
      // }
      return Status::OK();
    }

    bool UserTable::findUser(uint64_t user_id)
    {
      return level_table()->find(user_id);
    }

    Status UserTable::getUser(uint64_t user_id, user_info_t* user_info)
    {
      UserFetcher fetcher(user_info);

      if (level_table()->update(user_id, fetcher)) {
        return Status::OK();
      }
      std::stringstream oss;

      oss<<"Not found user: id=0x"<<std::hex<<user_id;
      return Status::InvalidArgument(oss.str());
    }

    Status UserTable::updateUser(uint64_t user_id, user_info_t* user_info)
    {
      if (!level_table()->find(user_id)) {
        if (level_table()->add(user_id, user_info)) {
          return Status::OK();
        }
      } 
      UserUpdater updater(user_info);

      if (level_table()->update(user_id, updater)) {
        return Status::OK();
      }
      std::stringstream oss;

      oss << "Cann`t update user info: user_id=" << user_id;
      return Status::InvalidArgument(oss.str());
    }

    Status UserTable::queryHistory(uint64_t user_id, id_set_t& id_set)
    {
      if (!level_table()->find(user_id)) {
        std::ostringstream oss;

        oss << "Obsolete user: " << std::hex << user_id;
        return Status::InvalidArgument(oss.str());
      } 
      HistoryFetcher fetcher(id_set);

      if (level_table()->update(user_id, fetcher)) {
        return Status::OK();
      }
      std::stringstream oss;

      oss << "Cann`t fetch user history: user_id=" << user_id;
      return Status::InvalidArgument(oss.str());
    }

    Status UserTable::filterCandidateSet(uint64_t user_id, candidate_set_t& candset)
    {
      if (!level_table()->find(user_id)) {
        std::ostringstream oss;

        oss << "Obsolete user: " << std::hex << user_id;
        return Status::InvalidArgument(oss.str());
      } 
      CandidateFilter filter(candset);

      if (level_table()->update(user_id, filter)) {
        return Status::OK();
      }
      std::stringstream oss;

      oss << "Cann`t filter candidate set: user_id=" << user_id;
      return Status::InvalidArgument(oss.str());
    }

    inline AheadLog* UserTable::createAheadLog() {
      return new UserAheadLog(this, options_.work_path, kUserFver);
    }
  } // namespace news
} // namespace rsys
