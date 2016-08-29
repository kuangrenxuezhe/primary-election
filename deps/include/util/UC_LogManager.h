//
//  UC_LogManager.h
//  code_library
//
//  Created by zhanghl on 12-11-1.
//  Copyright (c) 2012年 zhanghl. All rights reserved.
//

#ifndef _UC_LOG_MANAGER_H_
#define _UC_LOG_MANAGER_H_

#include "UH_Define.h"

#define UC_LOGMANAGER_BUFLEN	2048

class UC_LogManager
{
public:
    //////////////////////////////////////////////////////////////////////
    // 函数:      init
    // 功能:      初始化统计服务端
    // 入参:
    //                  save_path: 日志保存路径
    //                 log_prefix: 日志前缀名称
    //               reserve_days: 历史日志保留天数(包含当天)
    //                force_flush: 写入文件的时候是否调用fflush刷新文件缓冲区,0不刷新,1刷新
    //                   fix_flag: 文件修复标记
    //                   fun_drop: 日志删除处理函数
    //                   fun_argv: 传递给日志处理函数的参数
    //
    // 出参:
    // 返回值:    成功返回0,否则返回错误码
    // 备注:
    //////////////////////////////////////////////////////////////////////    
    const var_4 init(var_1* save_path, var_1* log_prefix, var_4 reserve_days, var_4 force_flush = 0, const var_1* fix_flag = "!@#LM#@!", var_vd (*fun_drop)(var_4 log_num /* 最大100 */, var_1 log_name[][256], void* var_vd) = NULL, var_vd* fun_argv = NULL)
    {
        strcpy(m_save_path, save_path);
        strcpy(m_log_prefix, log_prefix);
        m_reserve_days = reserve_days;
        m_force_flush = force_flush;
        
        strcpy(m_fix_flag, fix_flag);
        
        m_fun_drop = fun_drop;
        m_fun_argv = fun_argv;

        if(cp_create_dir(save_path))
            return -1;
        
        drop_reserve_log();
        
        struct tm now;
        cp_localtime(time(NULL), &now);
        
        var_1 filename[256];
        sprintf(filename, "%s/%s_%.4d%.2d%.2d.log", m_save_path, m_log_prefix, now.tm_year + 1900, now.tm_mon + 1, now.tm_mday);
        
        if(access(filename, 0) == 0)
        {
            if(cp_fix_file(filename, m_fix_flag, (var_4)strlen(m_fix_flag)))
                return -1;
            
            m_log_handle = fopen(filename, "rb+");
            if(m_log_handle == NULL)
                return -1;
            fseek(m_log_handle, 0, SEEK_END);
        }
        else
        {
            m_log_handle = fopen(filename, "wb");
            if(m_log_handle == NULL)
                return -1;
        }

		if(cp_create_thread(cs_daemon_thread, this))
			return -1;
        
        return 0;
    }
    
    //////////////////////////////////////////////////////////////////////
    // 函数:      uc_log_time
    // 功能:      记录日志
    // 入参:
    //                       flag: 日志处理函数 0 - 打印屏幕, 1 - 打印文件, 2 - 打印屏幕和文件
    //
    // 出参:
    // 返回值:    成功返回0,否则返回错误码
    // 备注:      注意: 每次打印的日志长度不能超过2000字节
    //////////////////////////////////////////////////////////////////////
    const var_4 uc_log_time(var_4 flag, const var_1* message, ...)
    { 
        var_1 buf_org[UC_LOGMANAGER_BUFLEN];
        var_1 buf_new[UC_LOGMANAGER_BUFLEN];
        
		var_4 len = 0;
		
        va_list ap;
        
        va_start(ap, message);
        len = vsnprintf(buf_org, UC_LOGMANAGER_BUFLEN, message, ap);
        va_end(ap);
		
		if(len >= UC_LOGMANAGER_BUFLEN)
			return -1;
    
		struct tm now;
		cp_localtime(time(NULL), &now);
		
		len = snprintf(buf_new, UC_LOGMANAGER_BUFLEN, "%4d.%2d.%.2d,%.2d:%.2d:%.2d %s", now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec, buf_org);
		
		if(len >= UC_LOGMANAGER_BUFLEN)
			return -1;
		
        m_log_locker.lock();
        
        if(flag == 0)
        {
            printf("%s", buf_new);
        }
        else if(flag == 1)
        {
            fprintf(m_log_handle, "%s", buf_new);            
            fprintf(m_log_handle, "%s", m_fix_flag);
            
            if(m_force_flush == 1)
                fflush(m_log_handle);
        }
        else
        {
            printf("%s", buf_new);
            
            fprintf(m_log_handle, "%s", buf_new);
            fprintf(m_log_handle, "%s", m_fix_flag);
            
            if(m_force_flush == 1)
                fflush(m_log_handle);
        }
        
        m_log_locker.unlock();
        
        return 0;
    }    

    //////////////////////////////////////////////////////////////////////
    // 函数:      uc_log
    // 功能:      记录日志
    // 入参:
    //                       flag: 日志处理函数 0 - 打印屏幕, 1 - 打印文件, 2 - 打印屏幕和文件
    //
    // 出参:
    // 返回值:    成功返回0,否则返回错误码
    // 备注:
    //////////////////////////////////////////////////////////////////////
    const var_4 uc_log(var_4 flag, const var_1* message, ...)
    {
        va_list ap;
        
        m_log_locker.lock();

        if(flag == 0)
        {
            va_start(ap, message);
            vprintf(message, ap);
            va_end(ap);
        }
        else if(flag == 1)
        {
            va_start(ap, message);
            vfprintf(m_log_handle, message, ap);
            va_end(ap);
            
            fprintf(m_log_handle, "%s", m_fix_flag);
            
            if(m_force_flush == 1)
                fflush(m_log_handle);
        }
        else
        {
            va_start(ap, message);
            vprintf(message, ap);
            va_end(ap);

            va_start(ap, message);
            vfprintf(m_log_handle, message, ap);
            va_end(ap);
            
            fprintf(m_log_handle, "%s", m_fix_flag);
            
            if(m_force_flush == 1)
                fflush(m_log_handle);
        }
        
        m_log_locker.unlock();
        
        return 0;
    }    
    
    //////////////////////////////////////////////////////////////////////
    // 函数:      uc_log
    // 功能:      记录日志
    // 入参:
    //                       flag: 日志处理函数 0 - 打印屏幕, 1 - 打印文件, 2 - 打印屏幕和文件
    //
    // 出参:
    // 返回值:    成功返回0,否则返回错误码
    // 备注:
    //////////////////////////////////////////////////////////////////////
    const var_4 uc_log(var_4 flag, var_1* message)
    {        
        m_log_locker.lock();
        
        if(flag == 0)
        {
            printf("%s", message);
        }
        else if(flag == 1)
        {
            fprintf(m_log_handle, "%s%s", message, m_fix_flag);
                        
            if(m_force_flush == 1)
                fflush(m_log_handle);
        }
        else
        {
            printf("%s", message);
            
            fprintf(m_log_handle, "%s%s", message, m_fix_flag);
                        
            if(m_force_flush == 1)
                fflush(m_log_handle);
        }
        
        m_log_locker.unlock();
        
        return 0;
    }
    
    static CP_THREAD_T cs_daemon_thread(var_vd* argv)
    {
        UC_LogManager* lm = (UC_LogManager*)argv;
        
        for(;; cp_sleep(10000))
        {
            time_t now_sec = (time(NULL) + 3600 * 8) % 86400;
            
            if(now_sec > 3600)
                continue;
            
            struct tm now;
            cp_localtime(time(NULL), &now);

            var_1 filename[256];
            sprintf(filename, "%s/%s_%.4d%.2d%.2d.log", lm->m_save_path, lm->m_log_prefix, now.tm_year + 1900, now.tm_mon + 1, now.tm_mday);
            
            if(access(filename, 0) == 0)
                continue;
            
            lm->m_log_locker.lock();
            fclose(lm->m_log_handle);
            do {
                lm->m_log_handle = fopen(filename, "wb");
                if(lm->m_log_handle)
                    break;
                printf("UC_LogManager.cs_daemon_thread open %s error\n", filename);
                cp_sleep(1000);
            } while(lm->m_log_handle == NULL);
            lm->m_log_locker.unlock();
            
            lm->drop_reserve_log();
        }
        
        return 0;
    }
public:
    var_4 drop_reserve_log()
    {
        time_t now_sec = time(NULL);
        now_sec -= 86400 * m_reserve_days;
        
        var_1 log_lst[100][256];
        var_1 log_num = 0;
        
        for(var_4 i = 0; i < 1000 && log_num < 100; i++, now_sec -= 86400)
        {
            struct tm now;
            cp_localtime(now_sec, &now);
        
            sprintf(log_lst[log_num], "%s/%s_%.4d%.2d%.2d.log", m_save_path, m_log_prefix, now.tm_year + 1900, now.tm_mon + 1, now.tm_mday);
        
            if(access(log_lst[log_num], 0) == 0)
                log_num++;
        }
        
        if(m_fun_drop)
            m_fun_drop(log_num, log_lst, m_fun_argv);
        
        for(var_4 i = 0; i < log_num; i++)
        {
            while(access(log_lst[i], 0) == 0 && remove(log_lst[i]))
            {
                printf("cs_daemon_thread.drop_reserve_log remove %s error\n", log_lst[i]);
                cp_sleep(5000);
            }
        }
        
        return 0;
    }
    
public:
    var_1 m_save_path[256];
    var_1 m_log_prefix[128];
    var_4 m_reserve_days;
    var_4 m_force_flush;
    
    var_1 m_fix_flag[256];
    
    FILE*        m_log_handle;
    CP_MUTEXLOCK m_log_locker;
    
    var_vd (*m_fun_drop)(var_4 log_num, var_1 log_name[][256], var_vd* argv);
    var_vd*  m_fun_argv;
};

#endif // _UC_LOG_MANAGER_H_
