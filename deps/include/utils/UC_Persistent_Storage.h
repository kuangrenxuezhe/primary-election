//
//  UC_Persistent_Storage.h
//  code_library
//
//  Created by zhanghl on 13-1-25.
//  Copyright (c) 2013年 zhanghl. All rights reserved.
//

#ifndef __UC_PERSISTENT_STORAGE_H__
#define __UC_PERSISTENT_STORAGE_H__

#include "UH_Define.h"

#define UC_PS_END_FLG_VAL	"PSENDFLG"
#define UC_PS_END_FLG_LEN	8

class UC_Persistent_Storage
{
public:
    var_4 init(var_1* store_path, var_1* store_file_prefix)
    {
        strcpy(m_store_path, store_path);

        sprintf(m_store_file_lib, "%s/%s.lib", store_path, store_file_prefix);
        sprintf(m_store_file_inc, "%s/%s.inc", store_path, store_file_prefix);
		sprintf(m_store_file_tmp, "%s/%s.tmp", store_path, store_file_prefix);
		sprintf(m_store_file_new, "%s/%s.new", store_path, store_file_prefix);
        
        if(cp_create_dir(m_store_path))
            return -1;
        
		if(access(m_store_file_new, 0) == 0)
		{
			cp_remove_file(m_store_file_inc);
			cp_remove_file(m_store_file_lib);
			
			while(cp_rename_file(m_store_file_new, m_store_file_lib))
				cp_sleep(5000);
		}
		
        if(access(m_store_file_lib, 0) == 0 && cp_fix_file(m_store_file_lib, UC_PS_END_FLG_VAL, UC_PS_END_FLG_LEN))
            return -1;
        if(access(m_store_file_inc, 0) == 0 && cp_fix_file(m_store_file_inc, UC_PS_END_FLG_VAL, UC_PS_END_FLG_LEN))
            return -1;
        
		if(access(m_store_file_inc, 0) == 0)
			m_store_inc = fopen(m_store_file_inc, "rb+");
		else
			m_store_inc = fopen(m_store_file_inc, "wb");
		
		if(fseek(m_store_inc, 0, SEEK_END))
			return -1;
		
		m_travel_fpr = NULL;
		m_travel_flg = 0;
		
		m_trim_fpr = NULL;
		
        return 0;
    }
    
    var_4 save(var_1* store_buf, var_4 store_len)
    {
		if(store_len < 0)
			return -1;
		
		if(store_len == 0)
			return 0;
		
		m_locker.lock();
		
		store_len += UC_PS_END_FLG_LEN;
		
		if(fwrite(&store_len, 4, 1, m_store_inc) != 1)
		{
			m_locker.unlock();
			return -1;
		}
		if(fwrite(store_buf, store_len - UC_PS_END_FLG_LEN, 1, m_store_inc) != 1)
		{
			m_locker.unlock();
			return -1;
		}
		if(fwrite(UC_PS_END_FLG_VAL, UC_PS_END_FLG_LEN, 1, m_store_inc) != 1)
		{
			m_locker.unlock();
			return -1;
		}

		if(fflush(m_store_inc))
		{
			m_locker.unlock();
			return -1;
		}

		m_locker.unlock();
		
        return 0;
    }
    
	var_4 travel_prepare()
	{
		m_locker.lock();
		m_travel_flg = 0;
		
		return 0;
	}
	
	var_4 travel_data(var_1* store_buf, var_4 store_len, var_4& ret_len, var_4& src_flg)
	{		
		if(m_travel_flg == 0)
		{
			if(access(m_store_file_lib, 0) == 0)
			{
				m_travel_fpr = fopen(m_store_file_lib, "rb");
				if(m_travel_fpr == NULL)
					return -1;
				
				m_travel_flg = 1;
			}
			else if(access(m_store_file_inc, 0) == 0)
			{
				m_travel_fpr = fopen(m_store_file_inc, "rb");
				if(m_travel_fpr == NULL)
					return -1;
				
				m_travel_flg = 2;
			}
			else
				return 1; // no data
		}
        
		while(fread(&ret_len, 4, 1, m_travel_fpr) != 1)
		{
			fclose(m_travel_fpr);
			
			if(m_travel_flg == 2)
				return 1; // no data
			
			if(access(m_store_file_inc, 0) == 0)
			{
				m_travel_fpr = fopen(m_store_file_inc, "rb");
				if(m_travel_fpr == NULL)
					return -1;
				
				m_travel_flg = 2;
			}
			else
				return 1; // no data
		}
		
		ret_len -= UC_PS_END_FLG_LEN;
		
		if(store_len < ret_len)
		{
			fclose(m_travel_fpr);
			return -1;
		}
		
		if(fread(store_buf, ret_len, 1, m_travel_fpr) != 1)
		{
			fclose(m_travel_fpr);
			return -1;
		}
		
		var_1 flg_buf[UC_PS_END_FLG_LEN];
		
		if(fread(flg_buf, UC_PS_END_FLG_LEN, 1, m_travel_fpr) != 1)
		{
			fclose(m_travel_fpr);
			return -1;
		}
		
		if(*(var_8*)flg_buf != *(var_8*)UC_PS_END_FLG_VAL)
		{
			fclose(m_travel_fpr);
			return -1;
		}
		
        src_flg = m_travel_flg;
        
		return 0;
	}
	
	var_4 travel_finish()
	{
		m_travel_flg = 0;
		m_locker.unlock();
		
		return 0;
	}
	
    var_4 trim_prepare()
    {
		m_locker.lock();
		
		m_trim_fpr = fopen(m_store_file_tmp, "wb");
		if(m_trim_fpr == NULL)
		{
			m_locker.unlock();
			return -1;
		}
		
        return 0;
    }
	
    var_4 trim_data(var_1* store_buf, var_4 store_len)
    {
		if(store_len < 0)
			return -1;
		
		if(store_len == 0)
			return 0;
		
		store_len += UC_PS_END_FLG_LEN;
		
		if(fwrite(&store_len, 4, 1, m_trim_fpr) != 1)
			return -1;
		if(fwrite(store_buf, store_len - UC_PS_END_FLG_LEN, 1, m_trim_fpr) != 1)
			return -1;
		if(fwrite(UC_PS_END_FLG_VAL, UC_PS_END_FLG_LEN, 1, m_trim_fpr) != 1)
			return -1;
		
        return 0;
    }
	
	var_4 trim_failure()
	{
		fclose(m_trim_fpr);
		
		remove(m_store_file_tmp);
		
		m_locker.unlock();
		
		return 0;
	}
	
    var_4 trim_success()
    {
		fclose(m_trim_fpr);

		while(cp_rename_file(m_store_file_tmp, m_store_file_new))
			cp_sleep(5000);
		
		fclose(m_store_inc);
		
		cp_remove_file(m_store_file_inc);
		cp_remove_file(m_store_file_lib);
		
		while(cp_rename_file(m_store_file_new, m_store_file_lib))
			cp_sleep(5000);
		
		m_store_inc = fopen(m_store_file_inc, "wb");
		while(m_store_inc == NULL)
		{
			printf("UC_PersistenceStore.trim_success.open %s failuer\n", m_store_file_inc);
			cp_sleep(5000);
			
			m_store_inc = fopen(m_store_file_inc, "wb");
		}
			
		m_locker.unlock();
		
        return 0;
    }
    
    var_4 get_library_name(var_4 name_len, var_1* name_buf)
    {
        if(name_len < 256)
            return -1;
        
        strcpy(name_buf, m_store_file_lib);
        
        return 0;
    }
    
private:
    var_1 m_store_path[256];
	
    var_1 m_store_file_lib[256];
    var_1 m_store_file_inc[256];
	var_1 m_store_file_tmp[256];
	var_1 m_store_file_new[256];
	
	FILE* m_store_inc;
	
	FILE* m_travel_fpr;
	var_4 m_travel_flg;
	
	FILE* m_trim_fpr;
	
	CP_MUTEXLOCK m_locker;
};

#endif // __UC_PERSISTENT_STORAGE_H__
