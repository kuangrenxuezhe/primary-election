// UC_IDAllocator.h

#ifndef __UC_ID_ALLOCATOR_H__
#define __UC_ID_ALLOCATOR_H__

#include "UH_Define.h"
#include "UT_HashSearch.h"

class UC_IDAllocator
{
public:
	UC_IDAllocator()
	{
		m_max_id_num = 0;
		m_cur_id_num = 0;
		
		m_fp_save = NULL;
		m_PID_to_DID = NULL;;
	}

	~UC_IDAllocator()
	{
		if(m_fp_save)
			fclose(m_fp_save);
		if(m_PID_to_DID)
			delete m_PID_to_DID;
	}

	var_4 init(var_u4 max_id_num, var_1* idx_save_file)
	{
		m_max_id_num = max_id_num;
		strcpy(m_idx_save_file, idx_save_file);

		m_PID_to_DID = new var_u8[m_max_id_num];
		if(m_PID_to_DID == NULL)
			return -1;
		memset(m_PID_to_DID, 0, m_max_id_num * sizeof(var_u8));

		var_4 hash_size = m_max_id_num / 3;
		if(hash_size <= 0)
			hash_size = 1;
		if(m_DID_to_PID.InitHashSearch(hash_size, 4))
			return -1;

		if(access(m_idx_save_file, 0))
			m_fp_save = fopen(m_idx_save_file, "wb");
		else
			m_fp_save = fopen(m_idx_save_file, "rb+");
		if(m_fp_save == NULL)
			return -1;

		struct stat info;
		stat(m_idx_save_file, &info);
		if(info.st_size % 12)
			printf("IDA warning: id table file length error, try fix it\n");
		var_4 file_pos = info.st_size / 12 * 12;
		if(fseek(m_fp_save, file_pos, SEEK_SET))
		{
			printf("IDA error: seek id table file failure\n");
			return -1;
		}

		return 0;
	}

	var_4 load()
	{
		if(access(m_idx_save_file, 0))
			return 0;

		struct stat info;
		stat(m_idx_save_file, &info);
		if(info.st_size % 12)
			printf("IDA warning: id table file length error\n");

		var_u4 id_num = info.st_size / 12;
		if(id_num == 0)
		{
			printf("load ID table success, next id = %u\n", m_cur_id_num);
			return 0;
		}

		var_u8 doc_id = 0;
		var_u4 page_id = 0;
		var_1  id_buf[12];

		FILE* fp = fopen(m_idx_save_file, "rb");
		if(fp == NULL)
			return -1;
		for(var_u4 i = 0; i < id_num; i++)	
		{
			if(fread(id_buf, 12, 1, fp) != 1)
				break;

			doc_id = *(var_u8*)id_buf;
			page_id = *(var_u4*)(id_buf + 8);

			if(m_cur_id_num >= m_max_id_num)
			{
				fclose(fp);

				printf("IDA error: id out of rang\n");			
				return -1;
			}
			if(m_cur_id_num < page_id)
				m_cur_id_num = page_id;

			m_PID_to_DID[page_id] = doc_id;
			var_4 retval = m_DID_to_PID.AddKey_FL(doc_id, (void*)&page_id);
			if(retval < 0)
			{
				fclose(fp);

				printf("IDA error: add DocID to table failure\n");			
				return -1;
			}
			else if(retval > 0)
			{
				fclose(fp);

				printf("IDA error: DID repeat\n");			
				return -1;
			}	
		}
		fclose(fp);

		m_cur_id_num++;

		printf("load ID table success, max id = %u, next id = %u\n", m_cur_id_num - 1, m_cur_id_num);

		return 0;
	}

	var_4 doc2page(var_u8 org_id, var_u4& new_id)
	{
		void* tmpval = NULL;
		if(m_DID_to_PID.SearchKey_FL(org_id, &tmpval) == 0)
		{
			new_id = *(var_u4*)tmpval;
			return 1;
		}

		if(m_cur_id_num >= m_max_id_num)
			return -1;

		var_1  tmp_buf[12];
		*(var_u8*)tmp_buf = org_id;
		*(var_u4*)(tmp_buf + 8) = m_cur_id_num;

		var_u8 cur_pos = ftell(m_fp_save);
		if(fwrite(tmp_buf, 12, 1, m_fp_save) != 1)
		{
			cp_change_file_size(m_fp_save, cur_pos);
			return -1;
		}
		if(fflush(m_fp_save))
		{
			cp_change_file_size(m_fp_save, cur_pos);
			return -1;
		}

		var_4 retval = m_DID_to_PID.AddKey_FL(org_id, (void*)&m_cur_id_num);
		if(retval != 0)
			return -1;

		m_PID_to_DID[m_cur_id_num] = org_id;

		new_id = m_cur_id_num++;

		return 0;		
	}

	var_4 page2doc(var_u4 org_id, var_u8& new_id)
	{
		if(org_id >= m_cur_id_num)
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
			if(org_id[i] >= m_cur_id_num)
				new_id[i] = 0;
			else
				new_id[i] = m_PID_to_DID[org_id[i]];
		}

		return 0;
	}

public:
	var_u4 m_max_id_num;
	var_u4 m_cur_id_num;
	var_1  m_idx_save_file[256];

	FILE* m_fp_save;

	var_u8* m_PID_to_DID;
	UT_HashSearch<var_u8> m_DID_to_PID;		
};

#endif // __UC_ID_ALLOCATOR_H__
