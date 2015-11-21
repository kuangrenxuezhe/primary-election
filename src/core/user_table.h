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
    enum id_type_ {
      IDTYPE_NONE = 0,
      IDTYPE_DUPLICATED = 1,
      IDTYPE_BAD_CONTENT =2,
      IDTYPE_SOURCE = 3,
      IDTYPE_ADVERTISEMENT = 4,
      IDTYPE_CATEGORY = 5,
      IDTYPE_CIRCLE = 6,
      IDTYPE_SRP = 7,
    };
    typedef enum id_type_ id_type_t;
    // ID组成
    struct id_component_ {
      uint64_t type:4; // ID类型：
      uint64_t id:60;
    };
    typedef struct id_component_ id_component_t;
    // 订阅SRP&圈子ID
    union subscribe_id_ {
      uint64_t subscribe_id;
      id_component_t subscribe_id_component;
    };
    typedef union subscribe_id_ subscribe_id_t;

    typedef std::map<uint64_t, int32_t> map_time_t;
    typedef std::map<uint64_t, std::string>  map_str_t;

    struct user_info_ {
      int32_t ctime; // 用户最后活跃时间
      map_str_t subscribe; // 用户订阅的SRP&圈子
      map_str_t dislike; // 不喜欢新闻集合
      map_time_t readed; // 已阅新闻集合
      map_time_t recommended; // 已推荐新闻集合
    };
    typedef struct user_info_ user_info_t;
    
    class UserAheadLog;
    class UserTable: public TableBase<user_info_t> {
      public:
        UserTable(const Options& opts);
        virtual ~UserTable();

      public:
        // 淘汰用户，包括已读，不喜欢和推荐信息
        Status eliminate(int32_t hold_time);

      public:
        // 查询用户是否存在
        bool findUser(uint64_t user_id);
        // 获取用户信息
        Status getUser(uint64_t user_id, user_info_t* user_info);
        // 更新用户订阅信息
        Status updateUser(uint64_t user_id, user_info_t* user_info);
        // 增量更新已读新闻
        Status updateAction(uint64_t user_id, const action_t& user_action);
        // 增量更新已推荐新闻
        Status updateCandidateSet(uint64_t user_id, const id_set_t& id_set);
        // 查询用户的历史浏览记录
        Status queryHistory(uint64_t user_id, id_set_t& id_set);
        // 候选集过滤掉用户已浏览，不喜欢和已推荐
        Status filterCandidateSet(uint64_t user_id, candidate_set_t& candset);

      protected:
        virtual user_info_t* newValue() {
          return new user_info_t();
        }
        virtual AheadLog* createAheadLog();

        virtual bool parseFrom(const std::string& data, uint64_t* user_id, user_info_t* user_info);
        virtual bool serializeTo(uint64_t user_id, const user_info_t* user_info, std::string& data);

        bool isObsoleteUser(const user_info_t* user_info, int32_t hold_time);
        bool isObsoleteUserInfo(const user_info_t* user_info, int32_t hold_time);

      private:
        Options options_;
        friend class UserAheadLog;
    };
  } // namespace news
} // namespace rsys
#endif // #define RSYS_NEWS_PRIMARY_ELECTION_H

