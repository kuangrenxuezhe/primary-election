#ifndef RSYS_NEWS_PRIMARY_ELECTION_H
#define RSYS_NEWS_PRIMARY_ELECTION_H

#include <stdint.h>
#include <pthread.h>

#include <map>

#include "core/core_type.h"
#include "core/options.h"
#include "util/base_table.h"
#include "util/status.h"
#include "util/level_table.h"
#include "sparsehash/dense_hash_map"

namespace rsys {
  namespace news {
    typedef std::map<uint64_t, int32_t> itemid_map_t;

    struct user_info_ {
      int32_t ctime; // 用户最后活跃时间
      id_set_t srp; // 用户订阅的srp
      id_set_t circle;	// 用户订阅的圈子

      itemid_map_t	dislike;			// 不喜欢新闻集合
      itemid_map_t  readed;			// 已阅新闻集合
      itemid_map_t	recommended;	// 已推荐新闻集合
    };
    typedef struct user_info_ user_info_t;
    
    class UserTable: public BaseTable<user_info_t> {
      public:
        UserTable(const Options& opts);
        ~UserTable();

      public:
        // 写入表的版本号
        fver_t tableVersion() const {
          return fver_;
        }
        virtual const std::string tableName() const {
          return options_.work_path + "/" + options_.table_name;
        }
        virtual const std::string workPath() const {
          return options_.work_path;
        }

      public:
      // 淘汰用户，包括已读，不喜欢和推荐信息
        Status eliminate(int32_t hold_time);

      public:
        // 查询用户是否存在
        bool findUser(uint64_t user_id);
        // 更新用户订阅信息
        Status updateUser(uint64_t user_id, user_info_t* user_info);

        // 增量更新已读新闻
        Status updateReaded(uint64_t user_id, const action_t& user_action);
        // 增量更新不喜欢新闻
        int updateDislike(uint64_t itemid);
        // 增量更新已推荐新闻
        Status updateCandidateSet(uint64_t user_id, const id_set_t& id_set);

        // 查询用户的历史浏览记录
        Status queryHistory(uint64_t user_id, id_set_t& id_set);
        // 候选集过滤掉用户已浏览，不喜欢和已推荐
        Status filterCandidateSet(uint64_t user_id, candidate_set_t& candset);

      protected:
        virtual value_t* newValue() {
          return NULL;
        }
        virtual bool rollback(const std::string& data) {
          return true;
        }

        bool isObsoleteUser(const user_info_t* user_info, int32_t hold_time);
        bool isObsoleteUserInfo(const user_info_t* user_info, int32_t hold_time);

        virtual bool parseFrom(const std::string& data, uint64_t* user_id, user_info_t* user_info);
        virtual bool serializeTo(uint64_t user_id, const user_info_t* user_info, std::string& data);

      private:
        static fver_t fver_;
        const Options& options_;
    };
  } // namespace news
} // namespace rsys
#endif // #define RSYS_NEWS_PRIMARY_ELECTION_H

