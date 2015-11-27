#ifndef RSYS_NEWS_PRIMARY_ELECTION_H
#define RSYS_NEWS_PRIMARY_ELECTION_H

#include <stdint.h>
#include <pthread.h>

#include <map>

#include "status.h"
#include "table_base.h"
#include "level_table.h"
#include "core/core_type.h"
#include "core/options.h"
#include "proto/record.pb.h"
#include "proto/message.pb.h"

namespace rsys {
  namespace news {
    class UserTable: public TableBase {
      public:
        typedef LevelTable<uint64_t, user_info_t> level_table_t;

      public:
        UserTable(const Options& opts);
        virtual ~UserTable();

      public:
        // 淘汰用户，包括已读，不喜欢和推荐信息
        Status eliminate();
       // 全局更新用户订阅信息, 写ahead-log
        Status updateSubscribe(const Subscribe& subscribe);
        // 更新用户候选集合, 写ahead-log
        Status updateFeedback(const Feedback& feedback);
         // 用户操作状态更新, 写ahead-log
        Status updateAction(const Action& action, Action& updated);

      public:
        // 查询用户是否存在
        bool findUser(uint64_t user_id);
        // 添加user, user_info由外部分配内存, 且不写ahead-log
        Status addUser(uint64_t user_id, user_info_t* user_info);
        // 获取用户信息
        Status queryUser(uint64_t user_id, user_info_t& user_info);
        // 获取用户的阅读历史
        Status queryHistory(uint64_t user_id, id_set_t& id_set);
        // 更新用户订阅信息, 不写ahead-log
        Status updateUser(uint64_t user_id, const map_str_t& subscribe);
        // 增量更新已读新闻, 不写ahead-log
        Status updateAction(uint64_t user_id, const action_t& user_action);
        // 增量更新已推荐新闻, 不写ahead-log
        Status updateFeedback(uint64_t user_id, const id_set_t& id_set);
        // 候选集过滤掉用户已浏览，不喜欢和已推荐
        Status filterCandidateSet(uint64_t user_id, candidate_set_t& candset);

      protected:
        virtual AheadLog* createAheadLog();
        virtual Status loadData(const std::string& data);
        virtual Status onLoadComplete();
        virtual Status dumpToFile(const std::string& name);

      protected:
        bool isObsolete(int32_t last_modified, int32_t ctime);

      private:
        Options            options_;
        level_table_t* level_table_;        
        friend class   UserAheadLog;
    };
  } // namespace news
} // namespace rsys
#endif // #define RSYS_NEWS_PRIMARY_ELECTION_H

