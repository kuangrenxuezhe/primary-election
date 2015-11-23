#include "service/service_glue.h"

#include "util/status.h"
#include "glog/logging.h"

namespace rsys {
  namespace news {
    ServiceGlue::ServiceGlue(CandidateDB* candidate_db)
      : candidate_db_(candidate_db)
    {
    }

    ServiceGlue::~ServiceGlue()
    {
    }

    var_4 ServiceGlue::update_action(Action& action)
    {
      Status status = candidate_db_->updateAction(action, action);
      if (status.ok()) {
        return 0;
      }
      LOG(ERROR)<<status.toString();

      return -1;
    }

    var_4 ServiceGlue::update_item(const Item& item)
    {
      Status status = candidate_db_->addItem(item);
      if (status.ok()) {
        return 0;
      }
      LOG(ERROR)<<status.toString();

      return -1;
    }

    var_4 ServiceGlue::update_subscribe(const Subscribe& subscribe)
    {
      Status status = candidate_db_->updateSubscribe(subscribe);
      if (status.ok()) {
        return 0;
      }
      LOG(ERROR)<<status.toString();

      return -1;
    }

    var_4 ServiceGlue::update_feedback(const Feedback& feedback)
    {
      Status status = candidate_db_->updateFeedback(feedback);
      if (status.ok()) {
        return 0;
      }
      LOG(ERROR)<<status.toString();

      return -1;
    }

    var_4 ServiceGlue::query_candidate_set(const Recommend& recommend, CandidateSet* cs)
    {
       Status status = candidate_db_->queryCandidateSet(recommend, *cs);
      if (status.ok()) {
        return 0;
      }
      LOG(ERROR)<<status.toString();

      return -1;
    }

    var_4 ServiceGlue::query_user_status(const User& user, UserStatus* us)
    {
      Status status = candidate_db_->findUser(user);
      if (status.ok()) {
        us->set_is_new_user(0);
        return 0;
      } else if (status.isNotFound()) {
        us->set_is_new_user(1);
        return 0;
      }
      LOG(ERROR)<<status.toString();

      return -1;
    }
  }; // namespace news
}; // namespace rsys

