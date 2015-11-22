#ifndef RSYS_NEWS_CORE_TYPE_H
#define RSYS_NEWS_CORE_TYPE_H

#include <stdint.h>
#include <set>
#include <map>
#include <list>
#include <string>

namespace rsys {
  namespace news {
    enum id_type_ {
      IDTYPE_NONE          = 0,
      IDTYPE_DUPLICATED    = 1,
      IDTYPE_BAD_CONTENT   = 2,
      IDTYPE_SOURCE        = 3,
      IDTYPE_ADVERTISEMENT = 4,
      IDTYPE_CATEGORY      = 5,
      IDTYPE_CIRCLE        = 6,
      IDTYPE_SRP           = 7,
    };
    typedef enum id_type_ id_type_t;
    // ID组成
    struct id_component_ {
      uint64_t          type:4; // ID类型：
      uint64_t           id:60;
    };
    typedef struct id_component_ id_component_t;
    // 订阅SRP&圈子ID
    union subscribe_id_ {
      uint64_t                 subscribe_id;
      id_component_t subscribe_id_component;
    };
    typedef union subscribe_id_ subscribe_id_t;
    typedef std::map<uint64_t, std::string>  subscribe_t;

    struct action_ {
      uint64_t           item_id; // 点击的item_id
      int32_t             action; // 用户行为
      int32_t        action_time; // 用户操作时间
      std::string dislike_reason; // 不喜欢原因
    };
    typedef struct action_ action_t;

    struct candidate_ {
      uint64_t     item_id;
      float          power;
      int32_t publish_time;
      int32_t  category_id;
      int32_t  picture_num;
    };
    typedef struct candidate_ candidate_t;

    typedef std::set<uint64_t>                id_set_t;
    typedef std::map<uint64_t, int32_t>     map_time_t;
    typedef std::map<uint64_t, std::string>  map_str_t;
    typedef std::list<candidate_t>     candidate_set_t;
  } // namespace news
} // namespace rsys
#endif // #define RSYS_NEWS_CORE_TYPE_H

