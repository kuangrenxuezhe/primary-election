// UT_HashTable_Lite.h

#ifndef __UT_HASH_TABLE_LITE_H__
#define __UT_HASH_TABLE_LITE_H__

#include "UH_Define.h"
#include "UC_Mem_Allocator_Recycle.h"

#define MAX_TABLE_LOCKER_NUM	1000
#define MIN_TABLE_LOCKER_NUM	1

#define MAX_DELAY_LOCKER_NUM	1000
#define MIN_DELAY_LOCKER_NUM	10

template <class T_Type>
class UT_HashTable_Lite
{
public:
	UT_HashTable_Lite()
	{
	}
	
	~UT_HashTable_Lite()
	{
	}
	
	var_4 init(var_8 table_size, var_4 store_size, var_u8 max_key_num = -1)
	{
		if(table_size <= 0 || store_size <= 0)
			return -1;
		
		m_table_size = table_size;
		m_store_size = store_size;
		
		m_max_key_num = max_key_num;
		m_now_key_num = 0;
				
		m_table_ptr = new var_1*[m_table_size];
		if(m_table_ptr == NULL)
			return -1;
		memset(m_table_ptr, 0, sizeof(var_1*) * m_table_size);
		
		m_table_flg = new var_4[m_table_size];
		if(m_table_flg == NULL)
			return -1;
		memset(m_table_flg, 0, sizeof(var_4) * m_table_size);
		
		m_table_lck_num = (var_4)(m_table_size / 1000);
		if(m_table_lck_num > MAX_TABLE_LOCKER_NUM)
			m_table_lck_num = MAX_TABLE_LOCKER_NUM;
		if(m_table_lck_num < MIN_TABLE_LOCKER_NUM)
			m_table_lck_num = MIN_TABLE_LOCKER_NUM;
		
		m_table_lck = new CP_MUTEXLOCK[m_table_lck_num];
		if(m_table_lck == NULL)
			return -1;
		
		// key + ptr + buf
		m_block_size = sizeof(T_Type) + sizeof(var_1*) + m_store_size;

		if(m_ma.init(m_block_size))
			return -1;
			
		return 0;
	}

	inline var_vd key_lock(T_Type key)
	{
		if(key < 0)
			key *= -1;

		lock_table_w((var_8)(key % m_table_size));
	}

	inline var_vd key_unlock(T_Type key)
	{
		if(key < 0)
			key *= -1;

		unlock_table((var_8)(key % m_table_size));
	}

	var_4 add(T_Type key, var_vd* value, var_vd** reference = NULL, var_4 is_overwrite = 0)
	{
		if(key < 0)
			key *= -1;

		var_8 idx = (var_8)(key % m_table_size);

		var_1* ptr = m_table_ptr[idx];
		var_1* pre = ptr;
		
		while(ptr)
		{
			if(*(T_Type*)ptr == key)
			{
				if(is_overwrite == 1)
					memcpy(ptr + sizeof(T_Type) + sizeof(var_1*), value, m_store_size);

				if(reference)
					*reference = (var_vd*)(ptr + sizeof(T_Type) + sizeof(var_1*));
				
				return 1;
			}
			
			pre = ptr;
			ptr = *(var_1**)(ptr + sizeof(T_Type));
		}
		
		ptr = (var_1*)m_ma.get_mem();
		if(ptr == NULL)
			return -1;
		
		m_lck_key_num.lock();
		if(m_now_key_num >= m_max_key_num)
		{
			m_lck_key_num.unlock();
			
			m_ma.put_mem(ptr);

			return -1;
		}
		
		m_now_key_num++;
		m_lck_key_num.unlock();
		
		*(T_Type*)ptr = key;
		memcpy(ptr + sizeof(T_Type) + sizeof(var_1*), value, m_store_size);
		
		*(var_1**)(ptr + sizeof(T_Type)) = m_table_ptr[idx];
		m_table_ptr[idx] = ptr;
		
		if(reference)
			*reference = (var_vd*)(ptr + sizeof(T_Type) + sizeof(var_1*));

		return 0;
	}

	var_4 query(T_Type key, var_vd*& reference)
	{
		if(key < 0)
			key *= -1;

		var_8 idx = (var_8)(key % m_table_size);

		var_1* ptr = m_table_ptr[idx];
		
		while(ptr)
		{
			if(*(T_Type*)ptr == key)
			{
				reference = (var_vd*)(ptr + sizeof(T_Type) + sizeof(var_1*));
				return 0;
			}
			
			ptr = *(var_1**)(ptr + m_len_ptr);
		}
	
		return -1;
	}
		
	// return: failure is -1, success is 0, no exist is 1
	var_4 del(T_Type key)
	{
		if(key < 0)
			key *= -1;

		var_8 idx = (var_8)(key % m_table_size);

		var_1* ptr = m_table_ptr[idx];
		var_1* pre = ptr;
		
		while(ptr)
		{
			if(*(T_Type*)ptr == key)
				break;
			
			pre = ptr;
			ptr = *(var_1**)(ptr + sizeof(T_Type));
		}
		
		if(ptr == NULL) // no find
			return 1;

		// drop node from list
		if (pre == ptr)
			m_table_ptr[idx] = *(var_1**)(ptr + sizeof(T_Type));
		else
			*(var_1**)(pre + sizeof(T_Type)) = *(var_1**)(ptr + sizeof(T_Type));
		
		// delete node
		m_ma.put_mem(ptr);
		
		m_lck_key_num.lock();
		m_now_key_num--;
		m_lck_key_num.unlock();
		
		return 0;
	}
	
	var_4 num()
	{
		return m_now_key_num;
	}
		
    var_4 save_quick(var_1* library_file, var_4 is_lock = 1)
	{
		if(is_lock)
			m_global_lck.lock_w();
		
		FILE* fp = fopen(library_file, "wb");
		if(fp == NULL)
		{
			if(is_lock)
				m_global_lck.unlock();
			return -1;
		}
		
		var_4 ret = 0;
		
		try {
			if(fwrite(&m_store_size, 4, 1, fp) != 1)
				throw -2;
			
            if(fwrite(&m_table_size, 8, 1, fp) != 1)
                throw -3;
            
            for(var_8 i = 0; i< m_table_size; i++)
            {
                var_1* ptr = m_table_ptr[i];
                var_4  num = 0;
                
                while(ptr)
                {
                    num++;
                    ptr = *(var_1**)(ptr + m_len_ptr);
                }
                
                if(fwrite(&num, 4, 1, fp) != 1)
                    throw -4;
                
                ptr = m_table_ptr[i];
                
                for(var_4 j = 0; j < num; j++)
                {
                    if(fwrite(ptr, sizeof(T_Type), 1, fp) != 1)
                        throw -5;
                    
                    if(m_store_size && fwrite(ptr + m_len_sto, m_store_size, 1, fp) != 1)
                        throw -6;
                }
            }
		}
		catch(var_4 err)
		{
			ret = err;
		}
		
		fclose(fp);
		
		if(is_lock)
			m_global_lck.unlock();
		
		return ret;
	}
	
	var_4 load_quick(var_1* library_file, var_4 is_lock = 1)
	{
		if(is_lock)
			m_global_lck.lock_w();
		
		FILE* fp = fopen(library_file, "rb");
		if(fp == NULL)
		{
			if(is_lock)
				m_global_lck.unlock();
			return -1;
		}
		
		var_4  ret = 0;
		T_Type  key;
		var_1* buf = NULL;
		
		try
		{
			var_4 store_size = 0;
			if(fread(&store_size, 4, 1, fp) != 1)
				throw -2;
			
			if(store_size > m_store_size)
				throw -3;
            if(store_size != m_store_size)
                printf("store size in store file(%d) != m_store_size(%d)", store_size, m_store_size);
			
            var_8 table_size = 0;
            if(fread(&table_size, 8, 1, fp) != 1)
                throw -4;
            
            if(table_size > m_table_size)
                throw -5;
	
            for(var_8 i = 0; i < table_size; i++)
            {
                var_4 num = 0;
                if(fread(&num, 4, 1, fp) != 1)
                    throw -6;
                
                var_1** tail_ptr = m_table_ptr + i;
                
                for(var_4 j = 0; j < num; j++)
                {
                    // create new node
                    var_1* node = m_ar.AllocMem();
                    if(node == NULL)
                        throw -7;
                    
                    if(fread(node, sizeof(T_Type), 1, fp) != 1)
                        throw -8;
                    
                    if(store_size && fread(node + m_len_sto, store_size, 1, fp) != 1)
                        throw -9;

                    *(var_1*)(node + m_len_lck) = 0;
                    *(var_u4*)(node + m_len_ref) = 0;
                    *(node + m_len_del) = 0;
                    *(var_1**)(node + m_len_ptr) = NULL;
                    
                    // insert new node to list
                    *tail_ptr = node;
                    tail_ptr = (var_1**)(node + m_len_ptr);
                }
            }
		}
		catch(var_4 err)
		{
			ret = err;
		}
        
		fclose(fp);
		
		if(is_lock)
			m_global_lck.unlock();
		
		return ret;
	}
	
	const var_1* version()
	{
		// v1.000 - 2013.05.21 - 初始版本
		return "v1.000";
	}
	
private:
	inline var_vd lock_table_r(var_8 idx)
	{
		var_4 lock_no = idx % m_table_lck_num;
		
		for(;;)
		{
			m_table_lck[lock_no].lock();
			
			if(m_table_flg[idx] >= 0)
				break;
			
			m_table_lck[lock_no].unlock();
			
			cp_sleep(1);
		}
		
		m_table_flg[idx]++;
		
		m_table_lck[lock_no].unlock();
	}
	
	inline var_vd lock_table_w(var_8 idx)
	{
		var_4 lock_no = idx % m_table_lck_num;
		
		for(;;)
		{
			m_table_lck[lock_no].lock();
			
			if(m_table_flg[idx] == 0)
				break;
			
			m_table_lck[lock_no].unlock();
			
			cp_sleep(1);
		}
		
		m_table_flg[idx] = -1;
		
		m_table_lck[lock_no].unlock();
	}
	
	inline var_vd unlock_table(var_8 idx)
	{
		var_4 lock_no = idx % m_table_lck_num;
		
		m_table_lck[lock_no].lock();
		
		if(m_table_flg[idx] < 0)
			m_table_flg[idx] = 0;
		else
			m_table_flg[idx]--;
		
		m_table_lck[lock_no].unlock();
	}
	
public:
	var_8 m_table_size;
	var_4 m_store_size;
	
	var_u8       m_now_key_num;
	var_u8       m_max_key_num;
	CP_MUTEXLOCK m_lck_key_num;
	
	var_4 m_block_size;

	var_1** m_table_ptr;
	var_4*  m_table_flg;
	
	CP_MUTEXLOCK* m_table_lck;
	var_4         m_table_lck_num;
	
	UC_Mem_Allocator_Recycle m_ma;
};

#endif // __UT_HASH_TABLE_LITE_H__
