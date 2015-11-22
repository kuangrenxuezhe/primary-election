#ifndef RSYS_NEWS_PRIMARY_ELECTION_H
#define RSYS_NEWS_PRIMARY_ELECTION_H

#include <stdint.h>
#include <pthread.h>

#include <map>

#include "util/status.h"
#include "util/table_base.h"
#include "util/level_table.h"
#include "core/core_type.h"
#include "core/options.h"
#include "sparsehash/dense_hash_map"

namespace rsys {
  namespace news {
    struct user_info_ {
      int32_t          ctime; // 用户最后活跃时间
      subscribe_t  subscribe; // 用户订阅的SRP&圈子
      map_str_t      dislike; // 不喜欢新闻集合
      map_time_t      readed; // 已阅新闻集合
      map_time_t recommended; // 已推荐新闻集合
    };
    typedef struct user_info_ user_info_t;
    
    class UserAheadLog;
    class UserTable: public TableBase {
      public:
        typedef LevelTable<uint64_t, user_info_t> level_table_t;

      public:
        UserTable(const Options& opts);
        virtual ~UserTable();

      public:
        // 淘汰用户，包括已读，不喜欢和推荐信息
        Status eliminate();

      public:
        // 查询用户是否存在
        bool findUser(uint64_t user_id);
        // 获取用户信息
        Status getUser(uint64_t user_id, user_info_t* user_info);
        // 更新用户订阅信息
        Status updateUser(uint64_t user_id, const subscribe_t& subscribe);
        // 增量更新已读新闻
        Status updateAction(uint64_t user_id, const action_t& user_action);
        // 增量更新已推荐新闻
        Status updateCandidateSet(uint64_t user_id, const id_set_t& id_set);
        // 查询用户的历史浏览记录
        Status queryHistory(uint64_t user_id, id_set_t& id_set);
        // 候选集过滤掉用户已浏览，不喜欢和已推荐
        Status filterCandidateSet(uint64_t user_id, candidate_set_t& candset);

      protected:
        bool isObsolete(const user_info_t* user_info);

      protected:
        virtual AheadLog* createAheadLog();
        virtual Status loadData(const std::string& data);
        virtual Status onLoadComplete();
        virtual Status dumpToFile(const std::string& name);
        
      private:
        Options            options_;
        level_table_t* level_table_;        
        friend class   UserAheadLog;
    };
  } // namespace news
} // namespace rsys
#endif // #define RSYS_NEWS_PRIMARY_ELECTION_H

