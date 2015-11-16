#ifndef RSYS_NEWS_SERVICE_GLUE_H
#define RSYS_NEWS_SERVICE_GLUE_H

#include "util/status.h"
#include "core/options.h"
#include "core/candidate_db.h"
#include "proto/service.pb.h"
#include "framework/CF_framework_interface.h"

namespace rsys {
  namespace news {
    static const int32_t kPersistentLibrary = 0;
    static const int32_t kUpdateTrain = 0;
    static const int32_t kPrimaryElectionModuleType = 1;

    class ServiceGlue: public CF_framework_interface {
      public:
        ServiceGlue(): candb_(NULL) {
        }
        virtual ~ServiceGlue() {
          if (candb_) delete candb_;
        }

      public:
        virtual var_4 module_type() {
          return kPrimaryElectionModuleType;
        }
        virtual var_4 init_module(var_vd* config_info);

        virtual var_4 update_user(var_1* user_info);
        virtual var_4 update_item(var_1* item_info);
        virtual var_4 update_click(var_1* click_info);

        virtual var_4 query_user(var_u8 user_id, var_1* result_buf, var_4 result_max, var_4& result_len);	
        virtual var_4 query_recommend(var_u8 user_id, var_4 flag, var_4 beg_time, var_4 end_time, 
            var_1* result_buf, var_4 result_max, var_4& result_len);
        virtual var_4 query_history(var_u8 user_id, var_1* result_buf, var_4 result_max, var_4& result_len);

        virtual var_4 is_persistent_library() {
          return kPersistentLibrary;
        }
        virtual var_4 is_update_train() {
          return kUpdateTrain;
        }
        virtual var_4 update_pushData(var_u8 user_id, var_4 push_num, var_u8* push_data);

      protected:
        int parseUserAction(const char* buffer, int len, UserAction* user_action);
        int parseItemInfo(const char* buffer, int len, ItemInfo* item_info);
        int parseUserSubscribe(const char* buffer, int len, UserSubscribe* user_subscribe);

        int serializeUserInfo(bool user_exist, char* buffer, int maxlen);
        int serializeCandidateSet(const CandidateSet& candset, char* buffer, int maxlen);
        int serializeUserHistory(const IdSet& id_set, char* buffer, int maxlen);

      private:
        static void* flushTimer(void* arg);

      private:
        Options options_;
        CandidateDB* candb_;
        pthread_t flush_handler_;
    };
  }; // namespace news
}; // namespace rsys

#endif // #define RSYS_NEWS_SERVICE_GLUE_H

