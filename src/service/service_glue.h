#ifndef RSYS_NEWS_SERVICE_GLUE_H
#define RSYS_NEWS_SERVICE_GLUE_H

#include "core/candidate_db.h"
#include "proto/service.pb.h"
#include "proto/record.pb.h"
#include "framework/CF_framework_interface.h"

namespace rsys {
  namespace news {
    static const int32_t kPersistentLibrary = 0;
    static const int32_t kUpdateTrain = 0;
    static const int32_t kPrimaryElectionModuleType = 1;

    class ServiceGlue: public CF_framework_interface {
      public:
        ServiceGlue(CandidateDB* candidate_db);
        virtual ~ServiceGlue();

      public:
        virtual var_4 module_type() {
          return kPrimaryElectionModuleType;
        }
        virtual var_4 init_module(var_vd* config_info) {
          return 0;
        }
        virtual var_4 is_persistent_library() {
          return kPersistentLibrary;
        }
        virtual var_4 is_update_train() {
          return kUpdateTrain;
        }

      public:
        virtual var_4 update_action(Action& action);
        virtual var_4 update_item(const Item& item);
        virtual var_4 update_subscribe(const Subscribe& subscribe);
        virtual var_4 update_feedback(const Feedback& feedback);
        virtual var_4 query_candidate_set(const Recommend& recommend, CandidateSet* cs);
        virtual var_4 query_user_status(const User& user, UserStatus* us);

      protected:
        int parseUserAction(const char* buffer, int len, UserAction* user_action);
        int parseItemInfo(const char* buffer, int len, ItemInfo* item_info);
        int parseUserSubscribe(const char* buffer, int len, UserSubscribe* user_subscribe);

        int serializeUserInfo(bool user_exist, char* buffer, int maxlen);
        int serializeCandidateSet(const CandidateSet& candset, char* buffer, int maxlen);
        int serializeUserHistory(const IdSet& id_set, char* buffer, int maxlen);

      private:
        CandidateDB* candidate_db_;
    };
  }; // namespace news
}; // namespace rsys

#endif // #define RSYS_NEWS_SERVICE_GLUE_H

