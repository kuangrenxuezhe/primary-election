#include "service/service_glue.h"

#include "status.h"
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

    var_4 ServiceGlue::init_module(var_vd* config_info) {
      const Options& options = candidate_db_->options();
      Status status = chrono_.parse(options.flush_timer);
      if (!status.ok()) {
        LOG(ERROR) << status.toString();
        return -1;
      }
      next_flush_time_ = chrono_.next(time(NULL));
    }

    var_4 ServiceGlue::update_action(Action& action)
    {
      Status status = candidate_db_->updateAction(action, action);
      if (status.ok()) {
        return 0;
      }
      LOG(ERROR)<<status.toString();

      if (status.isCorruption() || status.isIOError())
        return -1;
      return 0;
    }

    var_4 ServiceGlue::update_item(const Item& item)
    {
      Status status = candidate_db_->addItem(item);
      if (status.ok()) {
        return 0;
      }
      LOG(ERROR)<<status.toString();

      if (status.isCorruption() || status.isIOError())
        return -1;
      return 0;
    }

    var_4 ServiceGlue::update_subscribe(const Subscribe& subscribe)
    {
      Status status = candidate_db_->updateSubscribe(subscribe);
      if (status.ok()) {
        return 0;
      }
      LOG(ERROR)<<status.toString();

      if (status.isCorruption() || status.isIOError())
        return -1;
      return 0;
    }

    var_4 ServiceGlue::update_feedback(const Feedback& feedback)
    {
      Status status = candidate_db_->updateFeedback(feedback);
      if (status.ok()) {
        return 0;
      }
      LOG(ERROR)<<status.toString();

      if (status.isCorruption() || status.isIOError())
        return -1;
      return 0;
    }

    var_4 ServiceGlue::query_candidate_set(const Recommend& recommend, CandidateSet* cs)
    {
      Status status = candidate_db_->queryCandidateSet(recommend, *cs);
      if (status.ok()) {
        return 0;
      }
      LOG(ERROR)<<status.toString();

      if (status.isCorruption() || status.isIOError())
        return -1;
      return 0;
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

      if (status.isCorruption() || status.isIOError())
        return -1;
      return 0;
    }

    var_4 ServiceGlue::persistent_library()
    {
      int32_t ctime = time(NULL);
      if (next_flush_time_ < ctime) {
        Status status = candidate_db_->flush();
        if (!status.ok()) {
          LOG(FATAL) << status.toString();
          return -1;
        }
        next_flush_time_ = chrono_.next(next_flush_time_);
      }
      return 0;
    }
  }; // namespace news
}; // namespace rsys

