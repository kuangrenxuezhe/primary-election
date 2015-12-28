#ifndef RSYS_NEWS_SERVICE_GRPC_H
#define RSYS_NEWS_SERVICE_GRPC_H

#include "core/candidate_db.h"
#include "proto/service.grpc.pb.h"

namespace souyue {
  namespace recmd {
    // 实现GRPC接口
    class ServiceGrpc: public module::protocol::PrimaryElection::Service {
      public:
        ServiceGrpc(CandidateDB* canddiate_db);
        virtual ~ServiceGrpc();

      public:
        //virtual grpc::Status updateAction(grpc::ServerContext* context, const Action* request, Action* response);
        //virtual grpc::Status updateItem(grpc::ServerContext* context, const Item* request, StatusCode* response);
        //virtual grpc::Status updateSubscribe(grpc::ServerContext* context, const Subscribe* request, StatusCode* response);
        //virtual grpc::Status updateFeedback(grpc::ServerContext* context, const Feedback* request, StatusCode* response);
        virtual grpc::Status queryUserStatus(grpc::ServerContext* context, const User* request, UserStatus* response);
        virtual grpc::Status queryCandidateSet(grpc::ServerContext* context, const Recommend* request, CandidateSet* response);
        virtual grpc::Status queryUserInfo(grpc::ServerContext* context, const UserQuery* request, UserInfo* response);
        virtual grpc::Status queryItemInfo(grpc::ServerContext* context, const ItemQuery* request, ItemInfo* response);

      protected:
        grpc::Status failed_status_glue(const Status& status);

      private:
        CandidateDB* candidate_db_;
    };
  } // namespace recmd
} // namespace souyue
#endif // #define RSYS_NEWS_SERVICE_GRPC_H

