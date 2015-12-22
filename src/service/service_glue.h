#ifndef RSYS_NEWS_SERVICE_GLUE_H
#define RSYS_NEWS_SERVICE_GLUE_H

#include "utils/chrono_expr.h"
#include "core/candidate_db.h"
#include "proto/service.pb.h"
#include "proto/supplement.pb.h"
#include "framework/CF_framework_interface.h"

namespace souyue {
  namespace recmd {
    static const int32_t kPersistentLibrary = 1;
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

       virtual var_4 is_persistent_library() {
          return kPersistentLibrary;
        }
        virtual var_4 is_update_train() {
          return kUpdateTrain;
        }

      public:
        virtual var_4 init_module(var_vd* config_info);

        virtual var_4 update_action(Action& action);
        virtual var_4 update_item(const Item& item);
        virtual var_4 update_subscribe(const Subscribe& subscribe);
        virtual var_4 update_feedback(const Feedback& feedback);
        virtual var_4 query_candidate_set(const Recommend& recommend, CandidateSet* cs);
        virtual var_4 query_user_status(const User& user, UserStatus* us);

        virtual var_4 persistent_library();

      private:
        int32_t   next_flush_time_;
        ChronoExpr         chrono_;
        CandidateDB* candidate_db_;
    };
  } // namespace recmd  
} // namespace souyue

#endif // #define RSYS_NEWS_SERVICE_GLUE_H

