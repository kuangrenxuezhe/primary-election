//
//  UC_DiskQuotaManager.h
//  code_library
//
//  Created by zhanghl on 12-11-28.
//  Copyright (c) 2012年 zhanghl. All rights reserved.
//

#ifndef _UC_DISKQUOTAMANAGER_H_
#define _UC_DISKQUOTAMANAGER_H_

#include "UH_Define.h"
#include "UT_Queue.h"
#include "UT_Sort.h"

#define DQM_MAX_STORE_PATH_COUNT    1025
#define DQM_MAX_PAYLOAD_SIZE        24
#define DQM_DATA_END_FLAG           "DQM_FLAG"
#define DQM_DATA_END_FLAG_SIZE      8

// #define DQM_QUICK_READ_DATA

class UC_DiskQuotaManager
{
public:
    //////////////////////////////////////////////////////////////////////
    // 函数:      init
    // 功能:      初始化磁盘配额管理器
    // 入参:
    //            in_sto_path_num: 磁盘存储系统可以用来保存库文件的路径列表个数
    //                in_sto_path: 磁盘存储系统可以用来保存库文件的路径名列表(需要以'/'结尾)
    //          in_sto_total_size: 磁盘存储系统对应每个库路径下可以使用的磁盘空间(以GB为单位)
    //            in_sto_one_size: 磁盘存储系统中每个库文件的大小(以GB为单位)
    //          in_max_write_size: 对一次请求的最大写入长度
    //     in_max_read_handle_num: 一个文件的最大并发读取句柄数
    //                  fun_judge: 整理时判断是否删除的回调函数
    //                 fun_update: 整理时更新内存索引的回调函数
    //
    // 出参:
    // 返回值:    成功返回0,否则返回错误码
    // 备注:
    //////////////////////////////////////////////////////////////////////
    var_4 dqm_init(var_4 in_sto_path_num, var_1** in_sto_path, var_4* in_sto_total_size, var_4 in_sto_one_size, var_4 in_max_write_size, var_4 in_max_read_handle_num, var_4 (*fun_judge)(var_4 file_no, var_8 file_offset, var_4& key_len, var_1*& key_buf, var_vd* argv), var_4 (*fun_update)(var_4 file_no, var_8 file_offset, var_4& key_len, var_1*& key_buf, var_vd* argv), var_vd* fun_argv = NULL, var_4 is_clear = 0)
    {
        if(in_sto_path_num > DQM_MAX_STORE_PATH_COUNT)
            return -1;
        
        m_fun_judge = fun_judge;
        m_fun_update = fun_update;
        
        m_fun_argv = fun_argv;
        
        m_sf_add_count = 0;
        
        m_flg_del = 0;
        m_flg_use = 1;
        
        m_max_write_size = in_max_write_size + DQM_MAX_PAYLOAD_SIZE;
        m_max_read_handle_num = in_max_read_handle_num;
        
        m_sf_inP_count = in_sto_path_num;
        m_sf_max_size = in_sto_one_size * (1LL<<30);
        m_sf_all_count = 0;
        
        if(m_max_write_size > m_sf_max_size)
            return -1;
        
        for(var_4 i = 0; i < in_sto_path_num; i++)
        {
            if(cp_create_dir(in_sto_path[i]))
                return -1;
            
            m_sf_inP_allnum[i] = (in_sto_total_size[i] + in_sto_one_size - 1) / in_sto_one_size;
            m_sf_all_count += m_sf_inP_allnum[i];
        }
        
		if(is_clear && dqm_clear(in_sto_path_num, in_sto_path))
			return -1;
		
        m_sf_cur_size = new var_8[m_sf_all_count];
        if(m_sf_cur_size == NULL)
            return -1;
        memset(m_sf_cur_size, -1, m_sf_all_count<<3);
        
        m_sf_tmp_size = new var_8[m_sf_all_count];
        if(m_sf_tmp_size == NULL)
            return -1;
        memset(m_sf_tmp_size, -1, m_sf_all_count<<3);
        
        m_sf_index_no = new var_4[m_sf_all_count];
        if(m_sf_index_no == NULL)
            return -1;
        memset(m_sf_index_no, -1, m_sf_all_count<<2);
        
        m_sf_use_flag = new var_4[m_sf_all_count];
        if(m_sf_use_flag == NULL)
            return -1;
        memset(m_sf_use_flag, 0, m_sf_all_count<<2);
        
        m_sf_name_lst = new var_1*[m_sf_all_count];
        if(m_sf_name_lst == NULL)
            return -1;
        for(var_4 i = 0; i < m_sf_all_count; i++)
        {
            m_sf_name_lst[i] = new var_1[256];
            if(m_sf_name_lst[i] == NULL)
                return -1;
        }
        
#ifdef DQM_QUICK_READ_DATA
		m_sf_memory_read = new UT_Queue<var_1*>[m_sf_all_count];
		if(m_sf_memory_read == NULL)
			return -1;
#else
		m_sf_file_read = new UT_Queue<var_4>[m_sf_all_count];
        if(m_sf_file_read == NULL)
            return -1;
#endif
		
		m_sf_file_write = new var_4[m_sf_all_count];
        if(m_sf_file_write == NULL)
            return -1;
		
        for(var_4 i = 0; i < in_sto_path_num; i++)
        {
            var_1   file_name[256];
            var_vd* handle = NULL;
            
            m_sf_inP_curnum[i] = 0;
            
            if(cp_dir_open(handle, in_sto_path[i]))
                return -1;
            while(cp_dir_travel(handle, file_name) == 0)
            {
                if(*(var_u8*)file_name != *(var_u8*)"dqm_sto_")
                    continue;
                
                var_4 file_no = (var_4)cp_strtoval_64(file_name + 8);
                if(file_no >= m_sf_all_count)
                    return -1;
                
                if(m_sf_index_no[file_no] != -1)
                    return -1;
                
                m_sf_index_no[file_no] = file_no;
                sprintf(m_sf_name_lst[file_no], "%s/%s", in_sto_path[i], file_name);
                
                if(cp_fix_file(m_sf_name_lst[file_no], DQM_DATA_END_FLAG, DQM_DATA_END_FLAG_SIZE, m_sf_cur_size + file_no))
                    return -1;
                
                m_sf_inP_curnum[i]++;
            }
            cp_dir_close(handle);
        }
        
        m_sf_cur_count = 0;
        
        for(var_4 i = 0; i < in_sto_path_num; i++)
        {
            for(var_4 j = m_sf_inP_curnum[i]; j < m_sf_inP_allnum[i]; j++)
            {
                while(m_sf_cur_count < m_sf_all_count && m_sf_index_no[m_sf_cur_count] >= 0)
                    m_sf_cur_count++;
                
                if(m_sf_cur_count >= m_sf_all_count)
                    return -1;
                
                m_sf_index_no[m_sf_cur_count] = m_sf_cur_count;
                sprintf(m_sf_name_lst[m_sf_cur_count], "%s/dqm_sto_%.5d.lib", in_sto_path[i], m_sf_cur_count);
                
                m_sf_cur_size[m_sf_cur_count] = -1;
                
                m_sf_cur_count++;
            }
        }
        
        for(var_4 i = 0; i < m_sf_all_count; i++)
        {
			
#ifdef DQM_QUICK_READ_DATA
			if(m_sf_memory_read[i].InitQueue(m_max_read_handle_num))
				return -1;
#else
			if(m_sf_file_read[i].InitQueue(m_max_read_handle_num))
                return -1;
#endif
			
            if(m_sf_cur_size[i] == -1)
            {
                m_sf_cur_size[i] = 0;
                
				m_sf_file_write[i] = open(m_sf_name_lst[i], O_WRONLY | O_CREAT | O_APPEND , 0664);
				if(m_sf_file_write <= 0)
                    return -1;
                
            }
            else
            {
				m_sf_file_write[i] = open(m_sf_name_lst[i], O_WRONLY | O_CREAT | O_APPEND, 0664);
				if(m_sf_file_write[i] <= 0)
                    return -1;
            }
            
#ifdef DQM_QUICK_READ_DATA
			var_8  size = cp_get_file_size(m_sf_name_lst[i]);
			var_1* buff = new var_1[size];
			assert(buff);
			
			FILE* file = fopen(m_sf_name_lst[i], "rb");
			assert(file);
			assert(fread(buff, size, 1, file) == 1);
			fclose(file);
			
			for(var_4 j = 0; j < m_max_read_handle_num; j++)
				m_sf_memory_read[i].PushData(buff);
#else
            for(var_4 j = 0; j < m_max_read_handle_num; j++)
            {
				var_4 fp = open(m_sf_name_lst[i], O_RDONLY);
				if(fp <= 0)
                    return -1;
                m_sf_file_read[i].PushData(fp);
            }
#endif
        }
        
        memcpy(m_sf_tmp_size, m_sf_cur_size, m_sf_all_count<<3);
        
        qs_recursion_1k_1p<var_8, var_4>(0, m_sf_all_count - 1, m_sf_tmp_size, m_sf_index_no);
        
        for(m_sf_cur_count = 0; m_sf_cur_count < m_sf_all_count; m_sf_cur_count++)
        {
            if(m_sf_max_size - m_sf_tmp_size[m_sf_cur_count] >= m_max_write_size)
                continue;
            
            break;
        }
        
        return 0;
    }
    
	var_4 dqm_clear(var_4 in_sto_path_num, var_1** in_sto_path)
	{
		for(var_4 i = 0; i < in_sto_path_num; i++)
        {
            var_1   file_name[256];
            var_vd* handle = NULL;
            
            if(cp_dir_open(handle, in_sto_path[i]))
                return -1;
            while(cp_dir_travel(handle, file_name) == 0)
            {
                if(*(var_u8*)file_name != *(var_u8*)"dqm_sto_")
                    continue;
                
				var_1 remove_name[256];
				sprintf(remove_name, "%s/%s", in_sto_path[i], file_name);
				
				if(remove(remove_name))
				{
					printf("UC_DiskQuotaManager.dqm_clear.remove %s failure\n", remove_name);
					return -1;
				}
            }
            cp_dir_close(handle);
        }
        
		return 0;
	}
	
    var_4 dqm_start_trim()
    {
#ifdef DQM_QUICK_READ_DATA
		return -1;
#endif
		
        if(cp_create_thread(thread_trim, this))
            return -1;
        
        return 0;
    }
    
	inline var_vd dqm_index2key(var_4 file_no, var_8 file_offset, var_u8& key)
	{
		key = file_no;
		key <<= 40;
		key |= file_offset;
	}
	
	inline var_vd dqm_key2index(var_4& file_no, var_8& file_offset, var_u8 key)
	{
		file_no = (var_4)(key >> 40);
		file_offset = key & 0xFFFFFFFFFF;
	}
	
	var_4 dqm_write_data(var_u8& out_key, var_4 in_one_buf_len, var_1* in_one_buf, var_4 in_two_buf_len = 0, var_1* in_two_buf = NULL)
	{
		var_4 file_no = 0;
		var_8 file_offset = 0;
		
		var_4 ret_val = dqm_write_data(file_no, file_offset, in_one_buf_len, in_one_buf, in_two_buf_len, in_two_buf);
		
		out_key = -1;
		if(ret_val == 0)
		{
			out_key = file_no;
			out_key <<= 40;
			out_key |= file_offset;
		}
        
		return ret_val;
	}
	
    var_4 dqm_write_data(var_4& out_file_no, var_8& out_file_offset, var_4 in_one_buf_len, var_1* in_one_buf, var_4 in_two_buf_len = 0, var_1* in_two_buf = NULL)
    {
        // flag(1) total_size(4) one_buf_len(4) one_buf two_buf_len(4) two_buf end_flag(8)
        
#ifdef DQM_QUICK_READ_DATA
		return -1;
#endif
        
        var_4 all_size = 0;
        if(in_one_buf_len > 0)
            all_size += in_one_buf_len + 4;
        if(in_two_buf_len > 0)
        {
            if(all_size <= 0)
                return -1;
            
            all_size += in_two_buf_len + 4;
        }
        
        var_4 write_size = all_size + 13;
        
        m_sf_travel_lock.lock_r();
        
        //
        var_u8 add_count = cp_add_and_fetch(&m_sf_add_count);
        
        for(;;)
        {
            m_sf_quota_lock.lock();
            
            if(m_sf_cur_count <= 0)
            {
                m_sf_quota_lock.unlock();
                
                m_sf_travel_lock.unlock();
                
                return 1;
            }
            
            out_file_no = m_sf_index_no[add_count % m_sf_cur_count];
            
            if(m_sf_use_flag[out_file_no] != 0)
            {
                m_sf_quota_lock.unlock();
                continue;
            }
            
            m_sf_use_flag[out_file_no] = 1;
            
            m_sf_quota_lock.unlock();
            
            break;
        }
        
        //
        out_file_offset = m_sf_cur_size[out_file_no];
        
        try
        {
            if(write(m_sf_file_write[out_file_no], &m_flg_use, 1) != 1)
                throw -2;
            if(all_size > 0)
            {
				if(write(m_sf_file_write[out_file_no], &all_size, 4) != 4)
                    throw -3;
				if(write(m_sf_file_write[out_file_no], &in_one_buf_len, 4) != 4)
                    throw -4;
				if(write(m_sf_file_write[out_file_no], in_one_buf, in_one_buf_len) != in_one_buf_len)
                    throw -5;
                if(in_two_buf_len > 0)
                {
					if(write(m_sf_file_write[out_file_no], &in_two_buf_len, 4) != 4)
                        throw -6;
					if(write(m_sf_file_write[out_file_no], in_two_buf, in_two_buf_len) != in_two_buf_len)
                        throw -7;
                }
            }
			if(write(m_sf_file_write[out_file_no], DQM_DATA_END_FLAG, DQM_DATA_END_FLAG_SIZE) != DQM_DATA_END_FLAG_SIZE)
                throw -8;
        }
        catch (var_4 error)
        {
            for(;;)
            {
                printf("DQM.dqm_write_data write data to %s error, code = %d\n", m_sf_name_lst[out_file_no], error);
                cp_sleep(5000);
            }
            
            m_sf_quota_lock.lock();
            m_sf_use_flag[out_file_no] = 0;
            m_sf_quota_lock.unlock();
        }
        
        //
        m_sf_cur_size[out_file_no] += write_size;
        
        m_sf_quota_lock.lock();
        
        m_sf_use_flag[out_file_no] = 0;
        
        if(m_sf_max_size - m_sf_cur_size[out_file_no] < m_max_write_size)
        {
            var_4 idx_no = 0;
            for(; idx_no < m_sf_cur_count; idx_no++)
            {
                if(m_sf_index_no[idx_no] == out_file_no)
                    break;
            }
            
            assert(idx_no != m_sf_cur_count);
            
            var_4 tmp_no = m_sf_index_no[idx_no];
            m_sf_index_no[idx_no] = m_sf_index_no[m_sf_cur_count - 1];
            m_sf_index_no[m_sf_cur_count - 1] = tmp_no;
            
            m_sf_cur_count--;
        }
        
        m_sf_quota_lock.unlock();
        
        m_sf_travel_lock.unlock();
        
        return 0;
    }
    
	var_4 dqm_read_data(var_u8 in_key, var_4& io_one_buf_len, var_1* out_one_buf, var_4& io_two_buf_len, var_1* out_two_buf)
	{
		var_4 file_no = (var_4)(in_key >> 40);
		var_8 file_offset = in_key & 0xFFFFFFFFFF;
        
		return dqm_read_data(file_no, file_offset, io_one_buf_len, out_one_buf, io_two_buf_len, out_two_buf);
	}
	
    var_4 dqm_read_data(var_4 in_file_no, var_8 in_file_offset, var_4& io_one_buf_len, var_1* out_one_buf, var_4& io_two_buf_len, var_1* out_two_buf)
    {
        // flag(1) total_size(4) one_buf_len(4) one_buf two_buf_len(4) two_buf end_flag(8)
        
        var_1 buffer[9];
        
#ifdef DQM_QUICK_READ_DATA
		var_1* org_file = m_sf_memory_read[in_file_no].PopData();
		var_1* mem_file = org_file;
#else
		var_4 fp = m_sf_file_read[in_file_no].PopData();
#endif
		
        try
        {
#ifdef DQM_QUICK_READ_DATA
			mem_file += in_file_offset;
			memcpy(buffer, mem_file, 9);
			mem_file += 9;
#else
			if(-1 == lseek(fp, in_file_offset, SEEK_SET))
                throw -1;
			if(read(fp, buffer, 9) != 9)
                throw -2;
#endif
			
            if(*buffer == m_flg_del)
                throw 1;
            
			if(*buffer != m_flg_del && *buffer != m_flg_use)
				throw -500;
            
			var_4 cur_size = *(var_4*)(buffer + 5);
			
			if(cur_size < 0)
				throw -600;
            
			if(cur_size > io_one_buf_len)
				throw -100;
			
            io_one_buf_len = cur_size;
			
#ifdef DQM_QUICK_READ_DATA
			memcpy(out_one_buf, mem_file, io_one_buf_len);
			mem_file += io_one_buf_len;
#else
			if(read(fp, out_one_buf, io_one_buf_len) != io_one_buf_len)
                throw -3;
#endif
            
            if(*(var_4*)(buffer + 1) - 4 != io_one_buf_len)
            {
#ifdef DQM_QUICK_READ_DATA
				cur_size = *(var_4*)mem_file;
				mem_file += 4;
#else
				if(read(fp, &cur_size, 4) != 4)
                    throw -4;
#endif
				
				if(cur_size < 0)
					throw -700;
				
				if(cur_size > io_two_buf_len)
					throw -200;
				
				io_two_buf_len = cur_size;
				
#ifdef DQM_QUICK_READ_DATA
				memcpy(out_two_buf, mem_file, io_two_buf_len);
				mem_file += io_two_buf_len;
#else
				if(read(fp, out_two_buf, io_two_buf_len) != io_two_buf_len)
                    throw -5;
#endif
            }
        }
        catch (var_4 error)
        {
#ifdef DQM_QUICK_READ_DATA
			m_sf_memory_read[in_file_no].PushData(org_file);
#else
            m_sf_file_read[in_file_no].PushData(fp);
#endif
			
            if(error < 0)
                return error;
            else
                return 1;
        }
        
#ifdef DQM_QUICK_READ_DATA
		m_sf_memory_read[in_file_no].PushData(org_file);
#else
		m_sf_file_read[in_file_no].PushData(fp);
#endif
		
        return 0;
    }
/*
    var_4 dqm_delete_data(var_u8 in_key)
    {
        var_4 file_no = (var_4)(in_key >> 40);
        var_8 file_offset = in_key & 0xFFFFFFFFFF;
        
        return dqm_delete_data(file_no, file_offset);
    }
    
    var_4 dqm_delete_data(var_4 in_file_no, var_8 in_file_offset)
    {
        m_sf_quota_lock.lock();
        m_sf_use_flag[in_file_no] = 1;
        m_sf_quota_lock.unlock();
        
        try
        {
            if(fseek(m_sf_file_write[in_file_no], in_file_offset, SEEK_SET))
                throw -1;
            if(fwrite(&m_flg_del, 1, 1, m_sf_file_write[in_file_no]) != 1)
                throw -2;
        }
        catch (var_4 error)
        {
            m_sf_quota_lock.lock();
            m_sf_use_flag[in_file_no] = 0;
            m_sf_quota_lock.unlock();
            
            return -1;
        }
        
        m_sf_quota_lock.lock();
        m_sf_use_flag[in_file_no] = 0;
        m_sf_quota_lock.unlock();
        
        return 0;
    }
*/
    var_4 dqm_travel(var_4 (*fun_travel)(var_4 file_no, var_8 file_offset, var_4 key_len, var_1* key_buf, var_4 data_len, var_1* data_buf, var_vd* fun_argv), var_vd* fun_argv)
    {
        var_4  one_len = 0;
        var_1* one_buf = new var_1[m_max_write_size];
        if(one_buf == NULL)
        {
            printf("DQM.dqm_travel alloc memory error\n");
            return 0;
        }
        
        var_4  two_len = 0;
        var_1* two_buf = new var_1[m_max_write_size];
        if(two_buf == NULL)
        {
            printf("DQM.dqm_travel alloc memory error\n");
            return 0;
        }
        
        m_sf_travel_lock.lock_w();
        
        var_1 head[9];
        
        for(var_4 i = 0; i < m_sf_all_count; i++)
        {
            var_4 idx_no = m_sf_index_no[i];
            
            var_4 fp = m_sf_file_read[idx_no].PopData();
            
            if(-1 == lseek(fp, 0, SEEK_SET))
            {
                delete one_buf;
                delete two_buf;
                
                m_sf_travel_lock.unlock();
                
                return -100;
            }

            var_8 offset = 0;
            var_8 offset_beg = 0;
            var_8 offset_end = 0;
            
            for(;;)
            {
                try
                {
                    offset_beg = offset_end;
                    
                    // flag(1) total_size(4) one_buf_len(4) one_buf two_buf_len(4) two_buf end_flag(8)
                    if(read(fp, head, 9) != 9)
                        break;
                    
                    one_len = *(var_4*)(head + 5);
                    if(read(fp, one_buf, one_len) != one_len)
                        throw -1;
                    
                    offset_end += 9 + one_len;
                    
                    if(*(var_4*)(head + 1) - 4 != one_len)
                    {
                        if(read(fp, &two_len, 4) != 4)
                            throw -2;
                        if(read(fp, two_buf, two_len) != two_len)
                            throw -3;
                        
                        offset_end += 4 + two_len;
                    }
                    else
                        two_len = 0;
                    
                    if(-1 == lseek(fp, DQM_DATA_END_FLAG_SIZE, SEEK_CUR))
                        throw -4;
                    
                    offset_end += DQM_DATA_END_FLAG_SIZE;
                    
                    fun_travel(idx_no, offset_beg, one_len, one_buf, two_len, two_buf, fun_argv);
                }
                catch (var_4 error)
                {
                    for(;;)
                    {
                        printf("DQM.dqm_travel read %s error, code = %d, offset=0x%x\n", m_sf_name_lst[idx_no], error, offset_end);
                        cp_sleep(5000);
                    }
                }
            }
            
            m_sf_file_read[idx_no].PushData(fp);
            
            printf("DQM.dqm_travel %d / %d, %d ok\n", i, m_sf_all_count, idx_no);
        }
        
        delete one_buf;
        delete two_buf;
        
        m_sf_travel_lock.unlock();
        
        printf("DQM.dqm_travel all ok\n");
        
        return 0;
    }
    
	static CP_THREAD_T thread_trim(var_vd* argv)
    {
        UC_DiskQuotaManager* dqm = (UC_DiskQuotaManager*)argv;
        
        var_4  one_len = 0;
        var_1* one_buf = new var_1[dqm->m_max_write_size];
        if(one_buf == NULL)
        {
            printf("DQM.thread_trim alloc memory error\n");
            return 0;
        }
        
        var_4  two_len = 0;
        var_1* two_buf = new var_1[dqm->m_max_write_size];
        if(two_buf == NULL)
        {
            printf("DQM.thread_trim alloc memory error\n");
            return 0;
        }
        
        var_1 head[9];
        
        for(;; cp_sleep(300000))
        {
            for(var_4 i = 0; i < dqm->m_sf_all_count; i++)
            {
                for(;; cp_sleep(10000))
                {
                    time_t now_sec = (time(NULL) + 3600 * 8) % 86400;
                    if(now_sec > 21600) // 0 - 6
                        continue;
                    
                    break;
                }
                
                dqm->m_sf_travel_lock.lock_r();
                
                var_4 pos = 0;
                var_4 idx_no = 0;
                
                for(;; cp_sleep(1))
                {
                    dqm->m_sf_quota_lock.lock();
                    
                    pos = 0;
                    // find trim pos
                    for(; pos < dqm->m_sf_all_count; pos++)
                    {
                        if(dqm->m_sf_index_no[pos] == i)
                            break;
                    }
                    
                    assert(pos < dqm->m_sf_all_count);
                    
                    idx_no = dqm->m_sf_index_no[pos];
                    
                    if(dqm->m_sf_use_flag[idx_no] != 0)
                    {
                        dqm->m_sf_quota_lock.unlock();
                        continue;
                    }
                    
                    break;
                }
                
                if(dqm->m_sf_cur_size[idx_no] <= 0)
                {
                    dqm->m_sf_quota_lock.unlock();
                    
                    dqm->m_sf_travel_lock.unlock();
                    
                    continue;
                }
                
                // change to off-line
                if(pos < dqm->m_sf_cur_count)
                {
                    var_4 tmp_no = dqm->m_sf_index_no[pos];
                    dqm->m_sf_index_no[pos] = dqm->m_sf_index_no[dqm->m_sf_cur_count - 1];
                    dqm->m_sf_index_no[dqm->m_sf_cur_count - 1] = tmp_no;
                    dqm->m_sf_cur_count--;
                    
                    pos = dqm->m_sf_cur_count;
                }
                
                dqm->m_sf_quota_lock.unlock();
                
                // trim
				close(dqm->m_sf_file_write[idx_no]);
                
				var_4 fp = open(dqm->m_sf_name_lst[idx_no], O_RDONLY);
				while(fp <= 0)
                {
                    printf("DQM.thread_trim open %s error\n", dqm->m_sf_name_lst[idx_no]);
                    
                    cp_sleep(5000);
					fp = open(dqm->m_sf_name_lst[idx_no], O_RDONLY);
                }
                
                var_8 offset = 0;
                var_8 offset_beg = 0;
                var_8 offset_end = 0;
                
                for(;;)
                {
                    try
                    {
                        offset_beg = offset_end;
                        
                        // flag(1) total_size(4) one_buf_len(4) one_buf two_buf_len(4) two_buf end_flag(8)
						if(read(fp, head, 9) != 9)
                            break;
                        
                        one_len = *(var_4*)(head + 5);
						if(read(fp, one_buf, one_len) != one_len)
                            throw -1;
                        
                        offset_end += 9 + one_len;
                        
                        if(*(var_4*)(head + 1) - 4 != one_len)
                        {
							if(read(fp, &two_len, 4) != 4)
                                throw -2;
							if(read(fp, two_buf, two_len) != two_len)
                                throw -3;
                            
                            offset_end += 4 + two_len;
                        }
                        else
                            two_len = 0;
                        
						if(-1 == lseek(fp, DQM_DATA_END_FLAG_SIZE, SEEK_CUR))
                            throw -4;
                        
                        offset_end += DQM_DATA_END_FLAG_SIZE;
                        
                        if(dqm->m_fun_judge(idx_no, offset_beg, one_len, one_buf, dqm->m_fun_argv))
                            continue;
                        
                        var_4 new_idx_no = 0;
                        
                        while(dqm->dqm_write_data(new_idx_no, offset, one_len, one_buf, two_len, two_buf) > 0)
                        {
                            printf("DQM.thread_trim disk is full\n");
                            cp_sleep(5000);
                        }
                        
                        dqm->m_fun_update(new_idx_no, offset, one_len, one_buf, dqm->m_fun_argv);
                    }
                    catch (var_4 error)
                    {
                        for(;;)
                        {
                            printf("DQM.thread_trim read %s error, code = %d, offset=0x%x\n", dqm->m_sf_name_lst[idx_no], error, offset_end);
                            cp_sleep(5000);
                        }
                    }
                }
                
				close(fp);
                
                for(var_4 k = 0; k < dqm->m_max_read_handle_num; k++)
                {
                    fp = dqm->m_sf_file_read[idx_no].PopData();
					close(fp);
                }
                
				dqm->m_sf_file_write[idx_no] = open(dqm->m_sf_name_lst[idx_no], O_WRONLY | O_TRUNC);
				if(dqm->m_sf_file_write[idx_no] <= 0)
                {
                    for(;;)
                    {
                        printf("DQM.thread_trim open(wb) %s error\n", dqm->m_sf_name_lst[idx_no]);
                        cp_sleep(5000);
                    }
                }
                
                dqm->m_sf_cur_size[idx_no] = 0;
                
                for(var_4 k = 0; k < dqm->m_max_read_handle_num; k++)
                {
					fp = open(dqm->m_sf_name_lst[idx_no], O_RDONLY);
					if(fp <= 0)
                    {
                        for(;;)
                        {
                            printf("DQM.thread_trim open(rb) %s error\n", dqm->m_sf_name_lst[idx_no]);
                            cp_sleep(5000);
                        }
                    }
                    
                    dqm->m_sf_file_read[idx_no].PushData(fp);
                }
                
                assert(idx_no == dqm->m_sf_index_no[pos]);
                
                dqm->m_sf_quota_lock.lock();
                
                var_4 tmp_no = dqm->m_sf_index_no[dqm->m_sf_cur_count];
                dqm->m_sf_index_no[dqm->m_sf_cur_count] = dqm->m_sf_index_no[pos];
                dqm->m_sf_index_no[pos] = tmp_no;
                dqm->m_sf_cur_count++;
                
                dqm->m_sf_quota_lock.unlock();
                
                dqm->m_sf_travel_lock.unlock();
                
                printf("DQM.thread_trim %d ok\n", idx_no);
            }
			
			printf("DQM.thread_trim all ok\n");
        }
        
        return 0;
    }
    
public:
    var_u8  m_sf_add_count;
    
    var_1   m_flg_del;
    var_1   m_flg_use;
    
    var_4   m_max_write_size;
    var_4   m_max_read_handle_num;
    
    var_4   m_sf_inP_count;
    var_4   m_sf_inP_allnum[DQM_MAX_STORE_PATH_COUNT];
    var_4   m_sf_inP_curnum[DQM_MAX_STORE_PATH_COUNT];
    
    var_4   m_sf_all_count;
    var_4   m_sf_cur_count;
    
    var_8   m_sf_max_size;
    var_8*  m_sf_cur_size;
    var_8*  m_sf_tmp_size;
    var_4*  m_sf_index_no;
    var_4*  m_sf_use_flag;
    
    var_1** m_sf_name_lst;
    
	UT_Queue<var_4>* m_sf_file_read;
	var_4*           m_sf_file_write;
    
#ifdef DQM_QUICK_READ_DATA
	UT_Queue<var_1*>* m_sf_memory_read;
#endif
	
    CP_MUTEXLOCK     m_sf_quota_lock;
    CP_MUTEXLOCK_RW  m_sf_travel_lock;
    
    var_4 (*m_fun_judge)(var_4, var_8, var_4&, var_1*&, var_vd*);
    var_4 (*m_fun_update)(var_4, var_8, var_4&, var_1*&, var_vd*);
    
    var_vd* m_fun_argv;
};

#endif // _UC_DISKQUOTAMANAGER_H_
