//
//  CF_framework_center.h
//  CollaborativeFiltering
//
//  Created by zhanghl on 14-9-3.
//  Copyright (c) 2014年 CrystalBall. All rights reserved.
//

#ifndef _CF_FRAMEWORK_CENTER_H_
#define _CF_FRAMEWORK_CENTER_H_

#include "util/UH_Define.h"
#include "util/UC_CenterMessage.h"
#include "util/UC_Mem_Allocator_Recycle.h"

#include "util/UC_MD5.h"
#include "util/UT_Sort.h"
#include "util/UT_Queue.h"
#include "util/UC_Queue_VL.h"
#include "util/UT_HashTable_Pro.h"

#include "util/cJSON.h"

#include "framework/CF_framework_config.h"
#include "framework/CF_framework_cache.h"


class ICM_Query_Processor : public Interface_CenterMessage_Processor
{
public: // for interface
    var_4 new_handle(var_vd*& handle)
    {
        handle = m_ma.get_mem();
        if(handle == NULL)
            return -1;
        
        return 0;
    }
    var_4 del_handle(var_vd*& handle)
    {
        m_ma.put_mem(handle);
        
        return 0;
    }
    
    var_4 distribute_task(var_vd* handle, var_1* message_buf, var_4 message_len) // return 'all distribute count'
    {
        if(m_cfg->fw_calc_send_size < message_len)
            return -1;
        
        *(FILE**)handle = *(FILE**)message_buf;
        
        var_1* buf = (var_1*)handle + m_offset_send + 8;
        
        *(var_4*)buf = message_len - 8;
        memcpy(buf + 4, message_buf + 8, message_len - 8);
        
        return m_cfg->fw_calc_num;
    }
    
    var_vd process_task(var_vd* handle, var_4 cur_count)
    {
        var_1* buf = (var_1*)handle + 8;
        var_1* s_buf = buf + m_offset_send;
        var_1* r_buf = buf + m_offset_recv + m_one_recv_size * cur_count;
        var_4  r_len = 0;
        
        *(var_4*)r_buf = -1;
        
        CP_SOCKET_T client;
        
        if(cp_connect_socket(client, m_cfg->fw_calc_ip[cur_count], m_cfg->fw_calc_port[cur_count]))
            return;
        
        cp_set_overtime(client, 30000);
        
        if(cp_sendbuf(client, s_buf + 4, *(var_4*)s_buf))
        {
            cp_close_socket(client);
            return;
        }
        
        if(cp_recvbuf(client, (var_1*)&r_len, 4))
        {
            cp_close_socket(client);
            return;
        }
        
        if(r_len + 4 > m_cfg->fw_calc_recv_size)
        {
            cp_close_socket(client);
            return;
        }
        
        if(cp_recvbuf(client, r_buf + 4, r_len))
        {
            cp_close_socket(client);
            return;
        }
        
        *(var_4*)r_buf = r_len;
        
        cp_close_socket(client);
        
        return;
    }
    
    var_4 combine_task(var_vd* handle)
    {
        FILE* log_file = *(FILE**)handle;
        
		var_1*  snd_buf = (var_1*)handle + m_offset_send + 8; // 4len 4len 4flag 8uid 4recnum 8recid*num 4hisnum 8hisid*num
		var_u8  log_uid = *(var_u8*)(snd_buf + 12);
		var_4   log_num = *(var_4*)(snd_buf + 20);
		var_u8* log_did = (var_u8*)(snd_buf + 24);
		
        var_1* org_buf = (var_1*)handle + m_offset_recv + 8;
        var_1* res_buf = (var_1*)handle + m_offset_result + 8;

		memset(res_buf, 0, 4 + m_cfg->fw_calc_result_size);

        var_4*  res_num = (var_4*)(res_buf + 4);
        var_f4* res_pwd = (var_f4*)(res_buf + 8);
		
		var_1 name[][16] = {"itemBased   ", "contentBased", "modelBased  ", "minHash     "};

		var_4 first_flg = 1;

        if(log_file)
        {
            fprintf(log_file, "uid         : %22lu\n", log_uid);
            fprintf(log_file, "did         : ");
            for(var_4 x = 0; x < log_num; x++)
                fprintf(log_file, "%22lu", log_did[x]);
            fprintf(log_file, "\n");
        }
        
        for(var_4 i = 0; i < m_cfg->fw_calc_num; i++)
        {
            // len num [power ...]
            
            var_4 len = *(var_4*)org_buf;
            if(len == -1)
            {
				printf("ICM_Query_Processor - communication error, (%d,%s)\n", i, name[i]);

                org_buf += m_one_recv_size;
                continue;
            }
			if(len == 0)
			{
				printf("ICM_Query_Processor - interface return error, (%d,%s)\n", i, name[i]);
				
				org_buf += m_one_recv_size;
				continue;
			}
            if(len % 4)
            {
				printf("ICM_Query_Processor - return result length error, (%d,%s)\n", i, name[i]);

                org_buf += m_one_recv_size;
                continue;
            }
            
            var_4 num = *(var_4*)(org_buf + 4);
            if((num + 1) * 4 != len)
            {
				printf("ICM_Query_Processor - return result number error, (%d,%s)\n", i, name[i]);

                org_buf += m_one_recv_size;
                continue;
            }
            
            if(first_flg == 1)
			{
                *res_num = num;
				first_flg = 0;
			}
            else
                assert(*res_num == num);
			
            if(log_file)
                fprintf(log_file, "%s: ", name[i]);
			
            var_f4* pwr = (var_f4*)(org_buf + 8);
            for(var_4 j = 0; j < num; j++)
			{
                res_pwd[j] += pwr[j] * m_cfg->fw_calc_factor[i];
				
                if(log_file)
                    fprintf(log_file, "%22f", pwr[j]);
			}
			
            if(log_file)
                fprintf(log_file, "\n");

            org_buf += m_one_recv_size;
        }
		
		if(first_flg == 1)
			*res_num = 0;
        
        *(var_4*)res_buf = (*res_num + 1) * 4;
        
        return 0;
    }
    
    var_4 extract_result(var_vd* handle, var_1* result_buf, var_4 result_len, var_4& return_len)
    {
        var_1* buf = (var_1*)handle + m_offset_result + 8;
        
        if(*(var_4*)buf > result_len)
            return -1;
        
        return_len = *(var_4*)buf;
        memcpy(result_buf, buf + 4, return_len);
        
        return 0;
    }
    
public:
    var_4 init(CENTER_CONFIG* config)
    {
        m_cfg = config;
        
        m_one_recv_size = 4 + m_cfg->fw_calc_recv_size;
        
        m_offset_send   = 0;
        m_offset_recv   = 4 + m_cfg->fw_calc_send_size;
        m_offset_result = m_offset_recv + m_one_recv_size * m_cfg->fw_calc_num;
        
        var_4 mem_size = 0;
        mem_size += sizeof(FILE*);
        mem_size += 4 + m_cfg->fw_calc_send_size;         // send size + send buffer
        mem_size += m_cfg->fw_calc_num * m_one_recv_size; // (recv size + recv buffer) * machine num
        mem_size += 4 + m_cfg->fw_calc_result_size;       // result size + result buffer
        
        if(m_ma.init(mem_size))
            return -1;
        
        return 0;
    }
    
private:
    CENTER_CONFIG* m_cfg;
    UC_Mem_Allocator_Recycle m_ma;
    
    var_4 m_offset_send;
    var_4 m_offset_recv;
    var_4 m_offset_result;
    
    var_4 m_one_recv_size;
};

class ICM_Update_Processor : public Interface_CenterMessage_Processor
{
public: // for interface
    var_4 new_handle(var_vd*& handle)
    {
        handle = m_ma.get_mem();
        if(handle == NULL)
            return -1;
        
        return 0;
    }
    var_4 del_handle(var_vd*& handle)
    {
        m_ma.put_mem(handle);
        
        return 0;
    }
    
    var_4 distribute_task(var_vd* handle, var_1* message_buf, var_4 message_len) // return 'all distribute count'
    {
        if(m_cfg->fw_calc_send_size < message_len)
            return -1;
        
        var_1* buf = (var_1*)handle;
        
        *(var_4*)buf = message_len;
        memcpy(buf + 4, message_buf, message_len);
        
        return m_cfg->fw_calc_num;
    }
    
    var_vd process_task(var_vd* handle, var_4 cur_count)
    {
        var_1* buf = (var_1*)handle;
        var_1* s_buf = buf + m_offset_send;
        var_1* r_buf = buf + m_offset_recv + m_one_recv_size * cur_count;
        var_4  r_len = 0;
        
        *(var_4*)r_buf = 0;
        
        CP_SOCKET_T client;
        
        if(cp_connect_socket(client, m_cfg->fw_calc_ip[cur_count], m_cfg->fw_calc_port[cur_count]))
            return;
        
        cp_set_overtime(client, 5000);
        
        if(cp_sendbuf(client, s_buf + 4, *(var_4*)s_buf))
        {
            cp_close_socket(client);
            return;
        }
        
        if(cp_recvbuf(client, (var_1*)&r_len, 4))
        {
            cp_close_socket(client);
            return;
        }
        
        if(r_len + 4 > m_cfg->fw_calc_recv_size)
        {
            cp_close_socket(client);
            return;
        }
        
        if(cp_recvbuf(client, r_buf + 4, r_len))
        {
            cp_close_socket(client);
            return;
        }
        
        *(var_4*)r_buf = r_len;
        
        cp_close_socket(client);
        
        return;
    }
    
    var_4 combine_task(var_vd* handle)
    {
        var_1* buf = (var_1*)handle + m_offset_recv;
        
        var_4 ret_val = 0;
		
		var_1 name[][16] = {"itemBased", "contentBased", "modelBased", "minHash"};
		
        for(var_4 i = 0; i < m_cfg->fw_calc_num; i++)
        {
			if(*(var_4*)buf == 0)
			{
				printf("ICM_Update_processor - communication error, (%d,%s)\n", i, name[i]);
				
				ret_val++;
				buf += m_one_recv_size;
				continue;
			}
			if(*(var_4*)buf != 4)
			{
				printf("ICM_Update_Processor - protocol error, idx (%d,%s)\n", i, name[i]);
			
                ret_val++;
                buf += m_one_recv_size;
                continue;
            }
            if(*(var_4*)(buf + 4) != 0)
            {
				printf("ICM_Update_Processor - interface return error(%d), (%d,%s)\n", *(var_4*)(buf + 4), i, name[i]);
				
                ret_val++;
                buf += m_one_recv_size;
                continue;
            }
            
            buf += m_one_recv_size;
        }
        
        buf = (var_1*)handle + m_offset_result;
        
        *(var_4*)buf = 4;
        *(var_4*)(buf + 4) = ret_val;
        
        return 0;
    }
    
    var_4 extract_result(var_vd* handle, var_1* result_buf, var_4 result_len, var_4& return_len)
    {
        var_1* buf = (var_1*)handle + m_offset_result;
        
        if(*(var_4*)buf > result_len)
            return -1;
        
        return_len = *(var_4*)buf;
        memcpy(result_buf, buf + 4, return_len);
        
        return 0;
    }
    
public:
    var_4 init(CENTER_CONFIG* config)
    {
        m_cfg = config;
        
        m_one_recv_size = 4 + m_cfg->fw_calc_recv_size;
        
        m_offset_send   = 0;
        m_offset_recv   = 4 + m_cfg->fw_calc_send_size;
        m_offset_result = m_offset_recv + m_one_recv_size * m_cfg->fw_calc_num;
        
        var_4 mem_size = 0;
        mem_size += 4 + m_cfg->fw_calc_send_size;         // send size + send buffer
        mem_size += m_cfg->fw_calc_num * m_one_recv_size; // (recv size + recv buffer) * machine num
        mem_size += 4 + m_cfg->fw_calc_result_size;       // result size + result buffer
        
        if(m_ma.init(mem_size))
            return -1;
        
        return 0;
    }
    
private:
    CENTER_CONFIG* m_cfg;
    UC_Mem_Allocator_Recycle m_ma;
    
    var_4 m_offset_send;
    var_4 m_offset_recv;
    var_4 m_offset_result;
    
    var_4 m_one_recv_size;
};

typedef struct _task_node_
{
    CP_SOCKET_T client;
    
    var_1*      recv_buf;
    var_4       recv_len;
    
    var_1*      send_buf;
    var_4       send_len;
    
    var_1*      parse_buf;
    var_4       parse_len;
    var_4       parse_max;
    
    var_1*      binary_buf;
    var_4       binary_len;
    
    var_4       type_flg;
    var_4       flag_log;
} TASK_NODE;

class CF_framework_center
{
public:
    var_4 init_framework()
    {
        if(m_cfg.init())
            return -1;

        if(cp_init_socket())
            return -1;
        if(cp_listen_socket(m_listen, m_cfg.fw_listen_port))
            return -1;
        
        if(m_cfg.fw_sync_flag)
        {
            m_sync_flag = (var_1*)"#!SYNC!#";
            
            sprintf(m_sync_save, "%s/sync.save", m_cfg.fw_sync_path);
            sprintf(m_sync_send, "%s/sync.send", m_cfg.fw_sync_path);
            
            cp_fix_file(m_sync_send, m_sync_flag, 8);
            cp_fix_file(m_sync_save, m_sync_flag, 8);
            
            if(cp_create_thread(thread_feedback, this))
                return -1;
        }
        
        if(cp_create_thread(thread_watch, this))
            return -1;
        
        if(m_taskQueue.InitQueue(m_cfg.fw_queue_size))
            return -1;
        
        if(m_ma_TN.init(m_cfg.fw_max_recv_size + m_cfg.fw_max_send_size + m_cfg.fw_max_binary_size + sizeof(TASK_NODE)))
            return -1;
        if(m_ma_TB.init(m_cfg.fw_max_task_size))
            return -1;
        
        if(m_cm_query_processor.init(&m_cfg))
            return -1;
        if(m_cm_query.init(20, 60, &m_cm_query_processor))
            return -1;
        
        if(m_cm_update_processor.init(&m_cfg))
            return -1;
        if(m_cm_update.init(20, 60, &m_cm_update_processor))
            return -1;
		
		if(m_cache.init((var_1*)m_cfg.fw_cache_path, m_cfg.fw_cache_capacity, m_cfg.fw_cache_savecount, m_cfg.fw_cache_flushcount, m_cfg.fw_cache_flushtime, m_cfg.fw_cache_debug))
			return -1;
		
		if(m_flags.init(m_cfg.fw_cache_capacity))
			return -1;
		
		if(m_flushCache.InitQueue(100, 12))
			return -1;
		
		if(cp_create_thread(thread_flushcache, this, m_cfg.fw_update_thread_num))
			return -1;
		
        if(cp_create_thread(thread_listen, this, m_cfg.fw_listen_thread_num))
            return -1;
     
        if(cp_create_thread(thread_process, this, m_cfg.fw_process_thread_num))
            return -1;
        
        printf("****** system init success ******\n");
        
        return 0;
    }
	
    static CP_THREAD_T thread_watch(var_vd* argv)
    {
        enum ErrorType
        {
            TYPE_OK = 0, //运行正常
            TYPE_MONITOR, //monitor错误
            TYPE_NETWORK, //网络故障
            TYPE_SERVICE, //服务错误
            TYPE_OTHER, //其它错误
        };
        enum ErrorLevel
        {
            LEVEL_A = 1, //严重
            LEVEL_B, //重要
            LEVEL_C, //一般
            LEVEL_D, //调试
            LEVEL_E //可忽略
        };
        
        CP_SOCKET_T watch;
        
        if(cp_listen_socket(watch, 8642))
            return NULL;
        
        char szInfoBuf[1024];
        memcpy(szInfoBuf,"MonitorP1 ",10);
        
        for(;;)
        {
            CP_SOCKET_T client;
            
            while(cp_accept_socket(watch, client))
                continue;
            
            cp_set_overtime(client, 1000);
            
            var_1* lpszStr = szInfoBuf + 10;
            
            if(cp_recvbuf(client, lpszStr, 4))
            {
                *(var_4*)lpszStr=4;
                lpszStr+=4;
                *(var_4*)lpszStr=TYPE_NETWORK;
                lpszStr+=4;
            }
            else
            {
                lpszStr += 4;
                var_1* lpszType = lpszStr;
                lpszStr += 4;
                var_1* lpszLevel = lpszStr;
                lpszStr += 4;
                
                /*
                    *(var_4*)lpszType=TYPE_SERVICE;
                    *(var_4*)lpszLevel=LEVEL_A;
                */
                
                *(var_4*)lpszType=TYPE_OK;
                *(var_4*)lpszLevel=LEVEL_B;
            
                *(var_4*)(szInfoBuf + 10) = (var_4)(lpszStr - szInfoBuf - 14);
            }
            
            cp_sendbuf(client, szInfoBuf, (var_4)(lpszStr - szInfoBuf));
            cp_close_socket(client);

            printf("thread_watch - monitor ok\n");
        }
        return NULL;
    }
    
    static CP_THREAD_T thread_listen(var_vd* argv)
    {
        CF_framework_center* cc = (CF_framework_center*)argv;
        
        CP_SOCKET_T client;

        for(;;)
        {
            if(cp_accept_socket(cc->m_listen, client))
                continue;
            
            printf("\nthread_listen - accept success\n");
            
            cp_set_overtime(client, 5000);
            
            TASK_NODE* task = NULL;
            
            var_4 ret_val = 0;
            
            try
            {
                task = cc->get_task_node();
                
                task->client = client;
                
                if(cp_recvbuf(client, (var_1*)&task->recv_len, 4))
                    throw -1;
                
                if(task->recv_len > cc->m_cfg.fw_max_recv_size - 1)
                    throw -2;
                
                if(cp_recvbuf(client, task->recv_buf, task->recv_len))
                    throw -3;
                
                task->recv_buf[task->recv_len] = 0;
                
                task->parse_len = task->parse_max;
                
                ret_val = cc->parse_json(task->recv_buf, task->parse_buf, task->parse_len);
                if(ret_val)
                    throw -4;

                *(var_4*)task->binary_buf = task->parse_len;
                task->binary_len = task->parse_len + 4;
                
                task->type_flg = *(var_4*)task->parse_buf;
                
                if(task->type_flg == 4)
                    task->flag_log = *(var_4*)(task->parse_buf + 4);
                else
                    task->flag_log = 0;
                
                printf("thread_listen - request type %d (1.click, 2.document, 3.circle, 4.query, 5.heartbeat)\n", task->type_flg);
                
                cc->m_taskQueue.PushData(task);
            }
            catch (var_4 err)
            {
                if(err == -1)
                    printf("thread_listen - recv head error\n");
                else if(err == -2)
                    printf("thread_listen - recv length over max length\n");
                else if(err == -3)
                    printf("thread_listen - recv body error\n");
                else if(err == -4)
                    printf("thread_listen - parse json error, %d\n", ret_val);

                cc->put_task_node(task);
                
                cp_close_socket(client);
            }
        }
        
        return NULL;
    }
	
    static CP_THREAD_T thread_process(var_vd* argv)
    {
        CF_framework_center* cc = (CF_framework_center*)argv;
        
        for(;;)
        {
            TASK_NODE* task = cc->m_taskQueue.PopData();
            
            var_4 error_flg = 1;
            
            switch (task->type_flg)
            {
                case 1:
                    printf("thread_process - type 1, process click\n");
                    
                    if(cc->save_log_action(task))
                        printf("thread_process - type 1, save_log_action failure\n");
                    
                    if(cc->process_action(task))
                        error_flg = 0;
                    break;
                    
                case 2:
                    printf("thread_process - type 2, process document\n");
                    
                    if(cc->save_log_document(task))
                        printf("thread_process - type 2, save_log_document failure\n");

                    if(cc->process_document(task))
                        error_flg = 0;
                    break;
                    
                case 3:
                    printf("thread_process - type 3, process subscribe\n");
                    if(cc->process_subscribe(task))
                        error_flg = 0;
                    break;

                case 4:
                    printf("thread_process - type 4, process query\n");
                    if(cc->process_query(task))
                        error_flg = 0;
                    break;
                    
                case 5:
                    printf("thread_process - type 5, process heartbeat\n");
                    task->send_len = sprintf(task->send_buf, "{\"type\":5,\"status\":\"ok\"}");
                    break;
                
                case 6:
                    printf("thread_process - type 6, process feedback\n");
                    if(cc->process_feedback(task, 0, 0, NULL))
                        error_flg = 0;
            }

            if(error_flg)
            {
                if(cp_sendbuf(task->client, (var_1*)&task->send_len, 4))
                    printf("send head error\n");
                if(cp_sendbuf(task->client, task->send_buf, task->send_len))
                    printf("send body error\n");
                
                printf("thread_process - process finish, type %d, ok\n", task->type_flg);
            }
            else
            {
                *(var_4*)task->send_buf = sprintf(task->send_buf + 4, "{\"type\":%d,\"code\":-1}", task->type_flg);
                task->send_len = *(var_4*)task->send_buf + 4;

                if(cp_sendbuf(task->client, task->send_buf, task->send_len))
                    printf("send body error\n");
                
                printf("thread_process - process finish, type %d, error\n", task->type_flg);
            }
            
            cp_close_socket(task->client);
            
            cc->put_task_node(task);
        }
        
        return NULL;
    }
    
    static CP_THREAD_T thread_feedback(var_vd* argv)
    {
        CF_framework_center* cc = (CF_framework_center*)argv;
        
        var_1* buffer = new(std::nothrow) var_1[1<<20];
        
        for(;;)
        {
            cp_sleep(5000);
            
            if(access(cc->m_sync_send, 0))
            {
                if(access(cc->m_sync_save, 0))
                    continue;
                
                cc->m_sync_lock.lock();
                while(cp_rename_file(cc->m_sync_save, cc->m_sync_send))
                {
                    printf("thread_feedback - rename %s to %s failure\n", cc->m_sync_save, cc->m_sync_send);
                    cp_sleep(5000);
                }
                cc->m_sync_lock.unlock();
            }
            
            FILE* fp = fopen(cc->m_sync_send, "rb");
            if(fp == NULL)
            {
                printf("thread_feedback - open %s failure\n", cc->m_sync_send);
                continue;
            }
            for(;;)
            {
                if(fread(buffer, 12, 1, fp) != 1)
                    break;
                if(fread(buffer + 12, (*(var_4*)(buffer + 8) + 1)<<3, 1, fp) != 1)
                {
                    printf("thread_feedback - read %s failure\n", cc->m_sync_send);
                    break;
                }
                
                while(cc->candidate_feedback(*(var_u8*)buffer, *(var_4*)(buffer + 8), (var_u8*)(buffer + 12)))
                {
                    printf("thread_feedback - candidate_feedback failure\n");
                    cp_sleep(2000);
                }
            }
            fclose(fp);
            
            cp_remove_file(cc->m_sync_send);
        }
        
        return 0;
    }

    static CP_THREAD_T thread_flushcache(var_vd* argv)
    {
        CF_framework_center* cc = (CF_framework_center*)argv;
        
        for(;;)
        {
            var_1 data[12];
            
            cc->m_flushCache.PopData(data);
            
            var_u8 uid = *(var_u8*)data;
            var_4  log = *(var_4*)(data + 8);
            
            cc->process_query(uid, log);
        }
        
        return NULL;
    }
    
    var_4 process_action(TASK_NODE* task)
    {
        // query filter history
        var_4  update_len = m_cfg.fw_max_task_size;
        var_1* update_buf = (var_1*)m_ma_TB.get_mem();
        if(update_buf == NULL)
            return -1;
        
        *(var_4*)(update_buf + 4) = task->type_flg;
        
        var_4  history_len = update_len - 8;
        var_1* history_buf = update_buf + 8; // len(var_4), type(var_4)
        
        var_1  send_buf[128];
        var_1* send_pos = send_buf + 4;
        
        *(var_4*)send_pos = 4; // query
        send_pos += 4;
        *(var_4*)send_pos = 2; // query type
        send_pos += 4;
        *(var_u8*)send_pos = *(var_u8*)(task->parse_buf + 8); // uid
        send_pos += 8;
        
        var_4 send_len = (var_4)(send_pos - send_buf);
        *(var_4*)send_buf = send_len - 4;

        // history_num(var_4), history_list(var_u8...)
        if(candidate_query(send_buf, send_len, history_buf, history_len))
        {
            printf("process_action - call candidate_query(history) error\n");
            
            m_ma_TB.put_mem((var_vd*)update_buf);
            return -1;
        }
        if(history_len == 0)
        {
            printf("process_action - call candidate_query(history) empty\n");
            
            m_ma_TB.put_mem((var_vd*)update_buf);
            return -1;
        }
        
        // update filter
        var_1 recv_buf[32];
        var_4 recv_len = 32;
        
        if(candidate_query(task->binary_buf, task->binary_len, recv_buf, recv_len))
        {
            printf("process_action - call candidate_query(update) error\n");
            
            m_ma_TB.put_mem((var_vd*)update_buf);
            return -1;
        }
        
        // update calc
        memcpy(history_buf + history_len, task->parse_buf, task->parse_len);
        
        update_len = history_len + task->parse_len + 8;
        *(var_4*)update_buf = update_len - 4;
        
        var_vd* handle = NULL;
        
        if(m_cm_update.put_message(update_buf, update_len, &handle))
        {
            printf("process_action - put_message error\n");
            
            m_ma_TB.put_mem((var_vd*)update_buf);
            return -1;
        }
        
        recv_len = 32;
        
        if(m_cm_update.get_message(recv_buf, recv_len, recv_len, &handle))
        {
            printf("process_action - get_message error\n");
            
            m_ma_TB.put_mem((var_vd*)update_buf);
            return -1;
        }
        
        m_ma_TB.put_mem((var_vd*)update_buf);
        
        // return buffer
        task->send_len = sprintf(task->send_buf, "{\"type\":%d,\"code\":%d}", task->type_flg, *(var_4*)recv_buf);
        
        return 0;
    }

    var_4 process_document(TASK_NODE* task)
    {
        // update filter
        var_1 recv_buf[32];
        var_4 recv_len = 32;
        
        if(candidate_query(task->binary_buf, task->binary_len, recv_buf, recv_len))
        {
            printf("process_document - candidate_query error\n");
            return -1;
        }
        
        // update calc
        var_vd* handle = NULL;
        
        if(m_cm_update.put_message(task->binary_buf, task->binary_len, &handle))
        {
            printf("process_document - put_message error\n");
            return -1;
        }
        
        recv_len = 32;
        
        if(m_cm_update.get_message(recv_buf, recv_len, recv_len, &handle))
        {
            printf("process_document - get_message error\n");
            return -1;
        }
        
        // return buffer
        task->send_len = sprintf(task->send_buf, "{\"type\":%d,\"code\":%d}", task->type_flg, *(var_4*)recv_buf);
        
        return 0;
    }
    
    var_4 process_subscribe(TASK_NODE* task)
    {
        // update filter
        var_1 recv_buf[32];
        var_4 recv_len = 32;
        
        if(candidate_query(task->binary_buf, task->binary_len, recv_buf, recv_len))
        {
            printf("process_subscribe - candidate_query error\n");
            return -1;
        }
        
        task->send_len = sprintf(task->send_buf, "{\"type\":%d,\"code\":%d}", task->type_flg, *(var_4*)recv_buf);
        
        return 0;
    }
    
	var_4 process_query(TASK_NODE* task)
	{
		// user id
        var_u8 user_id = *(var_u8*)(task->parse_buf + 8);
		
		// request num
        var_4 request_num = *(var_4*)(task->parse_buf + 16);
		
		printf("process_query - query cache num %d, uid " CP_PU64"\n", request_num, user_id);
		
		if(request_num > 20)
			request_num = 20;
		
		var_u8 ret_lst[16];
		var_4  ret_num = 0;
		
		// query cache
		while(m_flags.add(user_id))
			cp_sleep(1);
        
        var_4 is_cache_flush = 0;
        
		var_4 ret_val = m_cache.get(user_id, request_num, ret_lst, ret_num);
        if(ret_val < 0) // new user, flush cache
        {
            is_cache_flush = 1;
            
            if(candidate_isNewUser(user_id)) // real new user
                task->send_len = sprintf(task->send_buf, "{\"task\":4,\"returnnum\":0,\"docid\":[],\"userflag\":2");
            else // fake new user, becaue data lose, switch to overtime user
                task->send_len = sprintf(task->send_buf, "{\"task\":4,\"returnnum\":0,\"docid\":[],\"userflag\":3");
        }
        else if(ret_val == 1 || ret_val == 2) // normal user
        {
            if(ret_val == 1)
                is_cache_flush = 1;
            else
                is_cache_flush = 0;
            
            task->send_len = sprintf(task->send_buf, "{\"task\":4,\"returnnum\":%d,\"docid\":[", ret_num);
            for(var_4 i = 0; i < ret_num; i++)
            {
                if(i == 0)
                    task->send_len += sprintf(task->send_buf + task->send_len, CP_PU64, ret_lst[i]);
                else
                    task->send_len += sprintf(task->send_buf + task->send_len, "," CP_PU64, ret_lst[i]);
            }
            task->send_len += sprintf(task->send_buf + task->send_len, "],\"userflag\":1");
        }
        else if(ret_val == 3 || ret_val == 4) // overtime user
        {
            if(ret_val == 3)
                is_cache_flush = 1;
            else
                is_cache_flush = 0;
            
			task->send_len = sprintf(task->send_buf, "{\"task\":4,\"returnnum\":0,\"docid\":[],\"userflag\":3");
        }
		
		printf("%s\n", task->send_buf);
		
        // feedback recommend data
        while(ret_num && process_feedback(NULL, user_id, ret_num, ret_lst))
        {
            printf("process_query - call process_feedback failure\n");
            cp_sleep(2000);
        }

		// update cache
		if(is_cache_flush)
        {
            var_1 data[12];
            
            *(var_u8*)data = user_id;
            *(var_4*)(data + 8) = task->flag_log;
            
			m_flushCache.PushData(data);
        }
		
		var_4 ret = m_flags.del(user_id);
		assert(ret == 0);
		
		return 0;
	}
    
    var_4 process_query(var_u8 uid, var_4 log)
    {
        var_4 request_num = m_cfg.fw_cache_savecount;
        
		printf("process_query - update cache num %d\n", request_num);
		
        // query filter
        var_1  send_buf[128];
        
        var_1* send_pos = send_buf + 4;
        
        *(var_4*)send_pos = 4; // query
        send_pos += 4;
        *(var_4*)send_pos = 1; // query type
        send_pos += 4;
        *(var_u8*)send_pos = uid;
        send_pos += 8;
        
        var_4 send_len = (var_4)(send_pos - send_buf);
        *(var_4*)send_buf = send_len - 4;

        var_4  filter_len = m_cfg.fw_max_task_size;
        var_1* filter_buf = (var_1*)m_ma_TB.get_mem();
        if(filter_buf == NULL)
            return -1;
        
        if(candidate_query(send_buf, send_len, filter_buf, filter_len))
        {
			printf("process_query - call candidate_query error\n");
			
            m_ma_TB.put_mem((var_vd*)filter_buf);
            return -1;
        }
		if(filter_len == 0)
		{
			printf("process_query - candidate_query is empty\n");
			
			m_ma_TB.put_mem((var_vd*)filter_buf);
			return -1;
		}
		
        // recommend_num(var_4),
        // recommend_list(var_u8...), recommend_power(var_f4...), recommend_time(var_4...),
        // history_num(var_4), history_list(var_u8...)
        
        var_1* calc_buf = (var_1*)m_ma_TB.get_mem();
        if(calc_buf == NULL)
        {
			printf("process_query - alloc calc_buf error\n");
			
            m_ma_TB.put_mem((var_vd*)filter_buf);
            return -1;
        }
        
        var_1* src = filter_buf;
        var_1* des = calc_buf + 12;
		
		*(var_4*)des = 4; // type
		des += 4;
        *(var_u8*)des = uid;
        des += 8;
        
        var_4 recommend_num = *(var_4*)src;
        src += 4;
		
		printf("process_query - recommend num %d\n", recommend_num);
		if(recommend_num <= 0)
		{
			printf("process_query - recommend num error, is zero\n");
			
			m_ma_TB.put_mem((var_vd*)filter_buf);
			m_ma_TB.put_mem((var_vd*)calc_buf);
			return 0;
		}
		
        *(var_4*)des = recommend_num;
        des += 4;
        
        var_u8* recommend_did = (var_u8*)src;
        
        memcpy(des, src, recommend_num << 3);
        des += recommend_num << 3;
        src += recommend_num << 3;
        
        var_f4* recommend_pwr = (var_f4*)src;
        src += recommend_num << 2;
        
        var_4* recommend_time = (var_4*)src;
        src += recommend_num << 2;
        
        var_4 history_num = *(var_4*)src;
        src += 4;
        
        *(var_4*)des = history_num;
        des += 4;
        
        memcpy(des, src, history_num << 3);
        des += history_num << 3;
        
        var_4 calc_len = (var_4)(des - calc_buf);
        *(var_4*)(calc_buf + 8) = calc_len - 12;
		
		assert(calc_len <= m_cfg.fw_max_task_size);
		
        FILE* log_file = NULL;
        if(log == 1)
        {
            var_1 log_name[256];
            sprintf(log_name, "%s/%lu_%ld_%lu.log", m_cfg.fw_debug_path, uid, time(NULL), cp_get_uuid());
            
            log_file = fopen(log_name, "w");
            if(log_file == NULL)
                printf("open log file %s failure\n", log_name);
        }
        
        *(FILE**)calc_buf = log_file;
        
        var_vd* handle = NULL;

        if(m_cm_query.put_message(calc_buf, calc_len, &handle))
        {
			printf("process_query - put_message error\n");

            if(log_file)
                fclose(log_file);
            
			m_ma_TB.put_mem((var_vd*)filter_buf);
            m_ma_TB.put_mem((var_vd*)calc_buf);
            return -1;
        }
        
        if(m_cm_query.get_message(calc_buf, m_cfg.fw_max_task_size, calc_len, &handle))
        {
			printf("process_query - get_message error\n");

            if(log_file)
                fclose(log_file);
            
            m_ma_TB.put_mem((var_vd*)filter_buf);
            m_ma_TB.put_mem((var_vd*)calc_buf);
            return -1;
        }
        
        // power_num(var_4), power_list(var_f4...)
        var_4   calc_num = *(var_4*)calc_buf;
        var_f4* calc_pwr = (var_f4*)(calc_buf + 4);
		
		if(calc_num != 0 && calc_num != recommend_num)
			assert(0);
		if(calc_num == 0)
			printf("process_query - all algorithm offline, warning\n");

        if(log_file)
            fprintf(log_file, "\ntime        : ");
        
        var_4  top_pos = 0;
        var_u8 one_time = 0;
        
        for(var_4 i = 0; i < calc_num; i++)
        {
            if(log_file)
                fprintf(log_file, "%22d", recommend_time[i]);
            
            if(recommend_time[i] < 0)
            {
                var_4  t_tmp =  recommend_time[top_pos];
                recommend_time[top_pos] = recommend_time[i];
                recommend_time[i] = t_tmp;
                
                var_f4 f_tmp = recommend_pwr[top_pos];
                recommend_pwr[top_pos] = recommend_pwr[i];
                recommend_pwr[i] = f_tmp;
                
                var_u8 d_tmp = recommend_did[top_pos];
                recommend_did[top_pos] = recommend_did[i];
                recommend_did[i] = d_tmp;
                
                top_pos++;
                continue;
            }
            
            one_time += recommend_time[i];
        }
        
        if(log_file)
            fprintf(log_file, "\nbasePower   : ");
        
        for(var_4 i = 0; i < calc_num; i++)
		{
            if(log_file)
                fprintf(log_file, "%22f", recommend_pwr[i]);
            
            recommend_pwr[i] = m_cfg.fw_filter_factor * recommend_pwr[i] + calc_pwr[i];
            
            if(top_pos <= i)
                recommend_pwr[i] = m_cfg.fw_algorithm_factor * recommend_pwr[i] + m_cfg.fw_time_factor * recommend_time[i] / one_time;
		}

        if(log_file)
        {
            fprintf(log_file, "\n");
            
            fprintf(log_file, "finalPower  : ");
            for(var_4 i = 0; i < calc_num; i++)
                fprintf(log_file, "%22f", recommend_pwr[i]);
            fprintf(log_file, "\n\n");
            
            fprintf(log_file, "itemBased   : %f\n", m_cfg.fw_calc_factor[0]);
            fprintf(log_file, "contentBased: %f\n", m_cfg.fw_calc_factor[1]);
            fprintf(log_file, "modelBased  : %f\n", m_cfg.fw_calc_factor[2]);
            fprintf(log_file, "minHash     : %f\n", m_cfg.fw_calc_factor[3]);
            fprintf(log_file, "basePower   : %f\n", m_cfg.fw_filter_factor);
            
            fclose(log_file);
        }

        qs_recursion_1k_1p<var_f4, var_u8>(top_pos, recommend_num - 1, recommend_pwr, recommend_did);
        
        if(recommend_num < request_num)
            request_num = recommend_num;
		
		var_u8 request_lst[64];
		for(var_4 i = 0; i < request_num; i++)
			request_lst[i] = recommend_did[recommend_num - i - 1];
		
        m_ma_TB.put_mem((var_vd*)filter_buf);
        m_ma_TB.put_mem((var_vd*)calc_buf);
		
//      cp_random_shuffle<var_u8>(request_lst, request_num);
        
		while(m_flags.add(uid))
			cp_sleep(1);
		
		m_cache.put(uid, request_lst, request_num);
		
		var_4 ret = m_flags.del(uid);
		assert(ret == 0);

        return 0;
    }
	
	var_4 process_feedback(TASK_NODE* node, var_u8 uid, var_4 push_num, var_u8* push_lst)
	{
        if(node)
        {
            var_1* ptr = node->parse_buf + 4;
            
            uid = *(var_u8*)ptr;
            ptr += 8;
            push_num = *(var_4*)ptr;
            ptr += 4;
            push_lst = (var_u8*)ptr;
        }

        if(save_log_pushed(uid, push_num, push_lst))
            printf("process_feedback - save_log_pushed failure\n");

        if(m_cfg.fw_sync_flag && push_num > 0)
        {
            m_sync_lock.lock();
            
            var_8 size = cp_get_file_size(m_sync_save);
            
            for(;; cp_sleep(2000))
            {
                FILE* fp = NULL;
                
                if(access(m_sync_save, 0))
                    fp = fopen(m_sync_save, "wb");
                else
                    fp = fopen(m_sync_save, "rb+");
                
                if(fp == NULL)
                {
                    printf("process_feedback - open %s error\n", m_sync_save);
                    continue;
                }
                
                try
                {
                    if(fwrite(&uid, 8, 1, fp) != 1)
                        throw 100;
                    if(fwrite(&push_num, 4, 1, fp) != 1)
                        throw 200;
                    if(fwrite(push_lst, push_num<<3, 1, fp) != 1)
                        throw 300;
                    if(fwrite(m_sync_flag, 8, 1, fp) != 1)
                        throw 400;
                }
                catch (var_4 error)
                {
                    printf("process_feedback - write %s error, code = %d\n", m_sync_save, error);
                    
                    cp_change_file_size(fp, size);
                    
                    fclose(fp);
                    continue;
                }
                
                fclose(fp);
                break;
            }
            
            m_sync_lock.unlock();
        }
        
        while(candidate_feedback(uid, push_num, push_lst))
        {
            printf("process_feedback - call candidate_feedback failure\n");
            cp_sleep(2000);
        }
        
        if(node)
            node->send_len = sprintf(node->send_buf, "{\"type\":%d,\"code\":0}", node->type_flg);

		return 0;
	}

    var_4 candidate_isNewUser(var_u8 uid)
    {
        var_1 recv_buf[16];
        var_4 recv_len = 16;
        
        var_1  send_buf[32];
        var_1* send_pos = send_buf + 4;
        
        *(var_4*)send_pos = 4; // query
        send_pos += 4;
        *(var_4*)send_pos = 3; // query type
        send_pos += 4;
        *(var_u8*)send_pos = uid;
        send_pos += 8;
        
        var_4 send_len = (var_4)(send_pos - send_buf);
        *(var_4*)send_buf = send_len - 4;
        
        if(candidate_query(send_buf, send_len, recv_buf, recv_len))
        {
            printf("candidate_isNewUser - call candidate_query error\n");
            return -1;
        }
        
        if(recv_len == 0 || *(var_4*)recv_buf == 1)
            return 1; // new user

        return 0; // old user
    }
    
    var_4 candidate_feedback(var_u8 uid, var_4 push_num, var_u8* push_lst)
    {
        var_1 recv_buf[16];
        var_4 recv_len = 16;
        
        var_1 send_buf[10240];
        var_4 send_len = 0;
        
        var_1* send_pos = send_buf + 4;
        
        *(var_4*)send_pos = 666; // type
        send_pos += 4;
        *(var_u8*)send_pos = uid;
        send_pos += 8;
        *(var_u4*)send_pos = push_num;
        send_pos += 4;
        memcpy(send_pos, push_lst, push_num << 3);
        send_pos += push_num << 3;
        
        send_len = (var_4)(send_pos - send_buf);
        *(var_4*)send_buf = send_len - 4;
        
        if(candidate_query(send_buf, send_len, recv_buf, recv_len))
        {
            printf("candidate_feedback - call candidate_query error\n");
            return -1;
        }
        
        if(*(var_4*)recv_buf)
        {
            printf("candidate_feedback - candidate return failure, %d\n", *(var_4*)recv_buf);
            return -1;
        }
        
        printf("candidate_feedback - candidate return success\n");
        
        return 0;
    }
    
    var_4 candidate_query(var_1* request_buf, var_4 request_len, var_1* result_buf, var_4& result_len)
    {
        CP_SOCKET_T client;
		
        try
        {
            if(cp_connect_socket(client, m_cfg.fw_filter_ip, m_cfg.fw_filter_port))
                throw -1;
            
            cp_set_overtime(client, 5000);
            
            if(cp_sendbuf(client, request_buf, request_len))
                throw -2;
            
            var_4 recv_len = 0;
            if(cp_recvbuf(client, (var_1*)&recv_len, 4))
                throw -3;
            
            if(recv_len > result_len)
                throw -4;
            
            result_len = recv_len;
            if(cp_recvbuf(client, result_buf, result_len))
                throw -5;
            
            cp_close_socket(client);
        }
        catch (var_4 err)
        {
            if(err < -1)
                cp_close_socket(client);
            
            return -1;
        }
		
        return 0;
    }
	
	var_4 save_log_action(TASK_NODE* node)
	{
		var_1 filename[256];
		
		struct tm now;
		cp_localtime(time(NULL), &now);
		
		sprintf(filename, "%s/%.4d%.2d%.2d%.2d%.2d00.log", m_cfg.fw_log_path_action, now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min - (now.tm_min % m_cfg.fw_log_recoder_interval));
		
        // <uid>\t<did>\t<publictime>\t<action>
        
        m_log_action_lock.lock();
        
		FILE* fp = NULL;
		
		if(access(filename, 0) == 0)
			fp = fopen(filename, "a");
		else
			fp = fopen(filename, "w");
		
		if(fp == NULL)
        {
            m_log_action_lock.unlock();
            
            return -1;
        }
		
        var_1* ptr = node->parse_buf;
        
		fprintf(fp, CP_PU64"\t" CP_PU64"\t%d\t%d\n", *(var_u8*)(ptr + 8), *(var_u8*)(ptr + 16), *(var_4*)(ptr + 4), *(var_4*)(ptr + 28));
		
		fclose(fp);
		
        m_log_action_lock.unlock();
        
		return 0;
	}
    
    var_4 save_log_document(TASK_NODE* node)
    {
        var_1 filename[256];
        
        struct tm now;
        cp_localtime(time(NULL), &now);
        
        sprintf(filename, "%s/%.4d%.2d%.2d%.2d%.2d00.log", m_cfg.fw_log_path_document, now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min - (now.tm_min % m_cfg.fw_log_recoder_interval));
        
        // <did>\t<time>\t<class_num>\t<class>\t<word_num>\t<word1>\t<count1>
        m_log_document_lock.lock();
        
        FILE* fp = NULL;
        
        if(access(filename, 0) == 0)
            fp = fopen(filename, "a");
        else
            fp = fopen(filename, "w");
        
        if(fp == NULL)
        {
            m_log_document_lock.unlock();
            
            return -1;
        }
        
        var_1* ptr = node->parse_buf + 4;
        
        fprintf(fp, CP_PU64"\t%d", *(var_u8*)ptr, *(var_4*)(ptr + 12));
        ptr += 20;

        var_4 num = *(var_4*)ptr;
        
        fprintf(fp, "\t%d", num);
        ptr += 4;
        
        for(var_4 i = 0; i < num; i++)
        {
            var_4 len = *(var_4*)ptr;
            ptr += 4;

            var_1 chr = ptr[len];
            ptr[len] = 0;
            fprintf(fp, "\t%s", ptr);
            ptr[len] = chr;

            ptr += len;
        }
        
        num = *(var_4*)ptr;
        
        fprintf(fp, "\t%d", num);
        ptr += 4;
        
        for(var_4 i = 0; i < num; i++)
        {
            var_4 len = *(var_4*)ptr;
            ptr += 4;
            
            var_1 chr = ptr[len];
            ptr[len] = 0;
            fprintf(fp, "\t%s", ptr);
            ptr[len] = chr;
            
            ptr += len;
            
            fprintf(fp, "\t%d", *(var_4*)ptr);
            ptr += 4;
        }
        
        fprintf(fp, "\n");
        
        fclose(fp);
        
        m_log_document_lock.unlock();

        return 0;
    }
    
    var_4 save_log_pushed(var_u8 uid, var_4 push_num, var_u8* push_lst)
    {
        var_1 filename[256];
        
        struct tm now;
        cp_localtime(time(NULL), &now);
        
        sprintf(filename, "%s/%.4d%.2d%.2d%.2d%.2d00.log", m_cfg.fw_log_path_pushed, now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min - (now.tm_min % m_cfg.fw_log_recoder_interval));
        
        // <uid>\t<time>\t<did1>\t<did2>
        
        m_log_pushed_lock.lock();
        
        FILE* fp = NULL;
        
        if(access(filename, 0) == 0)
            fp = fopen(filename, "a");
        else
            fp = fopen(filename, "w");
        
        if(fp == NULL)
        {
            m_log_pushed_lock.unlock();
            
            return -1;
        }
        
        fprintf(fp, CP_PU64 "\t%d", uid, (var_4)time(NULL));
        for(var_4 i = 0; i < push_num; i++)
            fprintf(fp, "\t" CP_PU64, push_lst[i]);
        fprintf(fp, "\n");
        
        fclose(fp);
        
        m_log_pushed_lock.unlock();

        return 0;
    }
    
    TASK_NODE* get_task_node()
    {
        var_1* buf = (var_1*)m_ma_TN.get_mem();
        while(buf == NULL)
        {
            cp_sleep(1);
            buf = (var_1*)m_ma_TN.get_mem();
        }
        
        TASK_NODE* node = (TASK_NODE*)buf;
        
        node->recv_buf = buf + sizeof(TASK_NODE);
        node->recv_len = 0;
        
        node->send_buf = node->recv_buf + m_cfg.fw_max_recv_size;
        node->send_len = 0;
        
        node->binary_buf = node->send_buf + m_cfg.fw_max_send_size;
        node->binary_len = 0;
        
        node->parse_buf = node->binary_buf + 4;
        node->parse_len = 0;
        node->parse_max = m_cfg.fw_max_binary_size - 4;
        
        return node;
    }
    
    var_vd put_task_node(TASK_NODE*& node)
    {
        node->recv_buf = NULL;
        node->send_buf = NULL;
        node->binary_buf = NULL;
        node->parse_buf = NULL;
        
        m_ma_TN.put_mem((var_vd*)node);
        
        node = NULL;
    }
    
    var_4 parse_array(cJSON* json, var_1*& cur_buf, var_4& cur_size, var_4 all_size)
    {
        var_4 num = cJSON_GetArraySize(json);
        if(num < 0)
            return -1;
        
        cur_size += 4;
        if(cur_size > all_size)
            return -2;
        
        *(var_4*)cur_buf = num;
        cur_buf += 4;
        
        for(var_4 i = 0; i < num; i++)
        {
            cJSON* node = cJSON_GetArrayItem(json, i);
            if(node == NULL)
                return -1;
            
            var_4 size = (var_4)strlen(node->valuestring);
            
            cur_size += 4;
            if(cur_size > all_size)
                return -2;
            
            *(var_4*)cur_buf = size;
            cur_buf += 4;
            
            cur_size += size;
            if(cur_size > all_size)
                return -2;
            
            memcpy(cur_buf, node->valuestring, size);
            cur_buf += size;
        }
        
        return 0;
    }
    
    var_4 parse_arrayObject(cJSON* json, var_1*& cur_buf, var_4& cur_size, var_4 all_size)
    {
        var_4 num = cJSON_GetArraySize(json);
        if(num < 0)
            return -1;
        
        cur_size += 4;
        if(cur_size > all_size)
            return -2;
        
        *(var_4*)cur_buf = num;
        cur_buf += 4;
        
        for(var_4 i = 0; i < num; i++)
        {
            cJSON* object = cJSON_GetArrayItem(json, i);
            if(object == NULL)
                return -1;
            
            cJSON* node = cJSON_GetObjectItem(object, "word");
            if(node == NULL)
                return -1;
            
            var_4 size = (var_4)strlen(node->valuestring);
            
            cur_size += 4;
            if(cur_size > all_size)
                return -2;
            
            *(var_4*)cur_buf = size;
            cur_buf += 4;
            
            cur_size += size;
            if(cur_size > all_size)
                return -2;
            
            memcpy(cur_buf, node->valuestring, size);
            cur_buf += size;
            
            node = cJSON_GetObjectItem(object, "count");
            if(node == NULL)
                return -1;
            
            cur_size += 4;
            if(cur_size > all_size)
                return -2;
            
            *(var_4*)cur_buf = node->valueint;
            cur_buf += 4;
        }
        
        return 0;
    }
    
    var_4 parse_action(cJSON* json, var_1* binary_buffer, var_4& binary_size)
    {
        var_4 num = cJSON_GetArraySize(json);
        if(num != 7)
            return -101;
        
        var_1* pos = binary_buffer;
        var_4  len = 0;
        
        cJSON* node = NULL;
        
        // type 4
        node = cJSON_GetObjectItem(json, "type");
        if(node == NULL)
            return -102;
        
        len += 4;
        if(len > binary_size)
            return -100;
        
        *(var_4*)pos = node->valueint;
        pos += 4;
        
        // clicktime 4
        node = cJSON_GetObjectItem(json, "clicktime");
        if(node == NULL)
            return -103;
        
        len += 4;
        if(len > binary_size)
            return -100;
        
        *(var_4*)pos = node->valueint;
        pos += 4;
        
        // userid 8
        node = cJSON_GetObjectItem(json, "userid");
        if(node == NULL)
            return -104;
        
        len += 8;
        if(len > binary_size)
            return -100;
        
        *(var_u8*)pos = cp_strtoval_u64(node->valuestring);
        pos += 8;
        
        // docid 8
        node = cJSON_GetObjectItem(json, "docid");
        if(node == NULL)
            return -105;
        
        len += 8;
        if(len > binary_size)
            return -100;
        
        *(var_u8*)pos = cp_strtoval_u64(node->valuestring);
        pos += 8;
        
        // staytime 4
        node = cJSON_GetObjectItem(json, "staytime");
        if(node == NULL)
            return -106;
        
        len += 4;
        if(len > binary_size)
            return -100;
        
        *(var_4*)pos = node->valueint;
        pos += 4;
        
        // action 4
        node = cJSON_GetObjectItem(json, "action");
        if(node == NULL)
            return -107;
        
        len += 4;
        if(len > binary_size)
            return -100;
        
        *(var_4*)pos = node->valueint;
        pos += 4;
        
        // location 4 + len
        node = cJSON_GetObjectItem(json, "location");
        if(node == NULL)
            return -108;
        
        len += 4;
        if(len > binary_size)
            return -100;
        
        var_4 size = (var_4)strlen(node->valuestring);
        *(var_4*)pos = size;
        pos += 4;
        
        len += size;
        if(len > binary_size)
            return -100;
        
        memcpy(pos, node->valuestring, size);
        pos += size;
        
        binary_size = len;
        
        return 0;
    }
    
    var_4 parse_document(cJSON* json, var_1* binary_buffer, var_4& binary_size)
    {
        var_4 num = cJSON_GetArraySize(json);
        if(num != 12)
            return -201;
        
        var_1* pos = binary_buffer;
        var_4  len = 0;
        
        cJSON* node = NULL;
        
        // type 4
        node = cJSON_GetObjectItem(json, "type");
        if(node == NULL)
            return -202;
        
        len += 4;
        if(len > binary_size)
            return -200;
        
        *(var_4*)pos = node->valueint;
        pos += 4;
        
        // docid 8
        node = cJSON_GetObjectItem(json, "docid");
        if(node == NULL)
            return -203;
        
        len += 8;
        if(len > binary_size)
            return -200;
        
        *(var_u8*)pos = cp_strtoval_u64(node->valuestring);
        pos += 8;
        
        // publictime 4
        node = cJSON_GetObjectItem(json, "publictime");
        if(node == NULL)
            return -212;
        
        len += 4;
        if(len > binary_size)
            return -200;
        
        *(var_4*)pos = node->valueint;
        pos += 4;
        
        // pushtime 4
        node = cJSON_GetObjectItem(json, "pushtime");
        if(node == NULL)
            return -213;
        
        len += 4;
        if(len > binary_size)
            return -200;
        
        *(var_4*)pos = (var_4)time(NULL);
//      *(var_4*)pos = node->valueint;
        pos += 4;
        
        // power 4
        node = cJSON_GetObjectItem(json, "power");
        if(node == NULL)
            return -214;
        
        len += 4;
        if(len > binary_size)
            return -200;
        
        *(var_f4*)pos = (var_f4)node->valuedouble;
        pos += 4;
        
        // category num(4) {size(4) string(size)}
        cJSON* array = cJSON_GetObjectItem(json, "category");
        if(array == NULL)
            return -208;
        
        var_4 ret_val = parse_array(array, pos, len, binary_size);
        if(ret_val == -1)
            return -209;
        else if(ret_val == -2)
            return -200;
        
        // word num(4) {size(4) string(size) power(4)}
        array = cJSON_GetObjectItem(json, "word");
        if(array == NULL)
            return -210;
        
        ret_val = parse_arrayObject(array, pos, len, binary_size);
        if(ret_val == -1)
            return -211;
        else if(ret_val == -2)
            return -200;
        
        // srp num(4) {size(4) string(size)}
        array = cJSON_GetObjectItem(json, "srp");
        if(array == NULL)
            return -204;
        
        ret_val = parse_array(array, pos, len, binary_size);
        if(ret_val == -1)
            return -205;
        else if(ret_val == -2)
            return -200;
        
        // circle num(4) {size(4) string(size)}
        array = cJSON_GetObjectItem(json, "circle");
        if(array == NULL)
            return -206;
        
        ret_val = parse_array(array, pos, len, binary_size);
        if(ret_val == -1)
            return -207;
        else if(ret_val == -2)
            return -200;

        // topflag 4
        node = cJSON_GetObjectItem(json, "topflag");
        if(node == NULL)
            return -215;
        
        len += 4;
        if(len > binary_size)
            return -200;
        
        *(var_4*)pos = node->valueint;
        pos += 4;
        
        // topsrp num(4) {size(4) string(size)}
        array = cJSON_GetObjectItem(json, "topsrp");
        if(array == NULL)
            return -216;
        
        ret_val = parse_array(array, pos, len, binary_size);
        if(ret_val == -1)
            return -217;
        else if(ret_val == -2)
            return -200;
        
        // topcircle num(4) {size(4) string(size)}
        array = cJSON_GetObjectItem(json, "topcircle");
        if(array == NULL)
            return -218;
        
        ret_val = parse_array(array, pos, len, binary_size);
        if(ret_val == -1)
            return -219;
        else if(ret_val == -2)
            return -200;
        
        binary_size = len;
        
        return 0;
    }
    
    var_4 parse_subscribe(cJSON* json, var_1* binary_buffer, var_4& binary_size)
    {
        var_4 num = cJSON_GetArraySize(json);
        if(num != 4)
            return -301;
        
        var_1* pos = binary_buffer;
        var_4  len = 0;
        
        cJSON* node = NULL;
        
        // type 4
        node = cJSON_GetObjectItem(json, "type");
        if(node == NULL)
            return -302;
        
        len += 4;
        if(len > binary_size)
            return -300;
        
        *(var_4*)pos = node->valueint;
        pos += 4;
        
        // userid 8
        node = cJSON_GetObjectItem(json, "userid");
        if(node == NULL)
            return -303;
        
        len += 8;
        if(len > binary_size)
            return -300;
        
        *(var_u8*)pos = cp_strtoval_u64(node->valuestring);
        pos += 8;
        
        // circle num(4) {size(4) string(size)}
        cJSON* array = cJSON_GetObjectItem(json, "circle");
        if(array == NULL)
            return -304;
        
        var_4 ret_val = parse_array(array, pos, len, binary_size);
        if(ret_val == -1)
            return -305;
        else if(ret_val == -2)
            return -300;
        
        // srp num(4) {size(4) string(size)}
        array = cJSON_GetObjectItem(json, "srp");
        if(array == NULL)
            return -306;
        
        ret_val = parse_array(array, pos, len, binary_size);
        if(ret_val == -1)
            return -307;
        else if(ret_val == -2)
            return -300;
        
        binary_size = len;
        
        return 0;
    }
    
    var_4 parse_recommend(cJSON* json, var_1* binary_buffer, var_4& binary_size)
    {
        var_4 num = cJSON_GetArraySize(json);
        if(num != 4)
            return -401;
        
        var_1* pos = binary_buffer;
        var_4  len = 0;
        
        cJSON* node = NULL;
        
        // type 4
        node = cJSON_GetObjectItem(json, "type");
        if(node == NULL)
            return -402;
        
        len += 4;
        if(len > binary_size)
            return -400;
        
        *(var_4*)pos = node->valueint;
        pos += 4;
        
        // logo 4
        node = cJSON_GetObjectItem(json, "log");
        if(node == NULL)
            return -403;
        
        len += 4;
        if(len > binary_size)
            return -400;
        
        *(var_4*)pos = node->valueint;
        pos += 4;
        
        // userid 8
        node = cJSON_GetObjectItem(json, "userid");
        if(node == NULL)
            return -404;
        
        len += 8;
        if(len > binary_size)
            return -400;
        
        *(var_u8*)pos = cp_strtoval_u64(node->valuestring);
        pos += 8;
        
        // requestnum 4
        node = cJSON_GetObjectItem(json, "requestnum");
        if(node == NULL)
            return -405;
        
        len += 4;
        if(len > binary_size)
            return -400;
        
        *(var_4*)pos = node->valueint;
        pos += 4;
        
        binary_size = len;
        
        return 0;
    }
    
    var_4 parse_heartbeat(cJSON* json, var_1* binary_buffer, var_4& binary_size)
    {
        var_4 num = cJSON_GetArraySize(json);
        if(num != 2)
            return -501;
        
        var_1* pos = binary_buffer;
        var_4  len = 0;
        
        cJSON* node = NULL;
        
        // type 4
        node = cJSON_GetObjectItem(json, "type");
        if(node == NULL)
            return -502;
        
        len += 4;
        if(len > binary_size)
            return -500;
        
        *(var_4*)pos = node->valueint;
        pos += 4;
        
        // heartbeat size(4) buffer(size)
        node = cJSON_GetObjectItem(json, "heartbeat");
        if(node == NULL)
            return -503;
        
        var_4 size = (var_4)strlen(node->valuestring);
        
        len += 4;
        if(len > binary_size)
            return -500;
        
        *(var_4*)pos = size;
        pos += 4;
        
        len += size;
        if(len > binary_size)
            return -500;
        
        memcpy(pos, node->valuestring, size);
        pos += size;
        
        binary_size = len;
        
        return 0;
    }
    
    var_4 parse_feedback(cJSON* json, var_1* binary_buffer, var_4& binary_size)
    {
        var_4 num = cJSON_GetArraySize(json);
        if(num != 4)
            return -601;
        
        var_1* pos = binary_buffer;
        var_4  len = 0;
        
        cJSON* node = NULL;
        
        // type 4
        node = cJSON_GetObjectItem(json, "type");
        if(node == NULL)
            return -602;
        
        len += 4;
        if(len > binary_size)
            return -600;
        
        *(var_4*)pos = node->valueint;
        pos += 4;
        
        // userid 8
        node = cJSON_GetObjectItem(json, "userid");
        if(node == NULL)
            return -603;
        
        len += 8;
        if(len > binary_size)
            return -600;
        
        *(var_u8*)pos = cp_strtoval_u64(node->valuestring);
        pos += 8;
        
        // returnnum 4
        node = cJSON_GetObjectItem(json, "returnnum");
        if(node == NULL)
            return -604;
        
        len += 4;
        if(len > binary_size)
            return -600;
        
        *(var_4*)pos = node->valueint;
        pos += 4;
        
        // returnlist num(4) docid(8)
        cJSON* array = cJSON_GetObjectItem(json, "returnlist");
        if(array == NULL)
            return -605;
        
        num = cJSON_GetArraySize(array);
        if(num < 0)
            return -606;
        
        len += 4;
        if(len > binary_size)
            return -600;
        
        *(var_4*)pos = num;
        pos += 4;
        
        for(var_4 i = 0; i < num; i++)
        {
            node = cJSON_GetArrayItem(array, i);
            if(node == NULL)
                return -607;
            
            len += 8;
            if(len > binary_size)
                return -600;
            
            *(var_u8*)pos = cp_strtoval_u64(node->valuestring);
            pos += 8;
        }
        
        binary_size = len;
        
        return 0;
    }
    
    var_4 parse_json(var_1* json_buffer, var_1* binary_buffer, var_4& binary_size)
    {
        cJSON* json = cJSON_Parse(json_buffer);
        if(json == NULL)
        {
            printf("parse_json - parse error:\n%s\n", json_buffer);
            return -1;
        }
        
        cJSON* node = cJSON_GetObjectItem(json, "type");
        if(node == NULL)
        {
            cJSON_Delete(json);
            
            printf("parse_json - no find type:\n%s\n", json_buffer);
            return -1;
        }
        
        var_4 type = node->valueint;
        var_4 code = -888;
        
        printf("parse_json - type value %d\n", type);
        
        switch (type)
        {
            case 1:
                code = parse_action(json, binary_buffer, binary_size);
                break;
                
            case 2:
                code = parse_document(json, binary_buffer, binary_size);
                break;
                
            case 3:
                code = parse_subscribe(json, binary_buffer, binary_size);
                break;
                
            case 4:
                code = parse_recommend(json, binary_buffer, binary_size);
                break;
                
            case 5:
                code = parse_heartbeat(json, binary_buffer, binary_size);
                break;
                
            case 6:
                code = parse_feedback(json, binary_buffer, binary_size);
                break;
                
            default:
                printf("parse_json - type value error:\n%s\n", json_buffer);
                break;
        }
        
        cJSON_Delete(json);
        
        return code;
    }
    
public:
    CP_SOCKET_T m_listen;
    
    CENTER_CONFIG m_cfg;
    
    UC_MD5 m_md5;
    
    UT_Queue<TASK_NODE*>    m_taskQueue;
	UC_Queue_VL             m_flushCache;
    
    UC_Mem_Allocator_Recycle m_ma_TN; // task node
    UC_Mem_Allocator_Recycle m_ma_TB; // task buffer
    
    UC_CenterMessage    m_cm_query;
    ICM_Query_Processor m_cm_query_processor;
    
    UC_CenterMessage     m_cm_update;
    ICM_Update_Processor m_cm_update_processor;
		
	CF_framework_cache		 m_cache;
	UT_HashTable_Pro<var_u8> m_flags;
    
    var_1  m_sync_save[256];
    var_1  m_sync_send[256];
    var_1* m_sync_flag;
    
    CP_MUTEXLOCK m_sync_lock;

    CP_MUTEXLOCK m_log_action_lock;
    CP_MUTEXLOCK m_log_document_lock;
    CP_MUTEXLOCK m_log_pushed_lock;
};

#endif // _CF_FRAMEWORK_CENTER_H_
