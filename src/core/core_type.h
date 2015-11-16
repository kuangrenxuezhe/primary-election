#ifndef RSYS_NEWS_CORE_TYPE_H
#define RSYS_NEWS_CORE_TYPE_H

#include <stdint.h>
#include <set>
#include <list>

namespace rsys {
  namespace news {

    struct action_ {
      uint64_t item_id; // 点击的item_id
      int32_t action; // 用户行为
      int32_t action_time; // 用户操作时间
    };
    typedef struct action_ action_t;

    struct candidate_ {
      uint64_t item_id;
      float power;
      int32_t publish_time;
      int32_t category_id;
      int32_t picture_num;
    };
    typedef struct candidate_ candidate_t;

    typedef std::set<uint64_t> id_set_t;
    typedef std::list<candidate_t> candidate_set_t;
  } // namespace news
} // namespace rsys
#endif // #define RSYS_NEWS_CORE_TYPE_H

