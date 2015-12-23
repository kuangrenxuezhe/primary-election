#include "service/service_grpc.h"
#include "glog/logging.h"

namespace souyue {
  namespace recmd {
    ServiceGrpc::ServiceGrpc(CandidateDB* candidate_db): candidate_db_(candidate_db)
    {
    }

    ServiceGrpc::~ServiceGrpc()
    {
    }

    grpc::Status ServiceGrpc::failed_status_glue(const Status& status)
    {
      if (status.isNotFound()) {
        return grpc::Status(grpc::NOT_FOUND, status.toString());
      } else if (status.isCorruption()) {
        return grpc::Status(grpc::INTERNAL, status.toString());
      } else {
        return grpc::Status(grpc::UNKNOWN, status.toString());
      }
    }

    //grpc::Status ServiceGrpc::updateAction(grpc::ServerContext* context, const Action* request, Action* response)
    //{
    //  Status status = candidate_db_->updateAction(*request, *response);
    //  if (status.ok()) {
    //    return grpc::Status::OK;
    //  }
    //  LOG(ERROR)<<status.toString()<<", from="<<context->peer();

    //  return failed_status_glue(status);
    //}

    //grpc::Status ServiceGrpc::updateItem(grpc::ServerContext* context, const Item* request, StatusCode* response)
    //{
    //  Status status = candidate_db_->addItem(*request);
    //  if (status.ok()) {
    //    response->set_code(CODE_OK);
    //    return grpc::Status::OK;
    //  }
    //  LOG(ERROR)<<status.toString()<<", from="<<context->peer();

    //  return failed_status_glue(status);
    //}

    //grpc::Status ServiceGrpc::updateSubscribe(grpc::ServerContext* context, const Subscribe* request, StatusCode* response)
    //{
    //  Status status = candidate_db_->updateSubscribe(*request);
    //  if (status.ok()) {
    //    response->set_code(CODE_OK);
    //    return grpc::Status::OK;
    //  }
    //  LOG(ERROR)<<status.toString()<<", from="<<context->peer();
    //  
    //  return failed_status_glue(status);
    //}

    //grpc::Status ServiceGrpc::updateFeedback(grpc::ServerContext* context, const Feedback* request, StatusCode* response)
    //{
    //  Status status = candidate_db_->updateFeedback(*request);
    //  if (status.ok()) {
    //    response->set_code(CODE_OK);
    //    return grpc::Status::OK;
    //  }
    //  LOG(ERROR)<<status.toString()<<", from="<<context->peer();

    //  return failed_status_glue(status);
    //}

    grpc::Status ServiceGrpc::queryUserStatus(grpc::ServerContext* context, const User* request, UserStatus* response)
    {
      Status status = candidate_db_->findUser(*request);
      if (status.ok()) {
        response->set_is_new_user(0);
        return grpc::Status::OK;
      } else if (status.isNotFound()) {
        response->set_is_new_user(1);
        return grpc::Status::OK;
      }
      LOG(ERROR)<<status.toString()<<", from="<<context->peer();

      return failed_status_glue(status);
    }

    grpc::Status ServiceGrpc::queryCandidateSet(grpc::ServerContext* context, const Recommend* request, CandidateSet* response)
    {
      Status status = candidate_db_->queryCandidateSet(*request, *response);
      if (status.ok()) {
        return grpc::Status::OK;
      }
      LOG(ERROR)<<status.toString()<<", from="<<context->peer();

      return failed_status_glue(status);
    }

    grpc::Status ServiceGrpc::queryUserInfo(grpc::ServerContext* context, const UserQuery* request, UserInfo* response)
    {
      Status status = candidate_db_->queryUserInfo(*request, *response);
      if (status.ok()) {
        return grpc::Status::OK;
      }
      LOG(ERROR)<<status.toString()<<", from="<<context->peer();

      return failed_status_glue(status);
    }

    grpc::Status ServiceGrpc::queryItemInfo(grpc::ServerContext* context, const ItemQuery* request, ItemInfo* response)
    {
      Status status = candidate_db_->queryItemInfo(*request, *response);
      if (status.ok()) {
        return grpc::Status::OK;
      }
      LOG(ERROR)<<status.toString()<<", from="<<context->peer();

      return failed_status_glue(status);
    }
  } // namespace recmd
} // namespace souyue
