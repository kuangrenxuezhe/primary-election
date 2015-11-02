//
//  UC_RunManager.h
//  code_library
//
//  Created by zhanghl on 12-11-1.
//  Copyright (c) 2012年 zhanghl. All rights reserved.
//

#ifndef _UC_RUNMANAGER_H_
#define _UC_RUNMANAGER_H_

#include "UH_Define.h"
#include "UT_Queue.h"

class UC_RunManager
{
public:
    //////////////////////////////////////////////////////////////////////
    // 函数:      init
    // 功能:      初始化通讯服务端
    // 入参:
    //                   work_num: 工作线程数量
    //                 queue_size: 队列长度
    //                listen_port: 连接的超时时间
    //                 listen_num: 监听线程数量
    //                   time_out: 要连接的服务器端口
    //                 model_type: 通讯模式 1 - 单次收发 0 - 多次收发    
    //                  head_size: 协议头长度
    //              max_send_size: 最大发送长度
    //              min_recv_size: 最小接收长度
    //              max_recv_size: 最大接收长度
    //
    // 出参:
    // 返回值:    成功返回0,否则返回错误码
    // 备注:      mode_type参数为1(单次收发)的时候 通讯类会调用cs_fun_package解析包头,之后会调用cs_fun_process处理接收到的数据包
    //           如果模式为0则接收到连接请求后直接调用cs_fun_process,不会再调用cs_fun_package,但是无论那种模式,都不需要在处理函数中关闭句柄
    //////////////////////////////////////////////////////////////////////    
    var_4 init(var_4 worker_num, var_4 queue_size, var_u2 listen_port, var_4 listen_num, var_4 time_out, var_4 model_type = 0, var_4 head_size = 0, var_4 max_send_size = 0, var_4 min_recv_size = 1048576, var_4 max_recv_size = 10485760)
    {
        m_worker_num = worker_num;
        m_queue_size = queue_size;
        
        m_listen_port = listen_port;
        m_listen_num = listen_num;
        m_time_out = time_out;
        m_head_size = head_size;
        m_max_send_size = max_send_size;
        m_model_type = model_type;
        m_min_recv_size = min_recv_size;
        m_max_recv_size = max_recv_size;
                
        if(cp_init_socket())
        {
            printf("UC_RunManager.init init socket error\n");
            return -1;
        }
        
        if(cp_listen_socket(m_listen, m_listen_port))
        {
            printf("UC_RunManager.init listen %d error\n", m_listen_port);
            return -1;
        }
        
        if(m_queue.InitQueue(m_queue_size))
        {
            printf("UC_RunManager.init init queue error, size = %d\n", m_queue_size);
            return -1;
        }
        
        return 0;
    }
    
    //////////////////////////////////////////////////////////////////////
    // 函数:      start
    // 功能:      启动服务
    // 入参:
    //
    // 出参:
    // 返回值:    成功返回0,否则返回错误码
    // 备注:
    //////////////////////////////////////////////////////////////////////
    var_4 start()
    {
        for(var_4 i = 0; i < m_worker_num; i++)
        {
            m_success_flag = 0;
            
            if(cp_create_thread(rm_worker_thread, this))
                return -1;
            
            while(m_success_flag == 0)
                cp_sleep(1);
            
            if(m_success_flag < 0)
                return -1;
        }
        
        for(var_4 i = 0; i < m_listen_num; i++)
        {
            m_success_flag = 0;
            
            if(cp_create_thread(rm_accept_thread, this))
                return -1;
            
            while(m_success_flag == 0)
                cp_sleep(1);
            
            if(m_success_flag < 0)
                return -1;
        }

        return 0;
    }

    //////////////////////////////////////////////////////////////////////
    // 函数:      rm_fun_package
    // 功能:      继承类必须实现的包头解析函数
    // 入参:
    //                  in_buf: 接收到的包头
    //             in_buf_size: 接收到的包头长度
    //                  handle: 保留
    //
    // 出参:
    // 返回值:    成功返回后续需要接收的长度>=0,失败返回错误码<0
    // 备注:      此函数在模式0中不会被调用,子类可以实现个空函数
    //////////////////////////////////////////////////////////////////////
    virtual var_4 rm_fun_package(const var_1* in_buf, const var_4 in_buf_size, const var_vd* handle = NULL) = 0;
    
    //////////////////////////////////////////////////////////////////////
    // 函数:      rm_fun_process
    // 功能:      继承类必须实现的协议处理函数
    // 入参:
    //                  in_buf: 接收到的包内容
    //             in_buf_size: 接收到的包内容长度
    //                 out_buf: 发送使用的缓冲区
    //            out_buf_size: 发送使用的缓冲区大小
    //                  handle: 连接句柄,只有在模式0中用到
    //
    // 出参:
    // 返回值:    成功返回要发送的长度>=0,否则返回错误码<0
    // 备注:
    //////////////////////////////////////////////////////////////////////
    virtual var_4 rm_fun_process(const var_1* in_buf, const var_4 in_buf_size, const var_1* out_buf, const var_4 out_buf_size, const var_vd* handle = NULL) = 0;

private:
    static CP_THREAD_T rm_worker_thread(var_vd* argv)
    {
        UC_RunManager* rm = (UC_RunManager*)argv;
        
        var_1* recv_buf = NULL;
        var_4  recv_len = 0;
        var_1* send_buf = NULL;
        var_4  send_len = 0;
        var_1* temp_buf = NULL;
        
        if(rm->m_model_type == 1)
        {
            recv_buf = new var_1[rm->m_min_recv_size];
            if(recv_buf == NULL)
            {
                rm->m_success_flag = -1;
                return NULL;
            }
            
            if(rm->m_max_send_size > 0)
            {
                send_buf = new var_1[rm->m_max_send_size];
                if(send_buf == NULL)
                {
                    rm->m_success_flag = -1;
                    return NULL;
                }
            }
        }
        
        rm->m_success_flag = 1;
        
        for(;;)
        {
            CP_SOCKET_T socket = rm->m_queue.PopData();
            
            if(rm->m_model_type == 1)
            {
                if(temp_buf)
                {
                    if(recv_buf)
                        delete recv_buf;
                    recv_buf = temp_buf;
                    temp_buf = NULL;
                }
                
                if(cp_recvbuf(socket, recv_buf, rm->m_head_size))
                {
                    cp_close_socket(socket);
                    
                    printf("UC_RunManager.rm_worker_thread - recv head error, size = %d\n", rm->m_head_size);;
                    continue;
                }
                
                recv_len = rm->rm_fun_package(recv_buf, rm->m_head_size);
                if(recv_len < 0)
                {
                    cp_close_socket(socket);
                    
                    printf("UC_RunManager.rm_worker_thread - cs_fun_package error, code = %d\n", recv_len);
                    continue;
                }
                if(recv_len > 0)
                {
                    if(recv_len > rm->m_min_recv_size)
                    {
                        if(recv_len > rm->m_max_recv_size)
                        {
                            cp_close_socket(socket);
                            
                            printf("UC_RunManager.rm_worker_thread - recv length(%d) > max recv length(%d)\n", recv_len, rm->m_max_recv_size);
                            continue;
                        }
                        
                        temp_buf = recv_buf;
                        recv_buf = new var_1[recv_len];
                        if(recv_buf == NULL)
                        {
                            cp_close_socket(socket);
                            
                            printf("UC_RunManager.rm_worker_thread - alloc recv memory error, size = %d)\n", recv_len);
                            continue;
                        }
                    }
                    
                    if(cp_recvbuf(socket, recv_buf, recv_len))
                    {
                        cp_close_socket(socket);
                        
                        printf("UC_RunManager.rm_worker_thread - recv body error, size = %d)\n", recv_len);
                        continue;
                    }
                }
                
                send_len = rm->rm_fun_process(recv_buf, recv_len, send_buf, rm->m_max_send_size);
                if(send_len < 0)
                {
                    cp_close_socket(socket);
                    
                    printf("UC_RunManager.rm_worker_thread - cs_fun_process error, code = %d\n", send_len);
                    continue;
                }
                if(send_len > 0)
                {
                    if(cp_sendbuf(socket, send_buf, send_len))
                    {
                        cp_close_socket(socket);
                        
                        printf("UC_RunManager.rm_worker_thread - send buffer error, size = %d)\n", send_len);
                        continue;
                    }
                }
            }
            else
            {
                rm->rm_fun_process(recv_buf, recv_len, send_buf, send_len, (var_vd*)&socket);
            }
            
            cp_close_socket(socket);
        }
        
        return 0;
    }

    static CP_THREAD_T rm_accept_thread(var_vd* argv)
    {
        UC_RunManager* rm = (UC_RunManager*)argv;
        
        rm->m_success_flag = 1;
        
        for(;;)
        {
            CP_SOCKET_T socket;
            
            if(cp_accept_socket(rm->m_listen, socket))
            {
                cp_sleep(1);
                continue;
            }
            
            if(cp_set_overtime(socket, rm->m_time_out))
            {
                cp_close_socket(socket);
                
                printf("UC_RunManager.rm_accept_thread - set time out error, value = %d\n", rm->m_time_out);
                continue;
            }

            rm->m_queue.PushData(socket);
        }
        
        return 0;
    }
    
public:
    UT_Queue<CP_SOCKET_T> m_queue;
    
    var_4 m_worker_num;
    var_4 m_queue_size;

    var_u2 m_listen_port;
    var_4  m_listen_num;
    var_4  m_time_out;
    var_4  m_head_size;
    var_4  m_max_send_size;
    var_4  m_model_type;
    var_4  m_min_recv_size;
    var_4  m_max_recv_size;
    
    CP_SOCKET_T m_listen;
    
    var_4  m_success_flag;
};

#endif // _UC_RUNMANAGER_H_
