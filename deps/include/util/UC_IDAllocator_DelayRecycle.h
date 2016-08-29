// UC_IDAllocator_DelayRecycle.h

#ifndef __UC_ID_ALLOCATOR_DELAY_RECYCLE_H__
#define __UC_ID_ALLOCATOR_DELAY_RECYCLE_H__

#include "UH_Define.h"
#include "UT_HashSearch.h"

class UC_IDAllocator_DelayRecycle
{
public:
	UC_IDAllocator_DelayRecycle()
	{
		m_max_id = 0;
		m_cur_id = 0;

		m_fp_save = NULL;

		m_PID_to_DID = NULL;

		m_idle_lst = NULL;
		m_idle_pos = 0;
		m_idle_max = 0;
	}

	~UC_IDAllocator_DelayRecycle()
	{
		if(m_fp_save)
			fclose(m_fp_save);

		if(m_PID_to_DID)
			delete m_PID_to_DID;

		if(m_idle_lst)
			delete m_idle_lst;
	}

	var_4 init(var_4 max_id, var_1* save_file)
	{
		// init
		m_max_id = max_id;
		m_cur_id = 0;

		strcpy(m_file_save, save_file);

		m_PID_to_DID = new var_u8[m_max_id];
		if(m_PID_to_DID == NULL)
			return -1;
		memset(m_PID_to_DID, 0, m_max_id * sizeof(var_u8));

		var_4 hash_size = (m_max_id + 1) / 2;
		if(m_DID_to_PID.InitHashSearch(hash_size, 4))
			return -1;

		m_idle_max = (m_max_id + 9) / 10;
		m_idle_lst = new var_u4[m_idle_max];
		if(m_idle_lst == NULL)
			return -1;
		m_idle_pos = 0;

		// fix mutil file
		var_1 file_new[256];
		sprintf(file_new, "%s.new", m_file_save);
		if(access(file_new, 0) == 0)
		{
			if(access(m_file_save, 0) == 0 && remove(m_file_save))
			{
				printf("IDA: remove %s failure\n", m_file_save);
				return -1;
			}
			if(rename(file_new, m_file_save))
			{
				printf("IDA: rename %s to %s failure\n", file_new, m_file_save);
				return -1;
			}
		}

		// index file no exist
		if(access(m_file_save, 0))
		{
			m_fp_save = fopen(m_file_save, "wb");
			if(m_fp_save == NULL)
			{
				printf("IDA: open id table file failure\n");
				return -1;
			}

			printf("IDA: id table no exist, next id = %u\n", m_cur_id);

			return 0;
		}

		// fix index file
		struct stat info;
		stat(m_file_save, &info);
		if(info.st_size % 13)
		{
			printf("IDA: id table file length error, try fix it\n");

			FILE* fp = fopen(m_file_save, "rb+");
			if(fp == NULL)
			{
				printf("IDA: open id table file failure\n");
				return -1;
			}
			if(cp_change_file_size(fp, info.st_size / 13 * 13))
			{
				fclose(fp);

				printf("IDA: id table file fix failure\n");
				return -1;
			}
			else
				printf("IDA: id table file fix success");
			fclose(fp);
		}
		
		// index file is empty
		var_u4 id_num = info.st_size / 13;
		if(id_num == 0)
		{
			m_fp_save = fopen(m_file_save, "wb");
			if(m_fp_save == NULL)
			{
				printf("IDA: open id table file failure\n");
				return -1;
			}
			
			printf("IDA: id table is empty, next id = %u\n", m_cur_id);

			return 0;
		}

		// load index file
		var_1  buf[13];

		FILE* fp = fopen(m_file_save, "rb");
		if(fp == NULL)
		{
			printf("IDA: open id table file failure\n");
			return -1;
		}
		for(var_u4 i = 0; i < id_num; i++)
		{
			if(fread(buf, 13, 1, fp) != 1)
			{
				fclose(fp);

				printf("IDA: read id table file failure\n");
				return -1;
			}

			var_1  flg = *buf; // 0- 1+
			var_u4 pge = *(var_u4*)(buf + 1);
			var_u8 doc = *(var_u8*)(buf + 5);			
			
			if(m_cur_id < pge)
			{
				if(pge >= m_max_id)
				{
					fclose(fp);

					printf("IDA: id out of rang\n");			
					return -1;
				}
				m_cur_id = pge;
			}

			if(flg == 1)
			{
				void* tmpval = NULL;
				if(m_DID_to_PID.SearchKey_FL(doc, &tmpval) == 0)
					*(var_u4*)tmpval = pge;			
				else
				{
					if(m_DID_to_PID.AddKey_FL(doc, (void*)&pge))
					{
						fclose(fp);

						printf("IDA: add id to HS failure\n");
						return -1;
					}
				}

				m_PID_to_DID[pge] = doc;
			}
			else
			{
				var_u8 doc = m_PID_to_DID[pge];

				void* tmpval = NULL;
				if(m_DID_to_PID.SearchKey_FL(doc, &tmpval))
				{
					fclose(fp);

					printf("IDA: delete id from HS failure\n");
					return -1;
				}
				if(*(var_u4*)tmpval == pge)
				{
					if(m_DID_to_PID.DeleteKey_FL(doc, NULL))
						return -1;
				}

				m_PID_to_DID[pge] = 0;
			}
		}
		fclose(fp);

		m_cur_id++;

		// make idle list
		for(var_u4 i = 0; i < m_cur_id; i++)
		{
			if(m_PID_to_DID[i]) // work id
				continue;

			if(m_idle_pos >= m_idle_max)
			{
				printf("IDA: idle id list out of rang\n");
				return -1;
			}

			m_idle_lst[m_idle_pos++] = i;				
		}

		// rebuild index
		if(rebuild_index())
			return -1;

		// open index file
		m_fp_save = fopen(m_file_save, "rb+");
		if(m_fp_save == NULL)
		{
			printf("IDA: open id table file failure\n");
			return -1;
		}
		fseek(m_fp_save, 0, SEEK_END);

		// start clear up thread
		if(cp_create_thread(thread_ClearUpIndex, this))
		{
			printf("IDA: start clear up thread failure\n");
			return -1;
		}

		printf("IDA: init id allocator success, max id = %u\n", m_cur_id - 1);

		return 0;
	}

	var_4 rebuild_index()
	{
		// clear up index file
		var_1 file_tmp[256];
		sprintf(file_tmp, "%s.tmp", m_file_save);
				
		var_u8 file_len = cp_get_file_size(m_file_save);
		if(file_len % 13)
		{
			printf("IDA: id table file length error, try fix it\n");

			FILE* fp = fopen(m_file_save, "rb+");
			if(fp == NULL)
			{
				printf("IDA: open id table file failure\n");
				return -1;
			}
			if(cp_change_file_size(fp, file_len / 13 * 13))
			{
				fclose(fp);

				printf("IDA: id table file fix failure\n");
				return -1;
			}
			else
				printf("IDA: id table file fix success");
			fclose(fp);
		}
		var_u4 id_num = file_len / 13;

		FILE* fp_w = fopen(file_tmp, "wb");
		if(fp_w == NULL)
		{
			printf("IDA: open %s failure\n", file_tmp);
			return -1;
		}
		FILE* fp_r = fopen(m_file_save, "rb");
		if(fp_r == NULL)
		{
			fclose(fp_w);

			printf("IDA: open %s failure\n", m_file_save);
			return -1;
		}
		var_1 buf[13];
		for(var_u4 i = 0; i < id_num; i++)
		{
			if(fread(buf, 13, 1, fp_r) != 1)
			{
				fclose(fp_r);
				fclose(fp_w);

				printf("IDA: read id table file failure\n");
				return -1;
			}

			if(*buf == 0 || m_PID_to_DID[*(var_u4*)(buf + 1)] == 0)
				continue;

			if(fwrite(buf, 13, 1, fp_w) != 1)
			{
				fclose(fp_r);
				fclose(fp_w);

				printf("IDA: write id table file failure\n");
				return -1;
			}
		}
		fclose(fp_r);
		fclose(fp_w);

		// replace index file
		var_1 file_new[256];
		sprintf(file_new, "%s.new", m_file_save);

		if(access(file_new, 0) == 0 && remove(file_new))
		{
			printf("IDA: remove %s failure\n", file_new);
			return -1;
		}
		if(rename(file_tmp, file_new))
		{
			printf("IDA: rename %s to %s failure\n", file_tmp, file_new);
			return -1;
		}
		if(access(m_file_save, 0) == 0 && remove(m_file_save))
		{
			printf("IDA: remove %s failure\n", m_file_save);
			return -1;
		}
		if(rename(file_new, m_file_save))
		{
			printf("IDA: rename %s to %s failure\n", file_new, m_file_save);
			return -1;
		}

		return 0;
	}

	var_4 doc2page_query(var_u8 org_id, var_u4& new_id)
	{
		void* tmpval = NULL;
		if(m_DID_to_PID.SearchKey_FL(org_id, &tmpval))
			return -1;
		new_id = *(var_u4*)tmpval;

		return 0;
	}

	var_4 doc2page_index(var_u8 org_id, var_u4& new_id)
	{
		m_lock.lock();

		if(m_idle_pos > 0)
			new_id = m_idle_lst[m_idle_pos];
		else
		{
			if(m_cur_id < m_max_id)
				new_id = m_cur_id;
			else
			{
				m_lock.unlock();
				return -1;
			}
		}		

		void* tmpval = NULL;
		if(m_DID_to_PID.SearchKey_FL(org_id, &tmpval) == 0)
			*(var_u4*)tmpval = new_id;			
		else
		{
			if(m_DID_to_PID.AddKey_FL(org_id, (void*)&new_id))
			{
				m_lock.unlock();
				return -1;
			}
		}

		m_PID_to_DID[new_id] = org_id;

		var_1 buf[13];
		*buf = 1;
		*(var_u4*)(buf + 1) = new_id;
		*(var_u8*)(buf + 5) = org_id;

		var_u8 cur_pos = ftell(m_fp_save);
		if(fwrite(buf, 13, 1, m_fp_save) != 1)
		{
			cp_change_file_size(m_fp_save, cur_pos);
			m_lock.unlock();
			return -1;
		}
		if(fflush(m_fp_save))
		{
			cp_change_file_size(m_fp_save, cur_pos);
			m_lock.unlock();
			return -1;
		}

		if(m_idle_pos > 0)
			m_idle_pos--;
		else
			m_cur_id++;

		m_lock.unlock();

		return 0;
	}

	var_4 page2doc(var_u4 org_id, var_u8& new_id)
	{
		if(org_id >= m_cur_id)
		{
			new_id = 0;
			return -1;
		}

		new_id = m_PID_to_DID[org_id];

		return 0;
	}

	var_4 page2doc(var_u4* org_id, var_u8* new_id, var_4 num)
	{
		for(var_4 i = 0; i < num; i++)
		{
			if(org_id[i] >= m_cur_id)
				new_id[i] = 0;
			else
				new_id[i] = m_PID_to_DID[org_id[i]];
		}

		return 0;
	}

	var_4 delpage(var_u4 del_id)
	{	
		m_lock.lock();

		if(del_id >= m_cur_id)
		{
			m_lock.unlock();
			return -1;
		}

		var_u8 doc = m_PID_to_DID[del_id];
		
		void* tmpval = NULL;
		if(m_DID_to_PID.SearchKey_FL(doc, &tmpval))
		{
			m_lock.unlock();
			return -1;
		}
		if(*(var_u4*)tmpval == del_id)
		{
			if(m_DID_to_PID.DeleteKey_FL(doc, NULL))
			{
				m_lock.unlock();
				return -1;
			}
		}

		m_PID_to_DID[del_id] = 0;

		var_1 buf[13];
		*buf = 0;
		*(var_u4*)(buf + 1) = del_id;
		*(var_u8*)(buf + 5) = doc;

		var_u8 cur_pos = ftell(m_fp_save);
		if(fwrite(buf, 13, 1, m_fp_save) != 1)
		{
			cp_change_file_size(m_fp_save, cur_pos);

			m_lock.unlock();
			return -1;
		}
		if(fflush(m_fp_save))
		{
			cp_change_file_size(m_fp_save, cur_pos);

			m_lock.unlock();
			return -1;
		}

		m_lock.unlock();

		return 0;
	}

	static CP_THREAD_T thread_ClearUpIndex(var_vd* vd_ptr)
	{
		UC_IDAllocator_DelayRecycle* cc = (UC_IDAllocator_DelayRecycle*)vd_ptr;

		for(;; cp_sleep(30000))
		{
			struct tm* now_tm;
			time_t now_sec = time(NULL);
			now_tm = localtime(&now_sec);
			if(now_tm->tm_hour != 2 || now_tm->tm_min != 0)
				continue;

			cc->m_lock.lock();

			fclose(cc->m_fp_save);
			
			if(cc->rebuild_index())
				printf("IDA: rebuild index failure\n");

			cc->m_fp_save = fopen(cc->m_file_save, "rb+");
			while(cc->m_fp_save == NULL)
			{
				printf("IDA: open id table file failure\n");
				cp_sleep(1000);

				cc->m_fp_save = fopen(cc->m_file_save, "rb+");
			}
			fseek(cc->m_fp_save, 0, SEEK_END);

			cc->m_lock.unlock();

			cp_sleep(60000);
		}

		return 0;
	}

public:
	var_u4 m_max_id;
	var_u4 m_cur_id;

	var_1 m_file_save[256];
	FILE* m_fp_save;

	var_u8* m_PID_to_DID;
	UT_HashSearch<var_u8> m_DID_to_PID;

	var_u4* m_idle_lst;
	var_u4  m_idle_pos;
	var_u4  m_idle_max;

	CP_MUTEXLOCK m_lock;
};

#endif // __UC_ID_ALLOCATOR_DELAY_RECYCLE_H__
