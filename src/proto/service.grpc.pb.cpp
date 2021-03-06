// Generated by the gRPC protobuf plugin.
// If you make any local change, they will be lost.
// source: service.proto

#include "service.pb.h"
#include "service.grpc.pb.h"

#include <grpc++/channel.h>
#include <grpc++/impl/client_unary_call.h>
#include <grpc++/impl/rpc_service_method.h>
#include <grpc++/impl/service_type.h>
#include <grpc++/support/async_unary_call.h>
#include <grpc++/support/async_stream.h>
#include <grpc++/support/sync_stream.h>
namespace module {
namespace protocol {

static const char* PrimaryElection_method_names[] = {
  "/module.protocol.PrimaryElection/queryUserInfo",
  "/module.protocol.PrimaryElection/queryItemInfo",
  "/module.protocol.PrimaryElection/queryUserStatus",
  "/module.protocol.PrimaryElection/queryCandidateSet",
  "/module.protocol.PrimaryElection/updateAction",
  "/module.protocol.PrimaryElection/deleteUserDislike",
};

std::unique_ptr< PrimaryElection::Stub> PrimaryElection::NewStub(const std::shared_ptr< ::grpc::Channel>& channel, const ::grpc::StubOptions& options) {
  std::unique_ptr< PrimaryElection::Stub> stub(new PrimaryElection::Stub(channel));
  return stub;
}

PrimaryElection::Stub::Stub(const std::shared_ptr< ::grpc::Channel>& channel)
  : channel_(channel), rpcmethod_queryUserInfo_(PrimaryElection_method_names[0], ::grpc::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_queryItemInfo_(PrimaryElection_method_names[1], ::grpc::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_queryUserStatus_(PrimaryElection_method_names[2], ::grpc::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_queryCandidateSet_(PrimaryElection_method_names[3], ::grpc::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_updateAction_(PrimaryElection_method_names[4], ::grpc::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_deleteUserDislike_(PrimaryElection_method_names[5], ::grpc::RpcMethod::NORMAL_RPC, channel)
  {}

::grpc::Status PrimaryElection::Stub::queryUserInfo(::grpc::ClientContext* context, const ::module::protocol::UserQuery& request, ::module::protocol::UserInfo* response) {
  return ::grpc::BlockingUnaryCall(channel_.get(), rpcmethod_queryUserInfo_, context, request, response);
}

::grpc::ClientAsyncResponseReader< ::module::protocol::UserInfo>* PrimaryElection::Stub::AsyncqueryUserInfoRaw(::grpc::ClientContext* context, const ::module::protocol::UserQuery& request, ::grpc::CompletionQueue* cq) {
  return new ::grpc::ClientAsyncResponseReader< ::module::protocol::UserInfo>(channel_.get(), cq, rpcmethod_queryUserInfo_, context, request);
}

::grpc::Status PrimaryElection::Stub::queryItemInfo(::grpc::ClientContext* context, const ::module::protocol::ItemQuery& request, ::module::protocol::ItemInfo* response) {
  return ::grpc::BlockingUnaryCall(channel_.get(), rpcmethod_queryItemInfo_, context, request, response);
}

::grpc::ClientAsyncResponseReader< ::module::protocol::ItemInfo>* PrimaryElection::Stub::AsyncqueryItemInfoRaw(::grpc::ClientContext* context, const ::module::protocol::ItemQuery& request, ::grpc::CompletionQueue* cq) {
  return new ::grpc::ClientAsyncResponseReader< ::module::protocol::ItemInfo>(channel_.get(), cq, rpcmethod_queryItemInfo_, context, request);
}

::grpc::Status PrimaryElection::Stub::queryUserStatus(::grpc::ClientContext* context, const ::module::protocol::User& request, ::module::protocol::UserStatus* response) {
  return ::grpc::BlockingUnaryCall(channel_.get(), rpcmethod_queryUserStatus_, context, request, response);
}

::grpc::ClientAsyncResponseReader< ::module::protocol::UserStatus>* PrimaryElection::Stub::AsyncqueryUserStatusRaw(::grpc::ClientContext* context, const ::module::protocol::User& request, ::grpc::CompletionQueue* cq) {
  return new ::grpc::ClientAsyncResponseReader< ::module::protocol::UserStatus>(channel_.get(), cq, rpcmethod_queryUserStatus_, context, request);
}

::grpc::Status PrimaryElection::Stub::queryCandidateSet(::grpc::ClientContext* context, const ::module::protocol::Recommend& request, ::module::protocol::CandidateSet* response) {
  return ::grpc::BlockingUnaryCall(channel_.get(), rpcmethod_queryCandidateSet_, context, request, response);
}

::grpc::ClientAsyncResponseReader< ::module::protocol::CandidateSet>* PrimaryElection::Stub::AsyncqueryCandidateSetRaw(::grpc::ClientContext* context, const ::module::protocol::Recommend& request, ::grpc::CompletionQueue* cq) {
  return new ::grpc::ClientAsyncResponseReader< ::module::protocol::CandidateSet>(channel_.get(), cq, rpcmethod_queryCandidateSet_, context, request);
}

::grpc::Status PrimaryElection::Stub::updateAction(::grpc::ClientContext* context, const ::module::protocol::Action& request, ::module::protocol::StatusCode* response) {
  return ::grpc::BlockingUnaryCall(channel_.get(), rpcmethod_updateAction_, context, request, response);
}

::grpc::ClientAsyncResponseReader< ::module::protocol::StatusCode>* PrimaryElection::Stub::AsyncupdateActionRaw(::grpc::ClientContext* context, const ::module::protocol::Action& request, ::grpc::CompletionQueue* cq) {
  return new ::grpc::ClientAsyncResponseReader< ::module::protocol::StatusCode>(channel_.get(), cq, rpcmethod_updateAction_, context, request);
}

::grpc::Status PrimaryElection::Stub::deleteUserDislike(::grpc::ClientContext* context, const ::module::protocol::UserProfileFieldKey& request, ::module::protocol::StatusCode* response) {
  return ::grpc::BlockingUnaryCall(channel_.get(), rpcmethod_deleteUserDislike_, context, request, response);
}

::grpc::ClientAsyncResponseReader< ::module::protocol::StatusCode>* PrimaryElection::Stub::AsyncdeleteUserDislikeRaw(::grpc::ClientContext* context, const ::module::protocol::UserProfileFieldKey& request, ::grpc::CompletionQueue* cq) {
  return new ::grpc::ClientAsyncResponseReader< ::module::protocol::StatusCode>(channel_.get(), cq, rpcmethod_deleteUserDislike_, context, request);
}

PrimaryElection::AsyncService::AsyncService() : ::grpc::AsynchronousService(PrimaryElection_method_names, 6) {}

PrimaryElection::Service::Service() {
}

PrimaryElection::Service::~Service() {
}

::grpc::Status PrimaryElection::Service::queryUserInfo(::grpc::ServerContext* context, const ::module::protocol::UserQuery* request, ::module::protocol::UserInfo* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

void PrimaryElection::AsyncService::RequestqueryUserInfo(::grpc::ServerContext* context, ::module::protocol::UserQuery* request, ::grpc::ServerAsyncResponseWriter< ::module::protocol::UserInfo>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
  AsynchronousService::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
}

::grpc::Status PrimaryElection::Service::queryItemInfo(::grpc::ServerContext* context, const ::module::protocol::ItemQuery* request, ::module::protocol::ItemInfo* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

void PrimaryElection::AsyncService::RequestqueryItemInfo(::grpc::ServerContext* context, ::module::protocol::ItemQuery* request, ::grpc::ServerAsyncResponseWriter< ::module::protocol::ItemInfo>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
  AsynchronousService::RequestAsyncUnary(1, context, request, response, new_call_cq, notification_cq, tag);
}

::grpc::Status PrimaryElection::Service::queryUserStatus(::grpc::ServerContext* context, const ::module::protocol::User* request, ::module::protocol::UserStatus* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

void PrimaryElection::AsyncService::RequestqueryUserStatus(::grpc::ServerContext* context, ::module::protocol::User* request, ::grpc::ServerAsyncResponseWriter< ::module::protocol::UserStatus>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
  AsynchronousService::RequestAsyncUnary(2, context, request, response, new_call_cq, notification_cq, tag);
}

::grpc::Status PrimaryElection::Service::queryCandidateSet(::grpc::ServerContext* context, const ::module::protocol::Recommend* request, ::module::protocol::CandidateSet* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

void PrimaryElection::AsyncService::RequestqueryCandidateSet(::grpc::ServerContext* context, ::module::protocol::Recommend* request, ::grpc::ServerAsyncResponseWriter< ::module::protocol::CandidateSet>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
  AsynchronousService::RequestAsyncUnary(3, context, request, response, new_call_cq, notification_cq, tag);
}

::grpc::Status PrimaryElection::Service::updateAction(::grpc::ServerContext* context, const ::module::protocol::Action* request, ::module::protocol::StatusCode* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

void PrimaryElection::AsyncService::RequestupdateAction(::grpc::ServerContext* context, ::module::protocol::Action* request, ::grpc::ServerAsyncResponseWriter< ::module::protocol::StatusCode>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
  AsynchronousService::RequestAsyncUnary(4, context, request, response, new_call_cq, notification_cq, tag);
}

::grpc::Status PrimaryElection::Service::deleteUserDislike(::grpc::ServerContext* context, const ::module::protocol::UserProfileFieldKey* request, ::module::protocol::StatusCode* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

void PrimaryElection::AsyncService::RequestdeleteUserDislike(::grpc::ServerContext* context, ::module::protocol::UserProfileFieldKey* request, ::grpc::ServerAsyncResponseWriter< ::module::protocol::StatusCode>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
  AsynchronousService::RequestAsyncUnary(5, context, request, response, new_call_cq, notification_cq, tag);
}

::grpc::RpcService* PrimaryElection::Service::service() {
  if (service_) {
    return service_.get();
  }
  service_ = std::unique_ptr< ::grpc::RpcService>(new ::grpc::RpcService());
  service_->AddMethod(new ::grpc::RpcServiceMethod(
      PrimaryElection_method_names[0],
      ::grpc::RpcMethod::NORMAL_RPC,
      new ::grpc::RpcMethodHandler< PrimaryElection::Service, ::module::protocol::UserQuery, ::module::protocol::UserInfo>(
          std::mem_fn(&PrimaryElection::Service::queryUserInfo), this)));
  service_->AddMethod(new ::grpc::RpcServiceMethod(
      PrimaryElection_method_names[1],
      ::grpc::RpcMethod::NORMAL_RPC,
      new ::grpc::RpcMethodHandler< PrimaryElection::Service, ::module::protocol::ItemQuery, ::module::protocol::ItemInfo>(
          std::mem_fn(&PrimaryElection::Service::queryItemInfo), this)));
  service_->AddMethod(new ::grpc::RpcServiceMethod(
      PrimaryElection_method_names[2],
      ::grpc::RpcMethod::NORMAL_RPC,
      new ::grpc::RpcMethodHandler< PrimaryElection::Service, ::module::protocol::User, ::module::protocol::UserStatus>(
          std::mem_fn(&PrimaryElection::Service::queryUserStatus), this)));
  service_->AddMethod(new ::grpc::RpcServiceMethod(
      PrimaryElection_method_names[3],
      ::grpc::RpcMethod::NORMAL_RPC,
      new ::grpc::RpcMethodHandler< PrimaryElection::Service, ::module::protocol::Recommend, ::module::protocol::CandidateSet>(
          std::mem_fn(&PrimaryElection::Service::queryCandidateSet), this)));
  service_->AddMethod(new ::grpc::RpcServiceMethod(
      PrimaryElection_method_names[4],
      ::grpc::RpcMethod::NORMAL_RPC,
      new ::grpc::RpcMethodHandler< PrimaryElection::Service, ::module::protocol::Action, ::module::protocol::StatusCode>(
          std::mem_fn(&PrimaryElection::Service::updateAction), this)));
  service_->AddMethod(new ::grpc::RpcServiceMethod(
      PrimaryElection_method_names[5],
      ::grpc::RpcMethod::NORMAL_RPC,
      new ::grpc::RpcMethodHandler< PrimaryElection::Service, ::module::protocol::UserProfileFieldKey, ::module::protocol::StatusCode>(
          std::mem_fn(&PrimaryElection::Service::deleteUserDislike), this)));
  return service_.get();
}


}  // namespace module
}  // namespace protocol

