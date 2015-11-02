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
    var_4 init(var_4 interval_time, var_4 server_num, var_1** server_ip, var_u2* stat_port, var_u2* register_port, var_1** server_name, var_1** stat_name = NULL, var_4 cur_stat_column = 0, var_4 max_stat_column = 100)
    {
        m_interval_time = interval_time;
        m_server_num = server_num;
        
        for(var_4 i = 0; i < m_server_num; i++)
        {
            strcpy(m_server_ip[i], server_ip[i]);
            m_stat_port[i] = stat_port[i];
            m_register_port[i] = register_port[i];
            strcpy(m_server_name[i], server_name[i]);            
        }

        m_cur_stat_column = cur_stat_column;
        m_max_stat_column = max_stat_column;
        
        m_stat_value = new var_u8[m_max_stat_column];
        if(m_stat_value == NULL)
            return -1;
        
        m_stat_name = new var_1*[m_max_stat_column];
        if(m_stat_name == NULL)
            return -1;
        
        for(var_4 i = 0; i < m_cur_stat_column; i++)
        {
            m_stat_name[i] = new var_1[256];
            if(m_stat_name[i] == NULL)
                return -1;
            strcpy(m_stat_name[i], stat_name[i]);
            
            m_stat_value[i] = 0;
        }
        
        for(var_4 i = 0; i < m_server_num; i++)
        {
            if(m_cc[i].init(m_server_ip[i], m_stat_port[i], 5000))
                return -1;
        }
        
        if(m_rsc.init("0.0.0.0", 0))
            return -1;
        
        for(var_4 i = 0; i < m_server_num; i++)
        {
            if(m_rsc.register_to_server(m_server_name[i], (var_4)strlen(m_server_name[i]), m_server_finger + i, m_server_ip[i], m_register_port[i]) <= 0)
                return -1;
        }

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
        return cp_fetch_and_add(m_stat_value + stat_idx_no, 1);
    }
    
    //////////////////////////////////////////////////////////////////////
    // 函数:      get_stat_value
    // 功能:      取出指定列当前值
    // 入参:
    //                stat_idx_no: 要自加计数列的索引号
    //
    // 出参:
    // 返回值:    返回当前值
    // 备注:
    //////////////////////////////////////////////////////////////////////
    inline var_u8 get_stat_value(var_4 stat_idx_no)
    {        
        return cp_fetch_and_add(m_stat_value + stat_idx_no, 0);
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
        return 0;
    }

private:
    static CP_THREAD_T thread_flush_server(var_vd* argv)
    {
        return 0;
    }
    
public:    
    var_4  m_interval_time;

    var_4  m_server_num;
    var_1  m_server_ip[64][16];
    var_u2 m_stat_port[64];
    var_u2 m_register_port[64];
    var_1  m_server_name[64][128];
    var_u8 m_server_finger[64];
    
    var_4 m_max_stat_column;
    var_4 m_cur_stat_column;
    
    var_1** m_stat_name;
    var_u8* m_stat_value;
    
    UC_Communication_Client  m_cc[64];
    UC_RegisterSystem_Client m_rsc;
    
    CP_MUTEXLOCK m_locker;
};

class UC_CountStat_Server
{
public:
    //////////////////////////////////////////////////////////////////////
    // 函数:      init
    // 功能:      初始化统计服务端
    // 入参:
    //                  save_path: 统计日志保存路径
    //                listen_port: 监听端口号
    //               reserve_days: 日志保留天数
    //              interval_time: 数据处理函数调用间隔时间
    //
    // 出参:
    // 返回值:    成功返回0,否则返回错误码
    // 备注:
    //////////////////////////////////////////////////////////////////////
    var_4 init(var_1* save_path, var_u2 listen_port, var_4 reserve_days, var_4 interval_time = 0);
};

#endif // _UC_COUNT_STAT_H_
