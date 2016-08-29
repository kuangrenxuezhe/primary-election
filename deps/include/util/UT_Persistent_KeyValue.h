//
//  UT_Persistent_KeyValue.h
//  code_library
//
//  Created by zhanghl on 13-5-8.
//  Copyright (c) 2013年 zhanghl. All rights reserved.
//

#ifndef __UT_PERSISTENT_KEYVALUE_H__
#define __UT_PERSISTENT_KEYVALUE_H__

#include "UT_HashTable_Pro.h"
#include "UC_DiskQuotaManager.h"

#include <iostream>

template <class T_Key>
class UT_Persistent_KeyValue
{
public:
	static CP_THREAD_T thread_trim(var_vd* argv)
	{
		UT_Persistent_KeyValue<T_Key>* cc = (UT_Persistent_KeyValue*)argv;
		
		for(;;)
        {				
			for(;; cp_sleep(10000))
			{
				time_t now_sec = (time(NULL) + 3600 * 8) % 86400;
				if(now_sec > 3600) // 0 - 1
					continue;
				break;
			}
			
			cc->m_lck.lock();
			while(cc->m_idx.trim())
			{
				cp_sleep(5000);
				printf("UT_Persistent_KeyValue.thread_trim.trim error\n");
			}
			cc->m_lck.unlock();
			
			cp_sleep(3600000);
		}

		return 0;
	}
	
	static var_4 fun_judge(var_4 file_no, var_8 file_offset, var_4& key_len, var_1*& key_buf, var_vd* argv)
	{
		assert(key_len == sizeof(T_Key));

		UT_Persistent_KeyValue<T_Key>* obj = (UT_Persistent_KeyValue<T_Key>*) argv;
		var_u8 value_sto = 0;
		var_vd* value_idx = NULL;

		obj->m_dqm.dqm_index2key(file_no, file_offset, value_sto);

		obj->m_lck.lock();

		if (obj->m_idx.pop_value(*(T_Key*)key_buf, value_idx))
		{
			obj->m_lck.unlock();
			return -1;
		}

		var_4 ret = 1;
		if (value_sto == *(var_u8*)value_idx)
			ret = 0;

		obj->m_idx.push_value(value_idx);

		if (ret == 1)
			obj->m_lck.unlock();

		return ret;
	}
	
	static var_4 fun_update(var_4 file_no, var_8 file_offset, var_4& key_len, var_1*& key_buf, var_vd* argv)
	{
		assert(key_len == sizeof(T_Key));

		UT_Persistent_KeyValue<T_Key>* obj = (UT_Persistent_KeyValue<T_Key>*) argv;

		var_u8 value = 0;

		obj->m_dqm.dqm_index2key(file_no, file_offset, value);

		while (obj->m_idx.add(*(T_Key*)key_buf, &value, NULL, 1) != 1)
		{
			std::cout << "key: " << *(T_Key*)key_buf << " "
				<< "file_no: " << file_no << " "
				<< "file_offset: " << file_offset << std::endl;
			cp_sleep(5000);
		}

		obj->m_lck.unlock();

		return 0;
	}
	
	var_4 init(var_4 sto_path_num, var_1** sto_path, var_4* sto_total_size,
			   var_4 max_write_size, var_4 max_read_handle_num, var_u8 max_key_num, var_1* idx_path,
			   var_4 is_clear = 0)
	{
		if(cp_create_dir(idx_path))
			return -1;

		var_1 name_sto[256];
		sprintf(name_sto, "%s/d6_store.idx", idx_path);
		var_1 name_inc[256];
		sprintf(name_inc, "%s/d6_store.inc", idx_path);
		var_1 name_flg[256];
		sprintf(name_flg, "%s/d6_store.flg", idx_path);

		if(m_idx.init(max_key_num / 2 + 1, max_key_num, 8, name_sto, name_inc, name_flg, 0))
			return -1;

		if(is_clear && m_idx.clear())
			return -1;
			
		if(m_dqm.dqm_init(sto_path_num, sto_path, sto_total_size, 2, max_write_size, max_read_handle_num, fun_judge, fun_update, this, is_clear))
			return -1;

		if(m_dqm.dqm_start_trim())
			return -1;
		
		if(cp_create_thread(thread_trim, this))
			return -1;
		
		return 0;
	}
	
	var_4 add(T_Key key, var_vd* value, var_4 size)
	{
		m_lck.lock();
		
		var_u8 dat_key;
		
		var_4 ret_dqm = m_dqm.dqm_write_data(dat_key, sizeof(T_Key), (var_1*)&key, size, (var_1*)value);
		if(ret_dqm)
		{
			m_lck.unlock();
			return ret_dqm;
		}

		var_4 ret_idx = m_idx.add(key, &dat_key, NULL, 1);
		if(ret_idx < 0)
		{
			m_lck.unlock();
			return ret_idx;
		}
				  
		m_lck.unlock();

		return 0;
	}
	
	var_4 del(T_Key key)
	{
		m_lck.lock();
		
		if(m_idx.del(key))
		{
			m_lck.unlock();
			return -1;
		}
		
		m_lck.unlock();
		
		return 0;
	}

	var_4 query(T_Key key, var_vd* value, var_4& size)
	{
		var_vd* dat_key = NULL;
		
		if(m_idx.pop_value(key, dat_key))
			return -1;
		
		T_Key idx_key;
		var_4 idx_key_len = 8;
		
		var_4 ret_dqm = m_dqm.dqm_read_data(*(var_u8*)dat_key, idx_key_len, (var_1*)&idx_key, size, (var_1*)value);
		
		m_idx.push_value(dat_key);
		
		if(ret_dqm)
			return -1;
		
		return 0;
	}

public:
	UC_DiskQuotaManager m_dqm;
	UT_HashTable_Pro<T_Key> m_idx;
	
	CP_MUTEXLOCK m_lck;
};

#endif // __UT_PERSISTENT_KEYVALUE_H__
