//
//  UC_IncDataManager.h
//  code_library
//
//  Created by zhanghl on 14-9-30.
//  Copyright (c) 2014年 zhanghl. All rights reserved.
//

#ifndef _UC_INCDATAMANAGER_H_
#define _UC_INCDATAMANAGER_H_

#include "UH_Define.h"
#include "UT_LeastHeap.h"

#define END_FLAG		"INCDATAM"
#define END_FLAG_LEN	8

class UC_IncDataManager
{
public:
	var_4 init(var_1* save_path, var_4 data_size, var_4 interval_minute)
	{
		strcpy(m_save_path, save_path);
		
		m_data_size = data_size;
		m_interval_minute = interval_minute;
		
		var_vd* handle;
		
		if(cp_dir_open(handle, m_save_path))
			return -1;
		
		var_1 filename[256];
		var_1 wholename[256];
		
		while(cp_dir_travel(handle, filename) == 0)
		{
			sprintf(wholename, "%s/%s", m_save_path, filename);
			cp_fix_file(wholename, END_FLAG, END_FLAG_LEN);
		}
		
		cp_dir_close(handle);
		
		return 0;
	}

	var_4 save(var_1* save_buffer)
	{
		m_global_lck.lock();
		
		var_1 filename[256];
		
		struct tm now;
		cp_localtime(time(NULL), &now);
		
		sprintf(filename, "%s/%.4d%.2d%.2d%.2d%.2d00", m_save_path, now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min - (now.tm_min % m_interval_minute));
		
		FILE* fp = NULL;
		
		if(access(filename, 0) == 0)
			fp = fopen(filename, "ab");
		else
			fp = fopen(filename, "wb");
		
		if(fp == NULL)
		{
			m_global_lck.unlock();
			return -1;
		}
		
		var_8 size = cp_get_file_size(filename);
		if(size < 0)
		{
			fclose(fp);
			
			m_global_lck.unlock();
			return -1;
		}

		if(fwrite(save_buffer, m_data_size, 1, fp) != 1)
		{
			cp_change_file_size(fp, size);
				
			fclose(fp);
			
			m_global_lck.unlock();
			return -1;
		}
		
		if(fwrite(END_FLAG, END_FLAG_LEN, 1, fp) != 1)
		{
			cp_change_file_size(fp, size);
			
			fclose(fp);
			
			m_global_lck.unlock();
			return -1;
		}
		
		fclose(fp);

		m_global_lck.unlock();
		
		return 0;
	}
	
	var_4 travel_prepare(var_4 travel_num)
	{
		m_global_lck.lock();
		
		UT_LeastHeap<var_u8> lh;
		if(lh.init(travel_num))
		{
			m_global_lck.unlock();
			return -1;
		}
		
		var_vd* handle;
		
		if(cp_dir_open(handle, m_save_path))
		{
			m_global_lck.unlock();
			return -1;
		}
		
		var_1 name[256];
		
		while(cp_dir_travel(handle, name) == 0)
		{
			var_u8 key = cp_strtoval_u64(name);
			
			if(lh.num() < travel_num)
			{
				lh.add(key);
				continue;
			}
			
			if(key < lh.top())
				continue;
			
			lh.del();
			lh.add(key);
		};
		
		cp_dir_close(handle);
	
		m_travel_lck.lock();
		
		m_travel_fp = NULL;
		
		m_travel_cur = 0;
		m_travel_num = lh.num();
		
		m_travel_lst = new(std::nothrow) var_1*[m_travel_num];
		if(m_travel_lst == NULL)
		{
			m_travel_lck.unlock();
			m_global_lck.unlock();
			return -1;
		}
		
		m_travel_buf = new(std::nothrow) var_1[m_travel_num * 256];
		if(m_travel_buf == NULL)
		{
			delete m_travel_lst;
			
			m_travel_lck.unlock();
			m_global_lck.unlock();
			return -1;
		}
		
		var_u8* val = lh.val();
		
		
		for(var_4 i = 0; i < m_travel_num; i++)
		{
			m_travel_lst[i] = m_travel_buf + i * 256;
			
			sprintf(m_travel_lst[i], "%s/" CP_PU64, m_save_path, val[i]);
		}
		
		return 0;
	}
	
	var_4 travel_data(var_1* result_buffer)
	{
		var_1 tmp_buf[END_FLAG_LEN];
		
		for(;;)
		{
			if(m_travel_fp)
			{
				if(fread(result_buffer, m_data_size, 1, m_travel_fp) != 1)
				{
					if(feof(m_travel_fp) == 0)
					{
						fclose(m_travel_fp);
						return -1;
					}
				}
				else
				{
					if(fread(tmp_buf, END_FLAG_LEN, 1, m_travel_fp) != 1)
					{
						fclose(m_travel_fp);
						return -1;
					}

					return 0;
				}
				
				fclose(m_travel_fp);
				m_travel_cur++;
			}
			
			if(m_travel_cur >= m_travel_num)
				return 1;
			
			m_travel_fp = fopen(m_travel_lst[m_travel_cur], "rb");
			if(m_travel_fp == NULL)
				return -1;
		}
	}
	
	var_4 travel_finish()
	{
		m_travel_fp = NULL;
		
		m_travel_cur = 0;
		m_travel_num = 0;

		if(m_travel_lst) delete m_travel_lst;
		if(m_travel_buf) delete m_travel_buf;

		m_travel_lck.unlock();
		
		m_global_lck.unlock();
		
		return 0;
	}
	
private:
	var_1 m_save_path[256];
	
	var_4 m_data_size;
	var_4 m_interval_minute;
	
	CP_MUTEXLOCK m_global_lck;
	
	FILE*   m_travel_fp;
	
	var_4   m_travel_cur;
	var_4   m_travel_num;
	var_1*  m_travel_buf;
	var_1** m_travel_lst;
	
	CP_MUTEXLOCK m_travel_lck;
};

#endif // _UC_INCDATAMANAGER_H_
