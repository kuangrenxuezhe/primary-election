// UT_HashLibrary.h

#ifndef _UT_HASH_LIBRARY_H_
#define _UT_HASH_LIBRARY_H_

#include "UH_Define.h"

#include "UT_HashSearch.h"
#include "UC_Allocator_Recycle.h"

template <class M_Key, class S_Key>
class UT_HashLibrary
{
public:
	UT_HashLibrary()
	{
	}

	~UT_HashLibrary()
	{
	}

	var_4 init(var_4 key_num_m, var_4 key_num_s, var_4 sto_len, var_1* name_sto, var_1* name_inc, var_1* name_flg)
	{
		m_key_num = key_num_m;
		s_key_num = key_num_s;
		m_sto_len = sto_len;
		m_rel_len = sizeof(var_vd*) + sizeof(S_Key) + sto_len; // NODE_pointer  SLAVE_key  STORE_buffer

		strcpy(m_name_sto, name_sto);
		strcpy(m_name_inc, name_inc);
		strcpy(m_name_flg, name_flg);
				
		var_4 hash_size = m_key_num / 10;
		if(hash_size <= 0)
			hash_size = 10;
		if(m_hs.InitHashSearch(hash_size, 8 + sizeof(var_vd*))) // ref_num  lst_num
			return -1;

		m_del_buf = new var_1[m_sto_len];
		if(m_del_buf == NULL)
			return -1;
		memset(m_del_buf, 0xAA, m_sto_len);
		
		if(m_ar.initMem(m_rel_len, key_num_s))
			return -1;

		if(restore())
			return -1;

		if(cp_create_thread(thread_trim, this))
			return -1;

		return 0;
	}

	var_4 restore()
	{
		var_1 filename[256];
		sprintf(filename, "%s.tmp", m_name_sto);

		if(access(m_name_flg, 0) == 0) // clear
		{
			while(access(filename, 0) == 0 && remove(filename))
			{
				printf("UT_HashLibrary.restore - remove %s failure\n", filename);
				cp_sleep(5000);
			}
			while(access(m_name_sto, 0) == 0 && remove(m_name_sto))
			{
				printf("UT_HashLibrary.restore - remove %s failure\n", m_name_sto);
				cp_sleep(5000);
			}
			while(access(m_name_inc, 0) == 0 && remove(m_name_inc))
			{
				printf("UT_HashLibrary.restore - remove %s failure\n", m_name_inc);
				cp_sleep(5000);
			}
			while(remove(m_name_flg))
			{
				printf("UT_HashLibrary.restore - remove %s failure\n", m_name_flg);
				cp_sleep(5000);
			}
		}

		if(access(filename, 0) == 0 && access(m_name_sto, 0))
		{
			if(remove(m_name_inc))
				return -1;
			if(rename(filename, m_name_sto))
				return -1;
		}

		if(access(filename, 0) == 0 && access(m_name_sto, 0) == 0)
		{
			if(remove(filename))
				return -1;
		}

		var_4 buflen = 8 + sizeof(M_Key) + sizeof(S_Key) + m_sto_len;

		if(access(m_name_sto, 0) == 0)
		{
			var_1* buf = new var_1[buflen];
			if(buf == NULL)
				return -1;

			var_8 len = cp_get_file_size(m_name_sto);
			if(len % buflen)
				return -1;
			var_4 num = len / buflen;

			FILE* fp = fopen(m_name_sto, "rb");
			if(fp == NULL)
				return -1;
			for(var_4 i = 0; i < num; i++)
			{
				if(fread(buf, buflen, 1, fp) != 1)
					return -1;

				if(*(var_u8*)buf != (var_u8)0xFAFAFAFAFAFAFAFA)
					return -1;

				if(add(*(M_Key*)(buf + 8), *(S_Key*)(buf + 8 + sizeof(M_Key)), buf + 8 + sizeof(M_Key) + sizeof(S_Key), 1))
					return -1;
			}
			fclose(fp);

			delete buf;
		}

		if(access(m_name_inc, 0) == 0)
		{
			var_1* buf = new var_1[buflen];
			if(buf == NULL)
				return -1;

			var_8 len = cp_get_file_size(m_name_inc);
			if(len % buflen)
				return -1;
			var_4 num = len / buflen;

			m_file_inc = fopen(m_name_inc, "rb");
			if(m_file_inc == NULL)
				return -1;
			for(var_4 i = 0; i < num; i++)
			{
				if(fread(buf, buflen, 1, m_file_inc) != 1)
					return -1;
								
				if(*(var_u8*)buf == (var_u8)0xABABABABABABABAB)
				{
					if(add(*(M_Key*)(buf + 8), *(S_Key*)(buf + 8 + sizeof(M_Key)), buf + 8 + sizeof(M_Key) + sizeof(S_Key), 1))
						return -1;
				}
				else if(*(var_u8*)buf == (var_u8)0xCDCDCDCDCDCDCDCD)
				{
					if(del_m(*(M_Key*)(buf + 8), *(S_Key*)(buf + 8 + sizeof(M_Key)), 1))
						return -1;
				}
				else
				{
					if(del_s(*(M_Key*)(buf + 8), *(S_Key*)(buf + 8 + sizeof(M_Key)), 1) < 0)
						return -1;
				}
			}
			fclose(m_file_inc);

			m_file_inc = fopen(m_name_inc, "rb+");
			fseek(m_file_inc, 0, SEEK_END);
		}
		else
			m_file_inc = fopen(m_name_inc, "wb");

		if(m_file_inc == NULL)
			return -1;

		return 0;
	}

	var_4 add(M_Key m_key, S_Key s_key, var_1* key_buf, var_4 is_bak = 0)
	{
		if(is_bak == 0)
		{
			// write inc disk file
			m_lock_dsk.lock();

			var_8 size = ftell(m_file_inc);
			try
			{
				var_u8 flag = 0xABABABABABABABAB;
				if(fwrite(&flag, 8, 1, m_file_inc) != 1)
					throw -1;
				if(fwrite(&m_key, sizeof(M_Key), 1, m_file_inc) != 1)
					throw -2;
				if(fwrite(&s_key, sizeof(S_Key), 1, m_file_inc) != 1)
					throw -3;
				if(fwrite(key_buf, m_sto_len, 1, m_file_inc) != 1)
					throw -4;
				fflush(m_file_inc);
			}
			catch (var_4 err)
			{
				printf("UT_HashLibrary.add - write failure, code = %d\n", err);

				cp_change_file_size(m_file_inc, size);
				fflush(m_file_inc);

				m_lock_dsk.unlock();
				return -1;
			}

			m_lock_dsk.unlock();
		}		

		// update mem index
		m_lock_mem.lock_w();

		var_1* buf = m_ar.AllocMem();
		if(buf == NULL)
		{
			m_lock_mem.unlock();
			return -1;
		}
		
		*(var_1**)buf = NULL;
		*(S_Key*)(buf + sizeof(var_vd*)) = s_key;
		memcpy(buf + sizeof(var_vd*) + sizeof(S_Key), key_buf, m_sto_len);

		//
		var_1 head[8 + sizeof(var_vd*)]; // ref_num nod_num nxt_ptr
		*(var_4*)head = 0;
		*(var_4*)(head + 4) = 1;
		*(var_1**)(head + 8) = buf;

		//
		var_vd* ptr = NULL;
		var_4 ret = m_hs.AddKey_FL(m_key, (var_vd*)head, &ptr);
		if(ret < 0)
		{
			m_ar.FreeMem(buf);

			m_lock_mem.unlock();
			return -1;
		}
		else if(ret > 0)
		{
			var_1* old = (var_1*)ptr;

			*(var_4*)(old + 4) += 1;
			*(var_1**)buf = *(var_1**)(old + 8);
			*(var_1**)(old + 8) = buf;
		}

		m_lock_mem.unlock();

		return 0;
	}

	var_4 pop(M_Key key, var_1*& out_buf)
	{
		m_lock_mem.lock_r();

		var_vd* ptr = NULL;

		var_4 ret = m_hs.SearchKey_FL(key, &ptr);
		if(ret < 0)
		{
			m_lock_mem.unlock();

			out_buf = NULL;
			return -1;
		}

		out_buf = (var_1*)ptr;

		cp_lock_inc((var_u4*)out_buf);
		out_buf += 8;

		m_lock_mem.unlock();

		return 0;
	}

	var_4 push(var_1*& out_buf)
	{
		out_buf -= 8;
		cp_lock_dec((var_u4*)out_buf);

		return 0;
	}

	var_4 del_m(M_Key m_key, S_Key s_key, var_4 is_bak = 0)
	{
		if(is_bak == 0)
		{
			// write inc disk file
			m_lock_dsk.lock();

			var_8 size = ftell(m_file_inc);
			try
			{
				var_u8 flag = 0xCDCDCDCDCDCDCDCD;
				if(fwrite(&flag, 8, 1, m_file_inc) != 1)
					throw -1;
				if(fwrite(&m_key, sizeof(M_Key), 1, m_file_inc) != 1)
					throw -2;
				if(fwrite(&s_key, sizeof(S_Key), 1, m_file_inc) != 1)
					throw -3;
				if(fwrite(m_del_buf, m_sto_len, 1, m_file_inc) != 1)
					throw -4;
				fflush(m_file_inc);
			}
			catch (var_4 err)
			{
				printf("UT_HashLibrary.del - write failure, code = %d\n", err);

				cp_change_file_size(m_file_inc, size);
				fflush(m_file_inc);

				m_lock_dsk.unlock();
				return -1;
			}	

			m_lock_dsk.unlock();
		}

		// update mem index
		m_lock_mem.lock_w();

		var_vd* ptr = NULL;

		if(m_hs.SearchKey_FL(m_key, &ptr))
		{
			m_lock_mem.unlock();
			return -1;
		}

		var_1* buf = (var_1*)ptr;

		while(*(var_4*)buf != 0)
			cp_sleep(1);

		buf = *(var_1**)(buf + 8);

		if(m_hs.DeleteKey_FL(m_key))
		{
			m_lock_mem.unlock();
			return -1;
		}		

		while(buf)
		{
			var_1* del = buf;
			buf = *(var_1**)buf;

			m_ar.FreeMem(del);
		}

		m_lock_mem.unlock();

		return 0;
	}

	var_4 del_s(M_Key m_key, S_Key s_key, var_4 is_bak = 0)
	{
		if(is_bak == 0)
		{
			// write inc disk file
			m_lock_dsk.lock();

			var_8 size = ftell(m_file_inc);
			try
			{
				var_u8 flag = 0xEFEFEFEFEFEFEFEF;
				if(fwrite(&flag, 8, 1, m_file_inc) != 1)
					throw -1;
				if(fwrite(&m_key, sizeof(M_Key), 1, m_file_inc) != 1)
					throw -2;
				if(fwrite(&s_key, sizeof(S_Key), 1, m_file_inc) != 1)
					throw -3;
				if(fwrite(m_del_buf, m_sto_len, 1, m_file_inc) != 1)
					throw -4;
				fflush(m_file_inc);
			}
			catch (var_4 err)
			{
				printf("UT_HashLibrary.del - write failure, code = %d\n", err);

				cp_change_file_size(m_file_inc, size);
				fflush(m_file_inc);

				m_lock_dsk.unlock();
				return -1;
			}	

			m_lock_dsk.unlock();
		}

		// update mem index
		m_lock_mem.lock_w();

		var_vd* ptr = NULL;

		if(m_hs.SearchKey_FL(m_key, &ptr))
		{
			m_lock_mem.unlock();
			return -1;
		}

		var_1* head = (var_1*)ptr;

		while(*(var_4*)head != 0)
			cp_sleep(1);

		var_4  lst_num = *(var_4*)(head + 4);
		var_1* buf = *(var_1**)(head + 8);

		*(var_4*)(head + 4) = 0;
		*(var_1**)(head + 8) = NULL;

		var_1* tmp = NULL;

		for(var_4 i = 0; i < lst_num; i++)
		{
			tmp = buf;
			buf = *(var_1**)buf;

			if(*(S_Key*)(tmp + sizeof(var_vd*)) == s_key)
				m_ar.FreeMem(tmp);				
			else
			{
				*(var_4*)(head + 4) += 1;
				*(var_1**)tmp = *(var_1**)(head + 8);
				*(var_1**)(head + 8) = tmp;
			}
		}

		var_4 retval = *(var_4*)(head + 4);

		m_lock_mem.unlock();

		return retval;
	}

	var_4 clear()
	{
		m_lock_dsk.lock();
		m_lock_mem.lock_w();

		FILE* fp = fopen(m_name_flg, "wb");
		while(fp == NULL)
		{
			printf("UT_HashLibrary.clear - create %s failure\n", m_name_flg);
			cp_sleep(5000);
			fp = fopen(m_name_flg, "wb");
		}
		fclose(fp);

		while(access(m_name_sto, 0) == 0 && remove(m_name_sto))
		{
			printf("UT_HashLibrary.clear - remove %s failure\n", m_name_sto);
			cp_sleep(5000);
		}

		fclose(m_file_inc);

		while(access(m_name_inc, 0) == 0 && remove(m_name_inc))
		{
			printf("UT_HashLibrary.clear - remove %s failure\n", m_name_inc);
			cp_sleep(5000);
		}

		while(remove(m_name_flg))
		{
			printf("UT_HashLibrary.clear - remove %s failure\n", m_name_flg);
			cp_sleep(5000);
		}

		m_hs.ClearHashSearch();
		m_ar.ResetAllocator();

		m_file_inc = fopen(m_name_inc, "wb");
		while(m_file_inc == NULL)
		{
			printf("UT_HashLibrary.clear - create %s failure\n", m_name_inc);
			cp_sleep(5000);
			m_file_inc = fopen(m_name_inc, "wb");
		}

		m_lock_mem.unlock();
		m_lock_dsk.unlock();
		
		return 0;
	}

	void travel_pre()
	{
		m_lock_dsk.lock();
		m_lock_mem.lock_w();

		m_hs.PreTravelKey();

		m_tvl_ptr = NULL;
		m_mst_key = 0;
	}

	var_4 travel_key(M_Key& m_key, S_Key& s_key, var_1*& s_buf, var_4& s_len)
	{
		if(m_tvl_ptr)
		{
			m_key = m_mst_key;

			s_key = *(S_Key*)(m_tvl_ptr + sizeof(var_vd*));
			s_buf = m_tvl_ptr + sizeof(var_vd*) + sizeof(S_Key);
			s_len = m_sto_len;

			m_tvl_ptr = *(var_1**)m_tvl_ptr;
		}

		var_vd* ptr = NULL;
		var_4   len = 0;
		while(m_hs.TravelKey(m_key, ptr, len) == 0)
		{
			m_tvl_ptr = (var_1*)ptr;
			m_tvl_ptr = *(var_1**)(m_tvl_ptr + 8);

			m_mst_key = m_key;
			
			s_key = *(S_Key*)(m_tvl_ptr + sizeof(var_vd*));
			s_buf = m_tvl_ptr + sizeof(var_vd*) + sizeof(S_Key);
			s_len = m_sto_len;

			m_tvl_ptr = *(var_1**)m_tvl_ptr;

			return 0;
		}

		m_lock_mem.unlock();
		m_lock_dsk.unlock();

		return -1;
	}

	var_4 save()
	{
		var_1 filename[256];
		sprintf(filename, "%s.tmp", m_name_sto);
		FILE* fp = fopen(filename, "wb");
		while(fp == NULL)
		{
			printf("UT_HashLibrary.save - open %s failure\n", filename);
			cp_sleep(5000);
		}

		M_Key   key;
		var_vd* ptr;
		var_4   len;

		m_lock_dsk.lock();
		m_lock_mem.lock_w();

		var_u8 flag = 0xFAFAFAFAFAFAFAFA;

		m_hs.PreTravelKey();
		while(m_hs.TravelKey(key, ptr, len) == 0)
		{
			var_1* buf = (var_1*)ptr;
			buf = *(var_1**)(buf + 8);

			while(buf)
			{
				if(fwrite(&flag, 8, 1, fp) != 1)
				{
					printf("UT_HashLibrary.save - write failure, pause\n");

					fclose(fp);
					return -1;
				}
				if(fwrite(&key, sizeof(M_Key), 1, fp) != 1)
				{
					printf("UT_HashLibrary.save - write failure, pause\n");

					fclose(fp);
					return -1;
				}
				if(fwrite(buf + sizeof(var_vd*), sizeof(S_Key), 1, fp) != 1)
				{
					printf("UT_HashLibrary.save - write failure, pause\n");

					fclose(fp);
					return -1;
				}
				if(fwrite(buf + sizeof(var_vd*) + sizeof(S_Key), m_sto_len, 1, fp) != 1)
				{
					printf("UT_HashLibrary.save - write failure, pause\n");

					fclose(fp);
					return -1;
				}

				buf = *(var_1**)buf;
			}
		}

		fclose(fp);

		while(access(m_name_sto, 0) == 0 && remove(m_name_sto))
		{
			cp_sleep(5000);
			printf("UT_HashLibrary.save - remove %s failure\n", m_name_sto);
		}

		fclose(m_file_inc);
		while(remove(m_name_inc))
		{
			cp_sleep(5000);
			printf("UT_HashLibrary.save - remove %s failure\n", m_name_inc);
		}
		m_file_inc = fopen(m_name_inc, "wb");
		while(m_file_inc == NULL)
		{
			cp_sleep(5000);
			printf("UT_HashLibrary.save - open %s failure\n", m_name_inc);

			m_file_inc = fopen(m_name_inc, "wb");
		}

		while(rename(filename, m_name_sto))
		{
			cp_sleep(5000);
			printf("UT_HashLibrary.save - rename %s to %s failure\n", filename, m_name_sto);
		}

		m_lock_mem.unlock();
		m_lock_dsk.unlock();

		return 0;
	}

	static CP_THREAD_T thread_trim(var_vd* ptr)
	{
		UT_HashLibrary* cc = (UT_HashLibrary*)ptr;

		for(;;)
		{
			time_t now_sec = time(NULL);
			struct tm* now_time = localtime(&now_sec);

			if(now_time->tm_hour != 2 || now_time->tm_min != 0)
			{
				cp_sleep(10000);
				continue;
			}
			
			while(cc->save())
			{
				printf("UT_HashLibrary.thread_trim - save failure, try...\n");
				cp_sleep(10000);
			}
		}

		return 0;
	}

private:
	var_4 m_key_num;
	var_4 s_key_num;
	var_4 m_sto_len;
	var_4 m_rel_len;

	var_1 m_name_sto[256];
	var_1 m_name_inc[256];
	var_1 m_name_flg[256];
	
	UT_HashSearch<M_Key> m_hs;
	UC_Allocator_Recycle m_ar;

	FILE* m_file_inc;

	CP_MUTEXLOCK_RW m_lock_mem;
	CP_MUTEXLOCK    m_lock_dsk;

	var_1* m_del_buf;

	var_1* m_tvl_ptr; // tvl = travel
	var_u8 m_mst_key;
};

#endif // _UT_HASH_LIBRARY_H_
