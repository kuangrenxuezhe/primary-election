//
//  UC_Communication.h
//  code_library
//
//  Created by zhanghl on 12-11-1.
//  Copyright (c) 2012年 zhanghl. All rights reserved.
//

#ifndef _UC_COMMUNICATION_H_
#define _UC_COMMUNICATION_H_

#include "UH_Define.h"

class UC_Communication_Client
{
public:
    //////////////////////////////////////////////////////////////////////
    // 函数:      init
    // 功能:      初始化通讯客户端
    // 入参:
    //                  server_ip: 要连接的服务器IP
    //                server_port: 要连接的服务器端口
    //                   time_out: 连接的超时时间
    //
    // 出参:
    // 返回值:    成功返回0,否则返回错误码
    // 备注:
    //////////////////////////////////////////////////////////////////////
    var_4 init(const var_1* server_ip, const var_u2 server_port, const var_4 time_out)
    {
        strcpy(m_server_ip, server_ip);
        m_server_port = server_port;
        m_time_out = time_out;
        
        if(cp_init_socket())
        {
            printf("UC_Communication_Client.init init socket error\n");
            return -1;
        }

        return 0;
    }
    
    //////////////////////////////////////////////////////////////////////
    // 函数:      open
    // 功能:      创建到指定服务器的连接
    // 入参:
    //                  server_ip: 要连接的服务器IP
    //                server_port: 要连接的服务器端口
    //                   time_out: 连接的超时时间
    //
    // 出参:
    //                     handle: 连接句柄
    //
    // 返回值:    成功返回0,否则返回错误码
    // 备注:     time_out, server_ip, server_port 三个参数如果不指定则使用初始化时传入的参数，任意一个指定则使用当前指定的值
    //////////////////////////////////////////////////////////////////////
    var_4 open(var_vd*& handle, var_1* server_ip = NULL, var_u2 server_port = 0, var_4 time_out = -1)
    {
        CP_SOCKET_T* socket = new CP_SOCKET_T;
        
        if(server_ip == NULL)
            server_ip = m_server_ip;
        if(server_port == 0)
            server_port = m_server_port;
        if(time_out == -1)
            time_out = m_time_out;

        if(cp_connect_socket(*socket, server_ip, server_port))
        {
            printf("UC_Communication_Client.open connect %s:%d error\n", server_ip, server_port);
            return -1;
        }
        
        if(cp_set_overtime(*socket, time_out))
        {
            printf("UC_Communication_Client.open set overtime %d error\n", time_out);
            return -1;
        }
        
        handle = (var_vd*)socket;
        
        return 0;
    }
    
    //////////////////////////////////////////////////////////////////////
    // 函数:      send
    // 功能:      发送数据
    // 入参:
    //                     handle: 连接句柄
    //                   send_buf: 要发送的buf
    //                   send_len: 要发送的buf长度
    //
    // 出参:
    // 返回值:    成功返回0,否则返回错误码
    // 备注:
    //////////////////////////////////////////////////////////////////////
    var_4 send(var_vd* handle, var_1* send_buf, var_4 send_len)
    {
        if(cp_sendbuf(*(CP_SOCKET_T*)handle, send_buf, send_len))
            return -1;
        
        return 0;
    }
    
    //////////////////////////////////////////////////////////////////////
    // 函数:      recv
    // 功能:      接收数据
    // 入参:
    //                     handle: 连接句柄
    //                   recv_buf: 要接收的buf
    //                   recv_len: 要接收的buf长度
    //
    // 出参:
    // 返回值:    成功返回0,否则返回错误码
    // 备注:
    //////////////////////////////////////////////////////////////////////
    var_4 recv(var_vd* handle, var_1* recv_buf, var_4 recv_len)
    {
        if(cp_recvbuf(*(CP_SOCKET_T*)handle, recv_buf, recv_len))
            return -1;
        
        return 0;
    }
    
    //////////////////////////////////////////////////////////////////////
    // 函数:      close
    // 功能:      关闭连接
    // 入参:
    //                     handle: 连接句柄
    //
    // 出参:
    // 返回值:    成功返回0,否则返回错误码
    // 备注:
    //////////////////////////////////////////////////////////////////////
    var_4 close(var_vd* handle)
    {
        cp_close_socket(*(CP_SOCKET_T*)handle);
        delete (CP_SOCKET_T*)handle;
        
        return 0;
    }
    
private:
    var_1  m_server_ip[16];
    var_u2 m_server_port;
    var_4  m_time_out;
};

class UC_Communication_Server
{
public:
    //////////////////////////////////////////////////////////////////////
    // 函数:      init
    // 功能:      初始化通讯服务端
    // 入参:
    //                listen_port: 要连接的服务器端口
    //                 listen_num: 监听线程数
    //                   time_out: 连接的超时时间(单位ms)
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
    var_4 init(var_u2 listen_port, var_4 listen_num, var_4 time_out, var_4 model_type = 0, var_4 head_size = 0, var_4 max_send_size = 0, var_4 min_recv_size = 1048576, var_4 max_recv_size = 10485760)
    {
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
            printf("UC_Communication_Server.init init socket error\n");
            return -1;
        }
        
        if(cp_listen_socket(m_listen, m_listen_port))
        {
            printf("UC_Communication_Server.init listen %d error\n", m_listen_port);
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
        for(var_4 i = 0; i < m_listen_num; i++)
        {
            m_success_flag = 0;
            
            if(cp_create_thread(cs_daemon_thread, this))
                return -1;
            
            while(m_success_flag == 0)
                cp_sleep(1);
            
            if(m_success_flag < 0)
                return -1;
        }
        
        return 0;
    }

    //////////////////////////////////////////////////////////////////////
    // 函数:      send
    // 功能:      发送数据
    // 入参:
    //                     handle: 连接句柄
    //                   send_buf: 要发送的buf
    //                   send_len: 要发送的buf长度
    //
    // 出参:
    // 返回值:    成功返回0,否则返回错误码
    // 备注:
    //////////////////////////////////////////////////////////////////////
    var_4 send(var_vd* handle, var_1* send_buf, var_4 send_len)
    {
        if(cp_sendbuf(*(CP_SOCKET_T*)handle, send_buf, send_len))
            return -1;
        
        return 0;
    }

    //////////////////////////////////////////////////////////////////////
    // 函数:      send_file
    // 功能:      发送文件
    // 入参:
    //                     handle: 连接句柄
    //                  file_name: 要发送的文件
    //
    // 出参:
    // 返回值:    成功返回0,否则返回错误码
    // 备注:
    //////////////////////////////////////////////////////////////////////
    var_4 send_file(var_vd* handle, var_1* file_name)
    {
        if(cp_sendfile(*(CP_SOCKET_T*)handle, file_name))
            return -1;
        
        return 0;
    }

    //////////////////////////////////////////////////////////////////////
    // 函数:      recv
    // 功能:      接收数据
    // 入参:
    //                     handle: 连接句柄
    //                   recv_buf: 要接收的buf
    //                   recv_len: 要接收的buf长度
    //
    // 出参:
    // 返回值:    成功返回0,否则返回错误码
    // 备注:
    //////////////////////////////////////////////////////////////////////
    var_4 recv(var_vd* handle, var_1* recv_buf, var_4 recv_len)
    {
        if(cp_recvbuf(*(CP_SOCKET_T*)handle, recv_buf, recv_len))
            return -1;
        
        return 0;
    }

    //////////////////////////////////////////////////////////////////////
    // 函数:      close
    // 功能:      关闭连接
    // 入参:
    //                     handle: 连接句柄
    //
    // 出参:
    // 返回值:    成功返回0,否则返回错误码
    // 备注:
    //////////////////////////////////////////////////////////////////////
    var_4 close(var_vd* handle)
    {
        cp_close_socket(*(CP_SOCKET_T*)handle);
        
        return 0;
    }

    //////////////////////////////////////////////////////////////////////
    // 函数:      cs_fun_package
    // 功能:      继承类必须实现的包头解析函数
    // 入参:
    //                  in_buf: 接收到的包头
    //             in_buf_size: 接收到的包头长度
    //                  handle: 保留
    //
    // 出参:
    // 返回值:    成功返回后续需要接收的长度>0,失败返回错误码<0,如果返回=0则后续不再接收,直接调用cs_fun_process
    // 备注:      此函数在模式0中不会被调用,子类可以实现个空函数
    //////////////////////////////////////////////////////////////////////
    virtual var_4 cs_fun_package(const var_1* in_buf, const var_4 in_buf_size, const var_vd* handle = NULL) = 0;
    
    //////////////////////////////////////////////////////////////////////
    // 函数:      cs_fun_process
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
    virtual var_4 cs_fun_process(const var_1* in_buf, const var_4 in_buf_size, var_1* const out_buf, const var_4 out_buf_size, const var_vd* handle = NULL) = 0;
    
private:
    static CP_THREAD_T cs_daemon_thread(var_vd* argv)
    {
        UC_Communication_Server* cs = (UC_Communication_Server*)argv;
        
        var_1* recv_buf = NULL;
        var_4  recv_len = 0;
        var_1* send_buf = NULL;
        var_4  send_len = 0;
        var_1* temp_buf = NULL;
        
        if(cs->m_model_type == 1)
        {
            recv_buf = new var_1[cs->m_min_recv_size];
            if(recv_buf == NULL)
            {
                cs->m_success_flag = -1;
                return NULL;
            }
            
            if(cs->m_max_send_size > 0)
            {
                send_buf = new var_1[cs->m_max_send_size];
                if(send_buf == NULL)
                {
                    cs->m_success_flag = -1;
                    return NULL;
                }
            }
        }
        
        cs->m_success_flag = 1;
        
        for(;;)
        {
            CP_SOCKET_T socket;
            
            if(cp_accept_socket(cs->m_listen, socket))
            {
                cp_sleep(1);
                continue;
            }
            
            if(cp_set_overtime(socket, cs->m_time_out))
            {
                cp_close_socket(socket);
                
                printf("UC_Communication_Server.cs_daemon_thread - set time out error, value = %d\n", cs->m_time_out);
                continue;
            }
            
            if(cs->m_model_type == 1)
            {
                if(temp_buf)
                {
                    if(recv_buf)
                        delete recv_buf;
                    recv_buf = temp_buf;
                    temp_buf = NULL;
                }
                
                if(cp_recvbuf(socket, recv_buf, cs->m_head_size))
                {
                    cp_close_socket(socket);
                    
                    printf("UC_Communication_Server.cs_daemon_thread - recv head error, size = %d\n", cs->m_head_size);
                    continue;
                }
                
                recv_len = cs->cs_fun_package(recv_buf, cs->m_head_size);
                if(recv_len < 0)
                {
                    cp_close_socket(socket);
                    
                    printf("UC_Communication_Server.cs_daemon_thread - cs_fun_package error, code = %d\n", recv_len);
                    continue;
                }
                if(recv_len > 0)
                {
                    if(recv_len > cs->m_min_recv_size)
                    {
                        if(recv_len > cs->m_max_recv_size)
                        {
                            cp_close_socket(socket);
                            
                            printf("UC_Communication_Server.cs_daemon_thread - recv length(%d) > max recv length(%d)\n", recv_len, cs->m_max_recv_size);
                            continue;
                        }
                        
                        temp_buf = recv_buf;
                        recv_buf = new var_1[recv_len];
                        if(recv_buf == NULL)
                        {
                            cp_close_socket(socket);
                            
                            printf("UC_Communication_Server.cs_daemon_thread - alloc recv memory error, size = %d)\n", recv_len);
                            continue;
                        }
                    }
                    
                    if(cp_recvbuf(socket, recv_buf, recv_len))
                    {
                        cp_close_socket(socket);
                        
                        printf("UC_Communication_Server.cs_daemon_thread - recv body error, size = %d)\n", recv_len);
                        continue;
                    }
                }
                
                send_len = cs->cs_fun_process(recv_buf, recv_len, send_buf, cs->m_max_send_size);
                if(send_len < 0)
                {
                    cp_close_socket(socket);
                    
                    printf("UC_Communication_Server.cs_daemon_thread - cs_fun_process error, code = %d\n", send_len);
                    continue;
                }
                if(send_len > 0)
                {
                    if(cp_sendbuf(socket, send_buf, send_len))
                    {
                        cp_close_socket(socket);
                        
                        printf("UC_Communication_Server.cs_daemon_thread - send buffer error, size = %d)\n", send_len);
                        continue;
                    }
                }
            }
            else
            {
                cs->cs_fun_process(recv_buf, recv_len, send_buf, send_len, (var_vd*)&socket);
            }
            
            cp_close_socket(socket);
        }
        
        return 0;
    }
    
public:
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

#endif // _UC_COMMUNICATION_H_
