//
//  UC_CountStat.h
//  code_library
//
//  Created by zhanghl on 12-11-1.
//  Copyright (c) 2012年 zhanghl. All rights reserved.
//

#ifndef _UC_COUNT_STAT_H_
#define _UC_COUNT_STAT_H_

#include "UH_Define.h"
#include "UC_RegisterSystem.h"
#include "UC_Communication.h"
#include "UC_LogManager.h"

#define MAX_STAT_SERVER_COUNT   1024
#define MAX_STAT_COLUMN_COUNT   64
#define MAX_REGISTER_SERVER_NUM 64
#define MAX_NAME_LENGTH         64

class UC_CountStat_Client
{ 
public:
    //////////////////////////////////////////////////////////////////////
    // 函数:      init
    // 功能:      初始化统计客户端
    // 入参:
    //              interval_time: 自动发送统计数据间隔时间,ms单位
    //                 server_num: 要发送的服务器数量
    //                  server_ip: 要发送的服务器ip列表
    //                  stat_port: 要发送的服务器统计端口号列表
    //              register_port: 要发送的服务器注册端口号列表
    //                  uuid_name: 要注册到服务器的服务名称
    //                  stat_name: 每列的统计名称
    //            cur_stat_column: 当前指定的统计列数
    //            max_stat_column: 最大支持的统计列数
    //
    // 出参:
    // 返回值:    成功返回0,否则返回错误码
    // 备注:
    //////////////////////////////////////////////////////////////////////
    var_4 init(var_4 interval_time, var_4 server_num, var_1** server_ip, var_u2* stat_port, var_u2* register_port, var_1** server_name, var_1** stat_name = NULL, var_4 cur_stat_column = 0, var_4 max_stat_column = MAX_STAT_COLUMN_COUNT)
    {
        if(MAX_STAT_COLUMN_COUNT < max_stat_column)
            return -1;
        if(MAX_REGISTER_SERVER_NUM < server_num)
            return -1;
        
        m_interval_time = interval_time;
        m_server_num = server_num;
        
        for(var_4 i = 0; i < m_server_num; i++)
        {
            strcpy(m_server_ip[i], server_ip[i]);
            m_stat_port[i] = stat_port[i];
            m_register_port[i] = register_port[i];
            
            if(MAX_NAME_LENGTH < strlen(server_name[i]))
               return -1;
            strcpy(m_server_name[i], server_name[i]);            
        }

        m_cur_stat_column = cur_stat_column;
        m_max_stat_column = max_stat_column;
        
        for(var_4 i = 0; i < m_cur_stat_column; i++)
        {
            if(MAX_NAME_LENGTH < strlen(stat_name[i]))
                return -1;
            strcpy(m_stat_name[i], stat_name[i]);
            
            m_stat_value[i] = 0;
        }
 
        if(m_cc.init("0.0.0.0", 0, 0))
            return -1;

        if(m_rsc.init("0.0.0.0", 0))
            return -1;

        for(var_4 i = 0; i < m_server_num; i++)
        {
            if(m_rsc.register_to_server(m_server_name[i], (var_4)strlen(m_server_name[i]), m_server_finger + i, m_server_ip[i], m_register_port[i]) <= 0)
                return -1;
        }

        if(cp_create_thread(thread_flush_server, this))
            return -1;
        
        return 0;
    }
    
    //////////////////////////////////////////////////////////////////////
    // 函数:      get_stat_column
    // 功能:      取得当前系统统计列数
    // 入参:
    // 出参:
    // 返回值:    成功返回当前系统列数,否则返回错误码
    // 备注:
    //////////////////////////////////////////////////////////////////////
    var_4 get_stat_column()
    {
        m_locker.lock();
        var_4 cur_stat_column = m_cur_stat_column;
        m_locker.unlock();
        
        return cur_stat_column;
    }
    
    //////////////////////////////////////////////////////////////////////
    // 函数:      add_stat_column
    // 功能:      向当前系统增加新列
    // 入参:
    //                  stat_name: 新增列的统计名称
    //
    // 出参:
    // 返回值:    成功返回新增列的索引号,否则返回错误码<0
    // 备注:
    //////////////////////////////////////////////////////////////////////
    var_4 add_stat_column(var_1* stat_name)
    {
        m_locker.lock();
        
        if(m_cur_stat_column >= m_max_stat_column)
        {
            m_locker.unlock();
            return -1;
        }
        
        if(MAX_NAME_LENGTH < strlen(stat_name))
            return -1;
        strcpy(m_stat_name[m_cur_stat_column], stat_name);
        
        m_stat_value[m_cur_stat_column] = 0;
        
        var_4 cur_stat_column = m_cur_stat_column++;
        
        m_locker.unlock();

        return cur_stat_column;
    }
        
    //////////////////////////////////////////////////////////////////////
    // 函数:      stat
    // 功能:      对指定列进行自加计数
    // 入参:
    //                stat_idx_no: 要自加计数列的索引号
    //
    // 出参:
    // 返回值:    返回计数后的值
    // 备注:
    //////////////////////////////////////////////////////////////////////
    inline var_u8 add_stat_value(var_4 stat_idx_no)
    {
        return cp_add_and_fetch(m_stat_value + stat_idx_no);
    }
    
    //////////////////////////////////////////////////////////////////////
    // 函数:      get_stat_value
    // 功能:      取出指定列当前值
    // 入参:
    //                stat_idx_no: 要取得计数列的索引号
    //
    // 出参:
    // 返回值:    返回当前值
    // 备注:
    //////////////////////////////////////////////////////////////////////
    inline var_u8 get_stat_value(var_4 stat_idx_no)
    {        
        return m_stat_value[stat_idx_no];
    }
    
    //////////////////////////////////////////////////////////////////////
    // 函数:      force_flush
    // 功能:      强制将当前统计数据发往服务器
    // 入参:
    // 出参:
    // 返回值:    成功返回0,否则返回错误码
    // 备注:
    //////////////////////////////////////////////////////////////////////
    var_4 force_flush()
    {
        var_1 buffer[10240];
        var_4 buflen = 0;
        
        // "STATSTAT"(8) package_size(4) column_num(4) { value(8) name_size(4) name(name_size) }
        //                               stat_finger(8) server_name_size(4) server_name(server_name_size)
        // "STATSTAT"(8) "!OK!"(4)
        var_1* ptr = buffer;
        
        m_locker.lock();
        
        *(var_u8*)ptr = *(var_u8*)"STATSTAT";
        ptr += 12;
        *(var_4*)ptr = m_cur_stat_column;
        ptr += 4;
        for(var_4 i = 0; i < m_cur_stat_column; i++)
        {
            *(var_u8*)ptr = m_stat_value[i];
            
            m_stat_value[i] = 0;
            
            ptr += 8;
            *(var_4*)ptr = (var_4)strlen(m_stat_name[i]);
            ptr += 4;
            memcpy(ptr, m_stat_name[i], *(var_4*)(ptr - 4));
            ptr += *(var_4*)(ptr - 4);
        }
        
        m_locker.unlock();
        
        var_1* pos = ptr;
        
        var_vd* handle = NULL;
        
        for(var_4 i = 0; i < m_server_num; i++)
        {
            ptr = pos;
            
            *(var_u8*)ptr = m_server_finger[i];
            ptr += 8;
            *(var_4*)ptr = (var_4)strlen(m_server_name[i]);
            ptr += 4;
            memcpy(ptr, m_server_name[i], *(var_4*)(ptr - 4));
            ptr += *(var_4*)(ptr - 4);
            
            buflen = (var_4)(ptr - buffer);
            *(var_4*)(buffer + 8) = buflen - 12;
            
            if(m_cc.open(handle, m_server_ip[i], m_stat_port[i], 5000))
                continue;
            if(m_cc.send(handle, buffer, buflen))
            {
                m_cc.close(handle);
                continue;
            }
            if(m_cc.recv(handle, buffer, 12))
            {
                m_cc.close(handle);
                continue;
            }
            m_cc.close(handle);
            
            if(*(var_u8*)buffer != *(var_u8*)"STATSTAT")
                continue;
            if(*(var_4*)(buffer + 8) != *(var_4*)"!OK!")
                continue;
        }
        
        return 0;
    }

private:
    static CP_THREAD_T thread_flush_server(var_vd* argv)
    {
        UC_CountStat_Client* csc = (UC_CountStat_Client*)argv;
                
        for(;;)
        {
            cp_sleep(csc->m_interval_time);
            csc->force_flush();
        }
        
        return 0;
    }
    
public:    
    var_4  m_interval_time;

    var_4  m_server_num;
    var_1  m_server_ip[MAX_REGISTER_SERVER_NUM][16];
    var_u2 m_stat_port[MAX_REGISTER_SERVER_NUM];
    var_u2 m_register_port[MAX_REGISTER_SERVER_NUM];
    
    var_1  m_server_name[MAX_REGISTER_SERVER_NUM][MAX_NAME_LENGTH];
    var_u8 m_server_finger[MAX_REGISTER_SERVER_NUM];
    
    var_4 m_max_stat_column;
    var_4 m_cur_stat_column;
    
    var_1  m_stat_name[MAX_STAT_COLUMN_COUNT][MAX_NAME_LENGTH];
    var_u8 m_stat_value[MAX_STAT_COLUMN_COUNT];
    
    UC_Communication_Client  m_cc;
    UC_RegisterSystem_Client m_rsc;
    
    CP_MUTEXLOCK m_locker;
};

class UC_CountStat_Server : public UC_Communication_Server
{
public:
    //////////////////////////////////////////////////////////////////////
    // 函数:      init
    // 功能:      初始化统计服务端
    // 入参:
    //                  save_path: 统计日志保存路径
    //                listen_port: 监听端口号
    //              register_port: 注册端口号
    //               reserve_days: 日志保留天数
    //              interval_time: 日志保存间隔时间,单位s
    //
    // 出参:
    // 返回值:    成功返回0,否则返回错误码
    // 备注:
    //////////////////////////////////////////////////////////////////////
    var_4 init(var_1* save_path, var_u2 listen_port, var_u2 register_port, var_4 reserve_days, var_4 interval_time)
    {
        strcpy(m_save_path, save_path);
        m_reserve_days = reserve_days;
        m_interval_time = interval_time;
        
        m_server_num = 0;
        
        memset(m_stat_log, 0, sizeof(UC_LogManager*) * MAX_STAT_SERVER_COUNT);
        
        if(cp_create_thread(thread_flush, this))
            return -1;
        
        if(m_rss.init(save_path, register_port))
            return -1;

        if(UC_Communication_Server::init(listen_port, 10, 5000, 1, 12, 12, 10240, 10240))
            return -1;
        
        return 0;
    }

    var_4 cs_fun_package(const var_1* in_buf, const var_4 in_buf_size, const var_vd* handle = NULL)
    {
        // "STATSTAT"(8) package_size(4) column_num(4) { value(8) name_size(4) name(name_size) }
        //                               stat_finger(8) server_name_size(4) server_name(server_name_size)
        // "STATSTAT"(8) "!OK!"(4)

        if(*(var_u8*)in_buf != *(var_u8*)"STATSTAT")
            return -1;
        
        if(*(var_4*)(in_buf + 8) > 10240)
            return -1;
        
        return *(var_4*)(in_buf + 8);
    }
    
    var_4 cs_fun_process(const var_1* in_buf, const var_4 in_buf_size, var_1* const out_buf, const var_4 out_buf_size, const var_vd* handle = NULL)
    {
        // column_num(4) { value(8) name_size(4) name(name_size) }
        // stat_finger(8) server_name_size(4) server_name(server_name_size)
        // "STATSTAT"(8) "!OK!"(4)
        
        var_1* ptr = (var_1*)in_buf;
        
        var_4  column_num = *(var_4*)ptr;
        ptr += 4;
        var_1* val_pos = ptr;
        
        for(var_4 i = 0; i < column_num; i++)
        {
            ptr += 8;
            ptr += *(var_4*)ptr + 4;
        }
        
        var_u8 finger = *(var_u8*)ptr;
        ptr += 8;
        
        var_4 cur_server = 0;
        for(; cur_server < m_server_num; cur_server++)
        {
            if(finger == m_server_key[cur_server])
                break;
        }
        
        if(cur_server == m_server_num)
        {
            m_server_key[cur_server] = finger;
            memcpy(m_server_str[cur_server], ptr + 4, *(var_4*)ptr);
            m_server_str[cur_server][*(var_4*)ptr] = 0;
            
            m_column_num[cur_server] = 0;
            memset(m_column_val[cur_server], 0, MAX_STAT_COLUMN_COUNT<<3);
        }
        
        for(var_4 i = 0; i < column_num; i++)
        {
            m_column_val[cur_server][i] += *(var_u8*)val_pos;
            val_pos += 8;
            
            if(i >= m_column_num[cur_server])
            {
                memcpy(m_column_str[cur_server][i], val_pos + 4, *(var_4*)val_pos);
                m_column_str[cur_server][i][*(var_4*)val_pos] = 0;
            }
            val_pos += *(var_4*)val_pos + 4;
        }
        
        m_column_num[cur_server] = column_num;
        
        if(cur_server == m_server_num)
            m_server_num++;
        
        *(var_u8*)out_buf = *(var_u8*)"STATSTAT";
        *(var_u4*)(out_buf + 8) = *(var_u4*)"!OK!";
        
        return 12;
    }
    
private:
    static CP_THREAD_T thread_flush(var_vd* argv)
    {
        UC_CountStat_Server* css = (UC_CountStat_Server*)argv;
        
        var_1 path[256];
        var_1 buffer[10240];
        
        for(;;)
        {
            cp_sleep(css->m_interval_time);
            
            for(var_4 i = 0; i < css->m_server_num; i++)
            {
                if(css->m_stat_log[i] == NULL)
                {
                    sprintf(path, "%s/%s", css->m_save_path, css->m_server_str[i]);
                    while(cp_create_dir(path))
                    {
                        printf("UC_CountStat_Server.thread_flush create dir %s error\n", path);
                        cp_sleep(5000);
                    }
                    
                    while(css->m_stat_log[i] == NULL)
                    {
                        css->m_stat_log[i] = new UC_LogManager;
                        if(css->m_stat_log[i] == NULL)
                        {
                            printf("UC_CountStat_Server.thread_flush alloc UC_LogManager error, %s\n", path);
                            cp_sleep(5000);
                        }
                    }
                    
                    while(css->m_stat_log[i]->init(path, (var_1*)"count_stat_", css->m_reserve_days, 1))
                    {
                        printf("UC_CountStat_Server.thread_flush init UC_LogManager error, %s\n", path);
                        cp_sleep(5000);
                    }
                }
                
                var_1* ptr = buffer;
                
                ptr += sprintf(ptr, "%s - ", ctime(NULL));
                for(var_4 j = 0; j < css->m_column_num[j]; j++)
                    ptr += sprintf(ptr, "%s:" CP_PU64" ", css->m_column_str[i][j], css->m_column_val[i][j]);
                ptr += sprintf(ptr, "\n");
                
                css->m_stat_log[i]->uc_log(2, buffer);
            }
        }
        
        return 0;
    }
    
public:
    var_1 m_save_path[256];
    var_4 m_reserve_days;
    var_4 m_interval_time;

    UC_RegisterSystem_Server m_rss;
    
    var_4  m_server_num;
    var_u8 m_server_key[MAX_STAT_SERVER_COUNT];
    var_1  m_server_str[MAX_STAT_SERVER_COUNT][MAX_NAME_LENGTH];
    
    var_4  m_column_num[MAX_STAT_SERVER_COUNT];
    var_u8 m_column_val[MAX_STAT_SERVER_COUNT][MAX_STAT_COLUMN_COUNT];
    var_1  m_column_str[MAX_STAT_SERVER_COUNT][MAX_STAT_COLUMN_COUNT][MAX_NAME_LENGTH];
    
    UC_LogManager* m_stat_log[MAX_STAT_SERVER_COUNT];
};

#endif // _UC_COUNT_STAT_H_
