//
//  main.cpp
//  CollaborativeFiltering
//
//  Created by zhanghl on 14-9-3.
//  Copyright (c) 2014 CrystalBall. All rights reserved.
//
#include <pthread.h>
#include <grpc++/grpc++.h>

#include "utils/char_conv.h"
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "core/candidate_db.h"
#include "service/service_glue.h"
#include "service/service_grpc.h"
#include "framework/CF_framework_center.h"
#include "framework/CF_framework_module.h"

enum ErrorType {
  TYPE_OK = 0,    //运行正常
  TYPE_NETWORK,   //网络故障
  TYPE_SERVICE,   //服务错误
  TYPE_OTHER,     //其它错误
};

enum ErrorLevel {
  LEVEL_A = 1,    //严重
  LEVEL_B,        //重要
  LEVEL_C,        //一般
  LEVEL_D,        //调试
  LEVEL_E,        //可忽略
};

using namespace souyue::recmd;

struct arg_ {
  ModelOptions*   options;
  CandidateDB* candb;
};
typedef struct arg_ arg_t;

pthread_t grpc_;
void* service_grpc(void* args);
DEFINE_string(conf, "conf/candb.conf", "Candidate config file");
DEFINE_int32(monitor_port, -1, "Monitor port");

// 便于在Linux环境下调试GBK编码输出字符
// // usage: call printGBK(var)
void printGBK(const char *pstr);
void printGBK(const std::string &pstr);

int main(int argc, char* argv[])
{
  ModelOptions opts; 
  CandidateDB* candb;
  Status status = Status::OK();

  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  status = ModelOptions::fromConf(FLAGS_conf, opts);
  if (!status.ok()) {
    LOG(FATAL) << status.toString(); 
    return -1;
  }

  status = CandidateDB::openDB(opts, &candb); 
  if (!status.ok()) {
    LOG(FATAL) << status.toString();
    return -1;
  }
  arg_t arg = {&opts, candb};
  pthread_create(&grpc_, NULL, service_grpc, &arg);

  ServiceGlue serv_glue(candb);
  CF_framework_module cd_model;
  if(cd_model.init_framework((CF_framework_interface*)&serv_glue)) {
    LOG(FATAL) << "Framework start failed";
    return -1;
  }
  LOG(INFO) << "Candidate db starting successfully"; 

  // 支持可以从参数来指定监控端口
  int monitor_port = FLAGS_monitor_port;
  if (monitor_port <= 0)
    monitor_port = opts.monitor_port;

  //===================monitor=====================================
  CP_SOCKET_T lis_sock;
  var_4 ret = cp_listen_socket(lis_sock, monitor_port);
  if (ret) {   
    printf("listen monitor error\n");
    return -2; 
  }   
  var_1 monitor_buffer[1024];

  for (;;) {   
    CP_SOCKET_T sock;
    ret = cp_accept_socket(lis_sock, sock);
    if (ret) {   
      printf("monitor failed to accept");
      continue;                                                                                                      
    }                                                                                                                  
    cp_set_overtime(sock, 5000);                                                                                       

    ret = cp_recvbuf(sock, monitor_buffer, 4);                                                                         
    if (ret) {
      printf("monitor failed to recv error code[%d]\n", errno);                                           
      cp_close_socket(sock);                                                                                         
      continue;                                                                                                      
    }                                                                                                                  

    var_1* pos = monitor_buffer;                                                                                       
    memcpy(pos, "MonitorP1", 10);                                                                                      
    pos += 10 + 4;                                                                                                     
    *(var_4*)pos = TYPE_OK;                                                                                            
    pos += 4;                                                                                                          
    *(var_4*)pos = LEVEL_B;                                                                                            
    pos += 4;                                                                                                          

    *(var_4*)(monitor_buffer + 10) = pos - monitor_buffer - 14;                                                        
    ret = cp_sendbuf(sock, monitor_buffer, pos - monitor_buffer); 
    cp_close_socket(sock);
    if (ret) {
      printf("monitor failed to send error code[%d]\n", errno);
      continue;
    }
  }
  return 0;
}

using namespace grpc;
void* service_grpc(void* args)
{
  arg_t* arg = (arg_t*)args;
  ServiceGrpc grpc(arg->candb);
  ServerBuilder builder;

  char address[300];

  sprintf(address, "0.0.0.0:%d", arg->options->rpc_port);
  builder.AddListeningPort(std::string(address), grpc::InsecureServerCredentials());
  builder.RegisterService(&grpc);

  std::unique_ptr<Server> server(builder.BuildAndStart());
  LOG(INFO) << "GRPC Server listening on:" << arg->options->rpc_port;

  server->Wait();
  return NULL;
}

void printGBK(const std::string &pstr)
{
  CharConv cc("UTF-8", "GBK");
  fprintf(stderr, "%s\n", cc.charConv(pstr).c_str());
}

void printGBK(const char *pstr)
{
  std::string str(pstr);
  printGBK(str);
}


