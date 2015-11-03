//
//  CF_framework_cache.h
//  CollaborativeFiltering
//
//  Created by zhanghl on 14/10/27.
//  Copyright (c) 2014年 CrystalBall. All rights reserved.
//

#ifndef _CF_FRAMEWORK_CACHE_H_
#define _CF_FRAMEWORK_CACHE_H_

#include "utils/UH_Define.h"
#include "utils/UT_HashTable_Pro.h"

#include "CF_framework_config.h"

class CF_framework_cache
{
public:
	var_4 init(var_1* save_path, var_4 cache_capacity, var_4 cache_count, var_4 flush_count, var_4 flush_time, var_4 cache_debug)
	{
		strcpy(m_save_path, save_path);
		
		m_cache_capacity = cache_capacity;
		m_cache_count = cache_count;
		
		m_flush_count = flush_count;
		m_flush_time = flush_time;
		
        m_return_count = 4;
        for(var_4 i = 0; i < m_return_count; i++)
            m_return_value[i] = i + 3;
        
        srand((var_4)time(NULL));
        
		var_1 name_sto[256];
		sprintf(name_sto, "%s/cache.sto", save_path);
		var_1 name_inc[256];
		sprintf(name_inc, "%s/cache.inc", save_path);
		var_1 name_flg[256];
		sprintf(name_flg, "%s/cache.flg", save_path);
		
		// sendTime(4), updateTime(4), num(4), id(8) * N
		if(m_cache.init(m_cache_capacity, m_cache_capacity, 12 + (m_cache_count << 3), name_sto, name_inc, name_flg))
			return -1;
		
        if(cache_debug)
            m_debug_log = fopen("cache_debug.log", "w");
        
		return 0;
	}

	var_4 get(var_u8 uid, var_4 req_num, var_u8* ret_lst, var_4& ret_num)
	{
		ret_num = 0;
		
		var_vd* ref = NULL;
		
		if(m_cache.pop_value(uid, ref))
			return -1;
		
		var_4*  tmp = (var_4*)ref;
		var_4   stm = *tmp++; // search time
		var_4   utm = *tmp++; // update time
		var_4   num = *tmp++; // remain count
		var_u8* lst = (var_u8*)tmp;
        
        var_4 is_flush;
        var_4 remain_count = 0;
        var_1 buffer[2048];
        
        var_4 now = (var_4)time(NULL);
        
        if(now - utm < m_flush_time)
        {
            // calc return count
            var_4 rand_num = m_return_value[rand() % m_return_count];
            
            if(rand_num < req_num)
                ret_num = rand_num;
            else
                ret_num = req_num;
            
            if(num < ret_num)
                ret_num = num;
            
            // copy return id
            memcpy(ret_lst, lst, ret_num << 3);
            
            // modify cache buffer
            remain_count = num - ret_num;
            
            *(var_4*)buffer = stm;
            *(var_4*)(buffer + 4) = utm;
            *(var_4*)(buffer + 8) = remain_count;
            memcpy(buffer + 12, lst + ret_num, remain_count << 3);
            
            // flush cache ?
            if(remain_count <= m_flush_count)
            {
                if(now - stm > 60)
                {
                    is_flush = 1;
                    *(var_4*)buffer = now;
                }
            }
            else
                is_flush = 2;
        }
        else
        {
            memset(buffer, 0, 12);
            if(now - stm > 60)
            {
                is_flush = 3;
                *(var_4*)buffer = now;
            }
            else
            {
                is_flush = 4;
                *(var_4*)buffer = stm;
            }
        }
        
		m_cache.push_value(ref);
		
		if(m_cache.add(uid, (var_vd*)buffer, NULL, 1) < 0)
			printf("CF_framework_cache - get: update cache failure\n");
		
		return is_flush;
	}
	
	var_4 put(var_u8 uid, var_u8* lst, var_4 num)
	{
		var_1 buffer[1024];
		
		*(var_4*)buffer = 0;
		*(var_4*)(buffer + 4) = (var_4)time(NULL);
		*(var_4*)(buffer + 8) = num;
		memcpy(buffer + 12, lst, num << 3);
		
        if(m_debug_log)
        {
            time_t now = time(NULL);
            fprintf(m_debug_log, "%s, %lu(%d)\t", ctime(&now), uid, num);
            for(var_4 i = 0; i < num; i++)
                fprintf(m_debug_log, "%lu  ", lst[i]);
            fprintf(m_debug_log, "\n");
            
            fflush(m_debug_log);
        }
        
		if(m_cache.add(uid, (var_vd*)buffer, NULL, 1))
			return -1;
		
		return 0;
	}
	
	static CP_THREAD_T thread_trim(var_vd* argv)
	{
		CF_framework_cache* cc = (CF_framework_cache*)argv;
		
		for(;;)
		{
			for(;; cp_sleep(10000))
			{
				time_t now_sec = (time(NULL) + 3600 * 8) % 86400;
				if(now_sec > 600) // 0:00 - 0:10 
					continue;
				break;
			}
			
			while(cc->m_cache.trim())
			{
				cp_sleep(5000);
				printf("CF_framework_cache.thread_trim.trim error\n");
			}
			
			cp_sleep(60000 * 12);
		}
		
		return 0;
	}
	
public:
	var_1 m_save_path[256];
	
	var_4 m_cache_capacity;
	var_4 m_cache_count;
	
	var_4 m_flush_count;
	var_4 m_flush_time;
    
    var_4 m_return_value[4];
    var_4 m_return_count;
	
	UT_HashTable_Pro<var_u8> m_cache;
    
    FILE* m_debug_log;
};

#endif // _CF_FRAMEWORK_CACHE_H_
