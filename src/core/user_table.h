#ifndef RSYS_NEWS_PRIMARY_ELECTION_H
#define RSYS_NEWS_PRIMARY_ELECTION_H

#include <stdint.h>
#include <pthread.h>

#include <map>

#include "core/base_table.h"
#include "util/status.h"
#include "core/core_type.h"
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
    
    class UserTable: public BaseTable {
      public:
        UserTable(const char* path, const char* prefix);
        ~UserTable();

      public:
        // 保存用户表
        Status flushTable();
        // 加载用户表
        Status loadTable();
       // 淘汰用户，包括已读，不喜欢和推荐信息
        Status eliminate(int32_t hold_time);

      public:
        // 设置保存库的最大层级
        void set_max_level(int level);
        // 设置同步库文件的间隔，单位分钟
        void set_sync_gap(int32_t gap);

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
        bool isObsoleteUser(const user_info_t* user_info, int32_t hold_time);
        bool isObsoleteUserInfo(const user_info_t* user_info, int32_t hold_time);

        bool parseFrom(const std::string& data, uint64_t* user_id, user_info_t* user_info);
        bool serializeTo(uint64_t user_id, const user_info_t* user_info, std::string& data);

      private:
        int max_level_;
        int size_;
        int32_t sync_gap_;

        std::string path_;
        std::string prefix_;

        typedef LevelTable<uint64_t, user_info_t> level_table_t;
        level_table_t level_table_;
    };

  }; // namespace rsys
}; // namespace rsys::news
#endif // #define RSYS_NEWS_PRIMARY_ELECTION_H

