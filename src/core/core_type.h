#ifndef RSYS_NEWS_CORE_TYPE_H
#define RSYS_NEWS_CORE_TYPE_H

#include <stdint.h>

#include <set>
#include <map>
#include <list>
#include <string>

#include "proto/record.pb.h"
#include "proto/message.pb.h"

namespace rsys {
  namespace news {
    typedef std::pair<std::string, float> pair_t;
    typedef std::set<uint64_t>                id_set_t;
    typedef std::map<uint64_t, int32_t>     map_time_t;
    typedef std::map<uint64_t, std::string>  map_str_t;
    typedef std::map<uint64_t, pair_t>      map_pair_t;
 
    enum id_type_ {
      IDTYPE_NONE          = 0,
      IDTYPE_DUPLICATED    = 1,
      IDTYPE_BAD_CONTENT   = 2,
      IDTYPE_SOURCE        = 3,
      IDTYPE_ADVERTISEMENT = 4,
      IDTYPE_CATEGORY      = 5,
      IDTYPE_CIRCLE        = 6,
      IDTYPE_SRP           = 7,
      IDTYPE_CITY          = 8,  // 城市值要小于省份，便于排序时排在前面
      IDTYPE_PROVINCE      = 9,
    };
    typedef enum id_type_ id_type_t;

    // ID组成
    struct id_component_ {
      uint64_t          type:5; // ID类型：
      uint64_t           id:59;
    };
    typedef struct id_component_ id_component_t;

    // 带类型ID
    union type_id_ {
      uint64_t                 type_id;
      id_component_t type_id_component;
    };
    typedef union type_id_ type_id_t;

    struct action_ {
      uint64_t           item_id; // 点击的item_id
      int32_t             action; // 用户行为
      int32_t        action_time; // 用户操作时间
      std::string dislike_reason; // 不喜欢原因
    };
    typedef struct action_ action_t;

    struct user_info_ {
      int32_t  last_modified; // 用户最后活跃时间
      map_str_t    subscribe; // 用户订阅的SRP&圈子
      map_str_t      dislike; // 不喜欢新闻集合
      map_time_t      readed; // 已阅新闻集合
      map_time_t recommended; // 已推荐新闻集合
    };
    typedef struct user_info_ user_info_t;

     struct item_info_ {
      uint64_t       item_id; // itemid
      float            power; // 初选权重
      int32_t	  publish_time; // 发布时间
      int32_t      item_type; // item类型
      int32_t    picture_num; // 图片个数
      int32_t    click_count; // 点击计数
      int32_t     click_time; // 最近点击时间
      int32_t    category_id; // 所属分类, 用于返回
      map_pair_t   region_id; // 所属地区
      map_pair_t  belongs_to; // 所属分类, 圈子, SRP词
    };
    typedef struct item_info_ item_info_t;

    struct candidate_ {
      uint64_t     item_id;
      float          power;
      int32_t publish_time;
      int32_t  category_id;
      int32_t  picture_num;
    };
    typedef struct candidate_ candidate_t;

    struct query_ {
      int       request_num;
      int32_t    start_time;
      int32_t      end_time;
      int           network;
      uint64_t region_id[2];
    };
    typedef struct query_ query_t;

   typedef std::list<candidate_t>     candidate_set_t;

    namespace glue {
      void structed_action(const Action& proto, action_t& structed); 
      void structed_subscribe(const Subscribe& proto, map_str_t& structed);
      void structed_feedback(const Feedback& proto, id_set_t& structed);

      void structed_user_info(const proto::UserInfo& proto, user_info_t& structed);
      void proto_user_info(const user_info_t& structed, proto::UserInfo& proto);

      void structed_item_info(const proto::ItemInfo& proto, item_info_t& structed);
      void proto_item_info(const item_info_t& structed, proto::ItemInfo& proto);

      void structed_item(const Item& proto, item_info_t& structed);
      void structed_query(const Recommend& proto, query_t& structed);
    } // namespace glue
  } // namespace news
} // namespace rsys
#endif // #define RSYS_NEWS_CORE_TYPE_H

