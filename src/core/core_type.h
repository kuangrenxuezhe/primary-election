#ifndef SOUYUE_RECMD_MODELS_PRIMARY_ELECTION_CORE_TYPE_H
#define SOUYUE_RECMD_MODELS_PRIMARY_ELECTION_CORE_TYPE_H

#include <stdint.h>

#include <set>
#include <map>
#include <list>
#include <string>

#include "proto/message.pb.h"
#include "proto/supplement.pb.h"

namespace souyue {
  namespace recmd {
    static const uint64_t kInvalidRegionID = 0UL;

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
      IDTYPE_TOP          = 10,
    };
    typedef enum id_type_ id_type_t;

    enum query_type_ {
      kNormalItem = 0x00,
      kVideoItem  = 0x01,
      kRegionItem = 0x02
    };
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
      int32_t picture_num:16; // 图片个数
      int32_t category_id:16; // 所属分类, 用于返回
      int32_t    click_count; // 点击计数
      int32_t     click_time; // 最近点击时间
      map_pair_t   region_id; // 所属地区
      map_pair_t  belongs_to; // 所属分类, 圈子, SRP词
      map_pair_t         top; // 置顶标记
    };
    typedef struct item_info_ item_info_t;

    // 0x00 normal, 0x01 video, 0x02 region
    struct query_ {
      int       request_num;
      int32_t    start_time;
      int32_t      end_time;
      int         item_type;
      uint64_t    region_id;
    };
    typedef struct query_ query_t;

    enum candidate_type_ {
      kNormalCandidate     = 0,
      kTopCandidate        = 1,
      kPartialTopCandidate = 2,
      kSubscribeCandidate  = 3,
    };
    struct candidate_ {
      int    candidate_type;
      item_info_t item_info;

      candidate_(): candidate_type(0) {
      }
      candidate_(const item_info_t& o)
        : candidate_type(0), item_info(o) {
      }
    };
    typedef struct candidate_ candidate_t;

    typedef std::list<candidate_t>     candidate_set_t;

    namespace glue {
      void structed_action(const Action& proto, action_t& structed); 
      void structed_subscribe(const Subscribe& proto, map_str_t& structed);
      void structed_feedback(const Feedback& proto, id_set_t& structed);

      void structed_user_info(const UserInfo& proto, user_info_t& structed);
      void proto_user_info(const user_info_t& structed, UserInfo& proto);

      void structed_item_info(const ItemInfo& proto, item_info_t& structed);
      void proto_item_info(const item_info_t& structed, ItemInfo& proto);

      void structed_item(const Item& proto, item_info_t& structed);

      // region_id 城市在前省份在后存储，城市可省略
      int zone_to_region_id(const char* zone, uint64_t region_id[2]);
      // zone在外部来转换
      void structed_query(const Recommend& proto, query_t& structed);

      void copy_to_proto(const candidate_t& candidate, CandidateSet& cset);
      void remedy_candidate_weight(const candidate_t& candidate, CandidateSet& cset);

    } // namespace glue
  } // namespace recmd 
} // namespace souyue
#endif // #define SOUYUE_RECMD_MODELS_PRIMARY_ELECTION_CORE_TYPE_H

