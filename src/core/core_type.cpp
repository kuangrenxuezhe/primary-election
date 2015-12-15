#include "core/core_type.h"
#include "util.h"
#include "glog/logging.h"

namespace rsys {
  namespace news {
      void glue::structed_action(const Action& proto, action_t& structed)
      {
        structed.item_id = proto.item_id();
        structed.action = proto.action();
        structed.action_time = proto.click_time();
        structed.dislike_reason = proto.dislike();
      }

      void glue::structed_subscribe(const Subscribe& proto, map_str_t& structed)
      {
        for (int i = 0; i < proto.srp_id_size(); ++i) {
          type_id_t id;

          id.type_id_component.type = IDTYPE_SRP;
          id.type_id_component.id = makeID(proto.srp_id(i));
          structed.insert(std::make_pair(id.type_id, proto.srp_id(i)));
        }

        for (int i = 0; i < proto.circle_id_size(); ++i) {
          type_id_t id;

          id.type_id_component.type = IDTYPE_CIRCLE;
          id.type_id_component.id = makeID(proto.circle_id(i));
          structed.insert(std::make_pair(id.type_id, proto.circle_id(i)));
        }
      }

      void glue::structed_feedback(const Feedback& proto, id_set_t& structed)
      {
        for (int i = 0; i < proto.item_id_size(); ++i) {
          structed.insert(proto.item_id(i));
        }
      }

      void glue::structed_user_info(const proto::UserInfo& proto, user_info_t& structed)
      {
        structed.last_modified = proto.last_modified();
        for (int i = 0; i < proto.subscribe_size(); ++i) {
          const proto::KeyStr& pair = proto.subscribe(i);
          structed.subscribe.insert(std::make_pair(pair.key(), pair.str()));
        }
        for (int i = 0; i < proto.dislike_size(); ++i) {
          const proto::KeyStr& pair = proto.dislike(i);
          structed.dislike.insert(std::make_pair(pair.key(), pair.str()));
        }
        for (int i = 0; i < proto.readed_size(); ++i) {
          const proto::KeyTime& pair  = proto.readed(i);
          structed.readed.insert(std::make_pair(pair.key(), pair.last_modified()));
        }
        for (int i = 0; i < proto.recommended_size(); ++i) {
          const proto::KeyTime& pair  = proto.recommended(i);
          structed.recommended.insert(std::make_pair(pair.key(), pair.last_modified()));
        }
      }

      void glue::proto_user_info(const user_info_t& structed, proto::UserInfo& proto)
      {
        proto.set_last_modified(structed.last_modified);

        proto.mutable_subscribe()->Reserve(structed.subscribe.size());
        for (map_str_t::const_iterator citer = structed.subscribe.begin();
            citer != structed.subscribe.end(); ++citer) {
          proto::KeyStr* pair = proto.add_subscribe();

          pair->set_key(citer->first);
          pair->set_str(citer->second);
        }

        proto.mutable_dislike()->Reserve(structed.dislike.size());
        for (map_str_t::const_iterator citer = structed.dislike.begin();
            citer != structed.dislike.end(); ++citer) {
          proto::KeyStr* pair = proto.add_dislike();

          pair->set_key(citer->first);
          pair->set_str(citer->second);
        }

        proto.mutable_readed()->Reserve(structed.readed.size());
        for (map_time_t::const_iterator citer = structed.readed.begin();
            citer != structed.readed.end(); ++citer) {
          proto::KeyTime* pair = proto.add_readed();

          pair->set_key(citer->first);
          pair->set_last_modified(citer->second);
        }

        proto.mutable_recommended()->Reserve(structed.recommended.size());
        for (map_time_t::const_iterator citer = structed.recommended.begin();
            citer != structed.recommended.end(); ++citer) {
          proto::KeyTime* pair = proto.add_recommended();

          pair->set_key(citer->first);
          pair->set_last_modified(citer->second);
        }
      }

      void glue::structed_item_info(const proto::ItemInfo& proto, item_info_t& structed)
      {
        structed.item_id = proto.item_id();
        structed.click_count = proto.click_count();
        structed.click_time = proto.click_time();
        structed.publish_time = proto.publish_time();
        structed.power = proto.power();
        structed.item_type = proto.item_type();
        structed.picture_num = proto.picture_num();
        structed.category_id = proto.category_id();

        for (int i = 0; i < proto.region_id_size(); ++i) {
          const proto::KeyPair& pair = proto.region_id(i);
          pair_t value = std::make_pair(pair.name(), pair.power());

          structed.region_id.insert(std::make_pair(pair.key(), value));
        }
        for (int i = 0; i < proto.belongs_to_size(); ++i) {
          const proto::KeyPair& pair = proto.belongs_to(i);
          pair_t value = std::make_pair(pair.name(), pair.power());

          structed.belongs_to.insert(std::make_pair(pair.key(), value));
        }
        for (int i= 0; i < proto.top_size(); i++) {
          const proto::KeyPair& pair = proto.top(i);
          pair_t value = std::make_pair(pair.name(), pair.power());

          structed.top.insert(std::make_pair(pair.key(), value));
        }
      }

      void glue::proto_item_info(const item_info_t& structed, proto::ItemInfo& proto)
      {
        proto.set_item_id(structed.item_id);

        proto.set_power(structed.power);
        proto.set_publish_time(structed.publish_time);
        proto.set_item_type(structed.item_type);
        proto.set_picture_num(structed.picture_num);
        proto.set_click_count(structed.click_count);
        proto.set_click_time(structed.click_time);
        proto.set_category_id(structed.category_id);

        proto.mutable_region_id()->Reserve(structed.region_id.size());
        for (map_pair_t::const_iterator iter = structed.region_id.begin();
            iter != structed.region_id.end(); ++iter) {
          proto::KeyPair* pair = proto.add_region_id();

          pair->set_key(iter->first);
          pair->set_name(iter->second.first);
          pair->set_power(iter->second.second);
        }

        proto.mutable_belongs_to()->Reserve(structed.belongs_to.size());
        for (map_pair_t::const_iterator iter = structed.belongs_to.begin();
            iter != structed.belongs_to.end(); ++iter) {
          proto::KeyPair* pair = proto.add_belongs_to();

          pair->set_key(iter->first);
          pair->set_name(iter->second.first);
          pair->set_power(iter->second.second);
        }

        proto.mutable_top()->Reserve(structed.top.size());
        for (map_pair_t::const_iterator iter = structed.top.begin();
            iter != structed.top.end(); ++iter) {
          proto::KeyPair* pair = proto.add_top();

          pair->set_key(iter->first);
          pair->set_name(iter->second.first);
          pair->set_power(iter->second.second);
        }
      }

      void glue::structed_item(const Item& proto, item_info_t& structed)
      {
        structed.click_count = 1;
        structed.click_time = time(NULL);

        structed.item_id = proto.item_id();
        structed.publish_time = proto.publish_time();
        structed.power = proto.power();

        structed.item_type = kNormalItem;
        if (proto.item_type() == ITEM_TYPE_VIDEO)
          structed.item_type = kVideoItem;
        structed.picture_num = proto.picture_num();

        structed.category_id = 0;
        if (proto.category_size() > 0) {
          structed.category_id = proto.category(0).tag_id();
        }

        for (int i = 0; i < proto.category_size(); ++i) {
          type_id_t id;

          id.type_id_component.type = IDTYPE_CATEGORY;
          id.type_id_component.id = proto.category(i).tag_id();

          pair_t value = std::make_pair(proto.category(i).tag_name(), proto.category(i).tag_power());
          structed.belongs_to.insert(std::make_pair(id.type_id, value));
        }

        for (int i = 0; i < proto.srp_size(); ++i) {
          type_id_t id;

          id.type_id_component.type = IDTYPE_SRP;
          id.type_id_component.id = makeID(proto.srp(i).tag_name());

          pair_t value = std::make_pair(proto.srp(i).tag_name(), proto.srp(i).tag_power());
          structed.belongs_to.insert(std::make_pair(id.type_id, value));
        }

        for (int i = 0; i < proto.circle_size(); ++i) {
          type_id_t id;

          id.type_id_component.type = IDTYPE_CIRCLE;
          id.type_id_component.id = makeID(proto.circle(i).tag_name());

          pair_t value = std::make_pair(proto.circle(i).tag_name(), proto.circle(i).tag_power());
          structed.belongs_to.insert(std::make_pair(id.type_id, value));
        }

        if (proto.data_source().source_name().length() > 0) {
          type_id_t id;

          id.type_id_component.type = IDTYPE_SOURCE;
          id.type_id_component.id = makeID(proto.data_source().source_name());

          pair_t value = std::make_pair(proto.data_source().source_name(), 1.0f);
          structed.belongs_to.insert(std::make_pair(id.type_id, value));
        }

        for (int i = 0; i < proto.zone_size(); ++i) {
          uint64_t region_id[2];

          if (proto.zone(i).length() <= 0)
            continue;

          int zone_num = zone_to_region_id(proto.zone(i).c_str(), region_id);
          if (zone_num <= 0) {
            LOG(WARNING) << "Invalid region: " << proto.zone(i);
          } else {
            structed.item_type = kRegionItem;
            pair_t pair = std::make_pair(proto.zone(i), 1.0f);
            structed.region_id.insert(std::make_pair(region_id[0], pair));
            if (zone_num > 1)
              structed.region_id.insert(std::make_pair(region_id[1], pair));
          }
        }

        if (proto.top_info().top_type() == TOP_TYPE_GLOBAL) {
          type_id_t id;

          id.type_id_component.type = IDTYPE_TOP;
          id.type_id_component.id = 0;

          pair_t value = std::make_pair("global_top", 1.0f);
          structed.top.insert(std::make_pair(id.type_id, value));
        } else if (proto.top_info().top_type() == TOP_TYPE_PARTIAL) {
          // 置顶标记
          for (int i = 0; i < proto.top_info().top_srp_id_size(); ++i) {
            type_id_t id;

            id.type_id_component.type = IDTYPE_SRP;
            id.type_id_component.id = makeID(proto.top_info().top_srp_id(i));

            pair_t value = std::make_pair(proto.top_info().top_srp_id(i), 1.0f);
            structed.top.insert(std::make_pair(id.type_id, value));
          }

          for (int i = 0; i < proto.circle_size(); ++i) {
            type_id_t id;

            id.type_id_component.type = IDTYPE_CIRCLE;
            id.type_id_component.id = makeID(proto.circle(i).tag_name());

            pair_t value = std::make_pair(proto.circle(i).tag_name(), proto.circle(i).tag_power());
            structed.top.insert(std::make_pair(id.type_id, value));
          }
        }
      }

      // 地域格式--省:市
      int glue::zone_to_region_id(const char* zone, uint64_t region_id[2])
      {
        const char* seperator = strchr(zone, ':'); 
        if (NULL == seperator) {
          return 0;
        }
        type_id_t id;
        int seperator_len = strlen(seperator + 1);

        if (seperator_len < 1) {
          id.type_id_component.type = IDTYPE_PROVINCE;
          id.type_id_component.id = makeID(zone, seperator - zone);
          region_id[0] = id.type_id;

          return 1;
        } else {
          id.type_id_component.type = IDTYPE_PROVINCE;
          id.type_id_component.id = makeID(zone, seperator - zone);
          region_id[1] = id.type_id;
 
          seperator++;

          id.type_id_component.type = IDTYPE_CITY;
          id.type_id_component.id = makeID(seperator, seperator_len);
          region_id[0] = id.type_id;

          return 2;
        }
      }

      void glue::structed_query(const Recommend& proto, query_t& structed)
      {
        structed.request_num = proto.request_num();
        structed.start_time = proto.beg_time();
        structed.end_time = proto.end_time();
      }

      void glue::copy_to_proto(const candidate_t& candidate, CandidateSet& cset)
      {
        cset.mutable_base()->add_item_id(candidate.item_info.item_id);
        cset.mutable_payload()->add_power(candidate.item_info.power);
        cset.mutable_payload()->add_publish_time(candidate.item_info.publish_time);
        if (candidate.candidate_type == kTopCandidate || candidate.candidate_type == kPartialTopCandidate) {
          cset.mutable_payload()->add_type(CANDIDATE_TYPE_TOP);
        } else  {
          if (candidate.item_info.item_type == kVideoItem)
            cset.mutable_payload()->add_type(CANDIDATE_TYPE_VIDEO);
          else if (candidate.item_info.item_type == kRegionItem)
            cset.mutable_payload()->add_type(CANDIDATE_TYPE_ZONE);
          else
            cset.mutable_payload()->add_type(CANDIDATE_TYPE_NORMAL);
        }
        cset.mutable_payload()->add_picture_num(candidate.item_info.picture_num);
        cset.mutable_payload()->add_category_id(candidate.item_info.category_id);
      }

      void glue::remedy_candidate_weight(const candidate_t& candidate, CandidateSet& cset)
      {
        int last_index = cset.payload().power_size() - 1;
        float power = 0.0f;

        // 置顶和订阅加权
        if (candidate.candidate_type == kTopCandidate) {
          cset.mutable_payload()->set_publish_time(last_index, -1);
        } else if (candidate.candidate_type == kPartialTopCandidate) {
          cset.mutable_payload()->set_publish_time(last_index, -1);
        } else if (candidate.candidate_type == kSubscribeCandidate) {
          power = 10000;
        }

        // 累加点击权重
        power += 10 * candidate.item_info.click_count;
        if (candidate.item_info.power > 100000) {
          power += candidate.item_info.power/100000;  
        } else {
          power += candidate.item_info.power;
        }
        if (power > 100000) power = 100000;

        cset.mutable_payload()->set_power(last_index, power);
      }
  } // namespace news
} // namespace rsys
