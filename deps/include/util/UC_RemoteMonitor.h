//
//  UC_RemoteMonitor.h
//  code_library
//
//  Created by zhanghl on 12-12-5.
//  Copyright (c) 2012年 zhanghl. All rights reserved.
//

#ifndef _UC_REMOTEMONITOR_H
#define _UC_REMOTEMONITOR_H

#include "UH_Define.h"

class UC_RemoteMonitor
{
public:
    var_4 init(var_u2 monitor_port, var_4 (*fun_state)(var_vd* argv), var_vd* fun_argv, var_4 is_display_log = 1)
    {
        m_monitor_port = monitor_port;
        
        m_fun_state = fun_state;
        m_fun_argv = fun_argv;
        
        m_is_display_log = is_display_log;
        
        if(cp_init_socket())
            return -1;
        if(cp_listen_socket(m_listen, m_monitor_port))
            return -1;
        
        if(cp_create_thread(thread_monitor, this))
            return -1;
        
        return 0;
    }

    static CP_THREAD_T thread_monitor(var_vd* argv)
    {
        enum ErrorType {
            TYPE_OK = 0, //运行正常
            TYPE_MONITOR, //monitor错误
            TYPE_NETWORK, //网络故障
            TYPE_SERVICE, //服务错误
            TYPE_OTHER, //其它错误
        };
        enum ErrorLevel {
            LEVEL_A = 1, //严重
            LEVEL_B, //重要
            LEVEL_C, //一般
            LEVEL_D, //调试
            LEVEL_E //可忽略
        };

        UC_RemoteMonitor* rm = (UC_RemoteMonitor*)argv;
        
        var_1 buffer[256];
        
        for(;;)
        {
            CP_SOCKET_T client;
            
            if(cp_accept_socket(rm->m_listen, client))
            {
                if(rm->m_is_display_log)
                    printf("UC_RemoteMonitor: accept request error\n");
                continue;
            }
            
            cp_set_overtime(client, 1000);
            
            if(cp_recvbuf(client, buffer, 4))
            {
                if(rm->m_is_display_log)
                    printf("UC_RemoteMonitor: recv buffer error\n");

                cp_close_socket(client);
                continue;
            }
            
            var_4 state = rm->m_fun_state(rm->m_fun_argv);
            
            var_1* ptr = buffer;
            
            memcpy(ptr, "MonitorP1 ", 10);
            ptr += 10;
            *(var_4*)ptr = 0;
            ptr += 4;
            if(state == 0)
            {
                *(var_4*)ptr = TYPE_OK;
                ptr += 4;
                *(var_4*)ptr = LEVEL_B;
                ptr += 4;
            }
            else
            {
                *(var_4*)ptr = TYPE_SERVICE;
                ptr += 4;
                *(var_4*)ptr = LEVEL_A;
                ptr += 4;
            }
            
            *(var_4*)(buffer + 10) = (var_4)(ptr - buffer - 14);
            
            if(cp_sendbuf(client, buffer, (var_4)(ptr - buffer)))
            {
                if(rm->m_is_display_log)
                    printf("UC_RemoteMonitor: send buffer error\n");
            }
            
            cp_close_socket(client);
            
            if(rm->m_is_display_log)
                printf("UC_RemoteMonitor: remote monitor success\n");
        }
    }
    
public:
    var_u2 m_monitor_port;
    
    var_4  (*m_fun_state)(var_vd* argv);
    var_vd*  m_fun_argv;
    
    var_4 m_is_display_log;
    
    CP_SOCKET_T m_listen;
};

#endif // _UC_REMOTEMONITOR_H
