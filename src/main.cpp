//
//  main.cpp
//  CollaborativeFiltering
//
//  Created by zhanghl on 14-9-3.
//  Copyright (c) 2014å¹´ CrystalBall. All rights reserved.
//

#include "CF_framework_center.h"
#include "CF_framework_module.h"

#include "Candidate.h"

enum ErrorType
{
    TYPE_OK = 0,    //ÔËÐÐÕý³£
    TYPE_NETWORK,   //ÍøÂç¹ÊÕÏ
    TYPE_SERVICE,   //·þÎñ´íÎó
    TYPE_OTHER,     //ÆäËü´íÎó
};

enum ErrorLevel
{
    LEVEL_A = 1,    //ÑÏÖØ
    LEVEL_B,        //ÖØÒª
    LEVEL_C,        //Ò»°ã
    LEVEL_D,        //µ÷ÊÔ
    LEVEL_E,        //¿ÉºöÂÔ
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
