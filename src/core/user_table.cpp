#include "core/user_table.h"

#include <unistd.h>
#include <string.h>

#include "util/crc32c.h"
#include "glog/logging.h"

#include "proto/record.pb.h"
#include "proto/service.pb.h"

namespace rsys {
  namespace news {
    fver_t UserTable::fver_(0, 1);

    class UserUpdater: public LevelTable<uint64_t, user_info_t>::Updater {
      public:
        UserUpdater(user_info_t* user_info)
          : user_info_(user_info) {
          }
        virtual ~UserUpdater() {
        }
        virtual bool update(user_info_t* user_info) {
          // 用户订阅信息为全量更新
          user_info->srp = user_info_->srp;
          user_info->circle = user_info_->circle;
          return true;
        }
        virtual user_info_t* clone(user_info_t* value) {
          return NULL;
        }

      private:
        user_info_t* user_info_;
    };

    class ActionUpdater: public LevelTable<uint64_t, user_info_t>::Updater {
      public:
        ActionUpdater(const action_t& user_action)
          : user_action_(user_action) {
          }
        virtual ~ActionUpdater() {
        }
        virtual bool update(user_info_t* user_info) {
          std::pair<uint64_t, int32_t> item_id = 
            std::make_pair(user_action_.item_id, time(NULL));
          if (user_action_.action == ACTION_CLICK) {
            user_info->readed.insert(item_id);
          } else if (user_action_.action == ACTION_DISLIKE) {
            user_info->dislike.insert(item_id);
          }
          return true;
        }
        virtual user_info_t* clone(user_info_t* value) {
          return NULL;
        }

      private:
        const action_t& user_action_;
    };

    class CandidateUpdater: public LevelTable<uint64_t, user_info_t>::Updater {
      public:
        CandidateUpdater(const id_set_t& id_set)
          : id_set_(id_set) {
          }
        virtual ~CandidateUpdater() {
        }
        virtual bool update(user_info_t* user_info) {
          int32_t ctime = time(NULL);
          id_set_t::iterator iter = id_set_.begin();
          for (; iter != id_set_.end(); ++iter) {
            user_info->recommended.insert(std::make_pair(*iter, ctime));
          }
          return true;
        }
        virtual user_info_t* clone(user_info_t* value) {
          return NULL;
        }

      private:
        const id_set_t& id_set_;
    };

    class HistoryFetcher: public LevelTable<uint64_t, user_info_t>::Updater {
      public:
        HistoryFetcher(id_set_t& id_set)
          : id_set_(id_set) {
          }
        virtual ~HistoryFetcher() {
        }
        virtual bool update(user_info_t* user_info) {
          itemid_map_t::iterator iter = user_info->readed.begin();
          for (; iter != user_info->readed.end(); ++iter) {
            //TODO: 添加过期判定
            id_set_.insert(iter->first);
          }
          return true;
        }
        virtual user_info_t* clone(user_info_t* value) {
          return NULL;
        }

      private:
        id_set_t& id_set_;
    };

    class CandidateFilter: public LevelTable<uint64_t, user_info_t>::Updater {
      public:
        CandidateFilter(candidate_set_t& cand_set)
          : cand_set_(cand_set) {
          }
        virtual ~CandidateFilter() {
        }
        virtual bool update(user_info_t* user_info) {
          candidate_set_t::iterator iter = cand_set_.begin();
          while (iter != cand_set_.end()) {
            if (user_info->readed.find(iter->item_id) != user_info->readed.end()) {
              cand_set_.erase(iter++);
              continue;
            }
            if (user_info->dislike.find(iter->item_id) != user_info->dislike.end()) {
              cand_set_.erase(iter++);
              continue;
            }
            if (user_info->recommended.find(iter->item_id) != user_info->recommended.end()) {
              cand_set_.erase(iter++);
              continue;
            }
            ++iter;
          }
          return true;
        }
        virtual user_info_t* clone(user_info_t* value) {
          return NULL;
        }

      private:
        candidate_set_t& cand_set_;
    };

    class EliminateUpdater: public LevelTable<uint64_t, user_info_t>::Updater {
      public:
        EliminateUpdater() {
        }
        virtual ~EliminateUpdater() {
        }
        virtual bool update(user_info_t* user_info) {
          return true;
        }
        virtual user_info_t* clone(user_info_t* value) {
          return NULL;
        }
    };

    UserTable::UserTable(const Options& opts): options_(opts)
    {
      //mutex_ = new pthread_mutex_t;
      //if (NULL != path && '\0' == path[0])
      //  path_ = path;
      //if (NULL != prefix && '\0' == prefix[0])
      //  prefix_ = prefix;
      //pthread_mutex_init(mutex_, NULL);
    }

    UserTable::~UserTable()
    {
      //if (mutex_)
      //  delete mutex_;
      //if (user_map_) {
      //  for (int i=0; i<max_level_; ++i) {
      //    if (user_map_[i])
      //      delete user_map_[i];
      //  }
      //  delete[] user_map_;
      //}
      //if (rwlock_) {
      //  for (int i=0; i<max_level_; ++i) {
      //    if (rwlock_[i])
      //      delete rwlock_[i];
      //  }
      //  delete[] rwlock_;
      //}
    }

    bool UserTable::parseFrom(const std::string& data, uint64_t* user_id, user_info_t* user_info)
    {
      UserRecord user_record;
      if (!user_record.ParseFromString(data))
        return false;

      *user_id = user_record.user_id();
      user_info->ctime = user_record.ctime();
      for (int i = 0; i < user_record.srp_id_size(); ++i) {
        user_info->srp.insert(user_record.srp_id(i));
      }
      for (int i = 0; i < user_record.circle_id_size(); ++i) {
        user_info->circle.insert(user_record.circle_id(i));
      }
      for (int i = 0; i < user_record.dislike_size(); ++i) {
        const ItemPair& pair  = user_record.dislike(i);
        user_info->dislike.insert(std::make_pair(pair.item_id(), pair.ctime()));
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
      for (id_set_t::const_iterator citer = user_info->srp.begin();
          citer != user_info->srp.end(); ++citer) {
        user_record.add_srp_id(*citer);
      }
      for (id_set_t::const_iterator citer = user_info->circle.begin();
          citer != user_info->circle.end(); ++citer) {
        user_record.add_circle_id(*citer);
      }
      for (itemid_map_t::const_iterator citer = user_info->dislike.begin();
          citer != user_info->dislike.end(); ++citer) {
        ItemPair* pair = user_record.add_dislike();
        pair->set_item_id(citer->first);
        pair->set_ctime(citer->second);
      }
      for (itemid_map_t::const_iterator citer = user_info->readed.begin();
          citer != user_info->readed.end(); ++citer) {
        ItemPair* pair = user_record.add_readed();
        pair->set_item_id(citer->first);
        pair->set_ctime(citer->second);
      }
      for (itemid_map_t::const_iterator citer = user_info->recommended.begin();
          citer != user_info->recommended.end(); ++citer) {
        ItemPair* pair = user_record.add_recommended();
        pair->set_item_id(citer->first);
        pair->set_ctime(citer->second);
      }
      data = user_record.SerializeAsString();
      return true;
    }

    // 增量更新已读新闻
    Status UserTable::updateReaded(uint64_t user_id, const action_t& user_action)
    {
      if (level_table().find(user_id)) {
        std::ostringstream oss;

        // 点击了已淘汰的数据则不记录用户点击
        oss << "Obsolete user: " << std::hex << user_id;
        return Status::InvalidArgument(oss.str());
      } 
      ActionUpdater updater(user_action);

      if (level_table().update(user_id, updater)) {
        return Status::OK();
      }
      std::stringstream oss;

      oss << "Cann`t update user action: user_id=" << user_id
        << ", item_id=" << user_action.item_id;
      return Status::InvalidArgument(oss.str());
    }

    // 增量更新不喜欢新闻
    int UserTable::updateDislike(uint64_t itemid)
    {
      return 0;
    }

    // 增量更新已推荐新闻
    Status UserTable::updateCandidateSet(uint64_t user_id, const id_set_t& id_set)
    {
      if (!level_table().find(user_id)) {
        std::ostringstream oss;

        // 点击了已淘汰的数据则不记录用户点击
        oss << "Obsolete user: " << std::hex << user_id;
        return Status::InvalidArgument(oss.str());
      } 
      CandidateUpdater updater(id_set);

      if (level_table().update(user_id, updater)) {
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
      itemid_map_t::const_iterator iter = user_info->readed.begin();
      for (; iter != user_info->readed.end(); ++iter) {

      }

      iter = user_info->dislike.begin();
      for (; iter != user_info->dislike.end(); ++iter) {

      }

      iter = user_info->recommended.begin();
      for (; iter != user_info->recommended.end(); ++iter) {

      }
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
      return level_table().find(user_id);
    }

    Status UserTable::updateUser(uint64_t user_id, user_info_t* user_info)
    {
      if (!level_table().find(user_id)) {
        if (level_table().add(user_id, user_info)) {
          return Status::OK();
        }
      } 
      UserUpdater updater(user_info);

      if (level_table().update(user_id, updater)) {
        return Status::OK();
      }
      std::stringstream oss;

      oss << "Cann`t update user info: user_id=" << user_id;
      return Status::InvalidArgument(oss.str());
    }

    Status UserTable::queryHistory(uint64_t user_id, id_set_t& id_set)
    {
      if (!level_table().find(user_id)) {
        std::ostringstream oss;

        oss << "Obsolete user: " << std::hex << user_id;
        return Status::InvalidArgument(oss.str());
      } 
      HistoryFetcher fetcher(id_set);

      if (level_table().update(user_id, fetcher)) {
        return Status::OK();
      }
      std::stringstream oss;

      oss << "Cann`t fetch user history: user_id=" << user_id;
      return Status::InvalidArgument(oss.str());
    }

    Status UserTable::filterCandidateSet(uint64_t user_id, candidate_set_t& candset)
    {
      if (!level_table().find(user_id)) {
        std::ostringstream oss;

        oss << "Obsolete user: " << std::hex << user_id;
        return Status::InvalidArgument(oss.str());
      } 
      CandidateFilter filter(candset);

      if (level_table().update(user_id, filter)) {
        return Status::OK();
      }
      std::stringstream oss;

      oss << "Cann`t filter candidate set: user_id=" << user_id;
      return Status::InvalidArgument(oss.str());
    }
  } // namespace news
} // namespace rsys
