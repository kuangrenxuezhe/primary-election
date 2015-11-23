#include "core/core_type.h"
#include "util/util.h"

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

      void glue::structed_userinfo(const proto::UserInfo& proto, user_info_t& structed)
      {
        structed->ctime = proto.ctime();
        for (int i = 0; i < proto.subscribe_size(); ++i) {
          const KeyStr& pair = proto.subscribe(i);
          structed->subscribe.insert(std::make_pair(pair.key(), pair.str()));
        }
        for (int i = 0; i < proto.dislike_size(); ++i) {
          const KeyStr& pair = proto.dislike(i);
          structed->dislike.insert(std::make_pair(pair.key(), pair.str()));
        }
        for (int i = 0; i < proto.readed_size(); ++i) {
          const KeyTime& pair  = proto.readed(i);
          structed->readed.insert(std::make_pair(pair.key(), pair.ctime()));
        }
        for (int i = 0; i < proto.recommended_size(); ++i) {
          const KeyTime& pair  = proto.recommended(i);
          structed->recommended.insert(std::make_pair(pair.key(), pair.ctime()));
        }
      }

      void glue::proto_userinfo(const user_info_t& structed, proto::UserInfo& proto)
      {
        for (map_str_t::const_iterator citer = structed.subscribe.begin();
            citer != structed.subscribe.end(); ++citer) {
          KeyStr* pair = proto.add_subscribe();

          pair->set_key(citer->first);
          pair->set_str(citer->second);
        }
        for (map_str_t::const_iterator citer = structed.dislike.begin();
            citer != structed.dislike.end(); ++citer) {
          KeyStr* pair = proto.add_dislike();

          pair->set_key(citer->first);
          pair->set_str(citer->second);
        }
        for (map_time_t::const_iterator citer = structed.readed.begin();
            citer != structed.readed.end(); ++citer) {
          KeyTime* pair = proto.add_readed();

          pair->set_key(citer->first);
          pair->set_ctime(citer->second);
        }
        for (map_time_t::const_iterator citer = structed.recommended.begin();
            citer != structed.recommended.end(); ++citer) {
          KeyTime* pair = proto.add_recommended();

          pair->set_key(citer->first);
          pair->set_ctime(citer->second);
        }
      }
  } // namespace news
} // namespace rsys
