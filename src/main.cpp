//
//  main.cpp
//  CollaborativeFiltering
//
//  Created by zhanghl on 14-9-3.
//  Copyright (c) 2014骞� CrystalBall. All rights reserved.
//

#include "CF_framework_center.h"
#include "CF_framework_module.h"
#include "CF_candidate.h"

enum ErrorType
{
  TYPE_OK = 0,    //运行正常
  TYPE_NETWORK,   //网络故障
  TYPE_SERVICE,   //服务错误
  TYPE_OTHER,     //其它错误
};

enum ErrorLevel
{
  LEVEL_A = 1,    //严重
  LEVEL_B,        //重要
  LEVEL_C,        //一般
  LEVEL_D,        //调试
  LEVEL_E,        //可忽略
};

var_4 main(var_4 argc, var_1* argv[])
{
  Candidate candidate;

  CF_framework_module cd_model;
  if(cd_model.init_framework((CF_framework_interface*)&candidate))
    return -1;

  printf("candidate starting successfully"); 

  CP_SOCKET_T lis_sock;
  var_4 ret = cp_listen_socket(lis_sock, 19001);
  if (ret)
  {   
    printf("listen monitor error\n");
    return -2; 
  }   
  var_1 monitor_buffer[1024];

  for (;;)
  {   
    CP_SOCKET_T sock;
    ret = cp_accept_socket(lis_sock, sock);
    if (ret)
    {   
      printf("monitor failed to accept");
      continue;                                                                                                      
    }                                                                                                                  
    cp_set_overtime(sock, 5000);                                                                                       

    ret = cp_recvbuf(sock, monitor_buffer, 4);                                                                         
    if (ret)                                                                                                           
    {                                                                                                                  
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
    if (ret)
    {
      printf("monitor failed to send error code[%d]\n", errno);
      continue;
    }
  }
  return 0;
}
