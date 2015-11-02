// UC_StrIDAllocator.h

#ifndef _UC_StrIDAllocator_H_
#define _UC_StrIDAllocator_H_

#include "UH_Define.h"
#include "UT_HashSearch.h"
#include "UT_Allocator.h"
#include "UC_MD5.h"

class UC_StrIDAllocator
{
public:
	/* ====================  LIFECYCLE     ======================================= */
	UC_StrIDAllocator(){}				/* constructor      */
	~UC_StrIDAllocator(){}				/* destructor       */

	UC_StrIDAllocator(const UC_StrIDAllocator &other);	/* copy constructor */
	UC_StrIDAllocator& operator= (const UC_StrIDAllocator &other);	/* assignment operator */

	/* ====================  INTERFACE     ======================================= */

	/* 
	 * ===  FUNCTION  ======================================================================
	 *         Name:  init
	 *  Description:  初始化并加载数据文件
	 *   Parameters:  max_str_num: 允许的最多标签个数
	 *  ReturnValue:  成功返回0， 失败返回-1
	 * =====================================================================================
	 */
	var_4 init(var_u4 max_str_num, const var_1* save_file)
	{
		if (max_str_num == 0 || save_file == NULL)
			return -1;

		m_max_str_num = max_str_num;
		strcpy(m_save_file, save_file);

		m_i2s_map = new(std::nothrow) var_1*[m_max_str_num];
		if (m_i2s_map == NULL) {
			perror("UC_StrIDAllocator::init() error");
			return -1;
		}
		memset(m_i2s_map, 0, m_max_str_num * sizeof(var_vd*));

		var_4 hash_size = (m_max_str_num / 2) + 1;
		if (m_s2i_map.InitHashSearch(hash_size, sizeof(var_vd*))) {
			fprintf(stderr, "UC_StrIDAllocator::init() error: InitHashSearch failure\n");
			return -1;
		}
		
		m_file_len = 0;
		m_cur_str_num = 0;

		if (access(m_save_file, 0))
		{
			m_save_fp = fopen(m_save_file, "wb+");
			if(m_save_fp == NULL)
			{
				perror("UC_StrIDAllocator::init() error");
				return -1;
			}

			return 0;
		} 
		
		if(cp_fix_file(m_save_file, "!@#$$#@!", 8))
			return -1;
		
		m_save_fp = fopen(m_save_file, "rb+");	
		if (m_save_fp == NULL)
		{
			perror("UC_StrIDAllocator::init() error");
			return -1;
		}

		var_1 read_buf[16];
		
		for ( ; ; ) {

			if (fread(read_buf, 16, 1, m_save_fp) != 1)
				break;
			
			var_4 size = 16 + *(var_4*)(read_buf + 12) + 8;

			var_1* ptr = m_alloc.Allocate(size);
			if(ptr == NULL)
				return -1;

			memcpy(ptr, read_buf, 16);


			if(fread(ptr + 16, *(var_4*)(ptr + 12) + 8, 1, m_save_fp) != 1) 
			{
				perror("UC_StrIDAllocator::init() error");
				return -1;
			}
						
			assert(m_cur_str_num == *(var_u4*)ptr);

			if (m_s2i_map.AddKey_FL(*(var_u8*)(ptr + 4), &ptr) < 0) 
			{
				fprintf(stderr, "UC_StrIDAllocator::init() error: Add Name2ID mapping failure\n");
				return -1;
			}

			m_i2s_map[m_cur_str_num++] = ptr;

			m_file_len += size;
		}
				
		assert(m_file_len == ftell(m_save_fp));

		return 0;
	}

	/* 
	 * ===  FUNCTION  ======================================================================
	 *         Name:  str2id
	 *  Description:  通过字符串获取映射ID
	 *   Parameters:  str:
	 *                strlen:
	 *                id:
	 *  ReturnValue:  已经存在返回1，成功返回0， 失败返回-1
	 * =====================================================================================
	 */
	var_4 str2id(const var_1* str, var_u4 str_len, var_u4& id)
	{
		assert(str && str_len);

		var_u8 md5 = m_md5.MD5Bits64((var_u1*)str, str_len);
		
		m_locker.lock();

		var_vd* val = NULL;
		if (m_s2i_map.SearchKey_FL(md5, &val) == 0) {
			id = **(var_u4**)val;

			m_locker.unlock();
			return 1;
		}

		if (m_cur_str_num >= m_max_str_num) {
			m_locker.unlock();

			fprintf(stderr, "UC_StrIDAllocator::str2id() error: str id out of range\n");
			return -1;
		}

		var_u4 size = 4 + 8 + 4 + str_len + 8; // id, md5, len, str, flg

		var_1* buf = m_alloc.Allocate(size);
		if(buf == NULL)
		{
			m_locker.unlock();
			fprintf(stderr, "UC_StrIDAllocator::str2id() error: alloc memory error\n");

			return -1;
		}
		
		*(var_u4*)buf = m_cur_str_num;
		*(var_u8*)(buf + 4) = md5;
		*(var_u4*)(buf + 12) = str_len;
		memcpy(buf + 16, str, str_len);
		*(var_u8*)(buf + size - 8) = *(var_u8*)"!@#$$#@!";

		if (fwrite(buf, size, 1, m_save_fp) != 1) {
			cp_change_file_size(m_save_fp, m_file_len);
			m_locker.unlock();
			
			fprintf(stderr, "UC_StrIDAllocator::str2id() error: save in file error\n");
			return -1;
		}

		if (fflush(m_save_fp)) {
			cp_change_file_size(m_save_fp, m_file_len);
			m_locker.unlock();
			
			fprintf(stderr, "UC_StrIDAllocator::str2id() error: save in file error\n");
			return -1;
		}

		if (m_s2i_map.AddKey_FL(md5, (var_vd*)&buf)) {
			cp_change_file_size(m_save_fp, m_file_len);
			m_locker.unlock();
			
			fprintf(stderr, "UC_TagIDAllocator::name2id() error: Add Name2ID mapping failure\n");
			return -1;
		}

		m_file_len += size;
		
		m_i2s_map[m_cur_str_num] = buf;

		id = m_cur_str_num++;

		m_locker.unlock();

		return 0;
	}
	
	/* 
	 * ===  FUNCTION  ======================================================================
	 *         Name:  id2str
	 *  Description:  通过ID号获取映射字符串
	 *   Parameters:  id:
	 *                strlen:
	 *                str:
	 *  ReturnValue:  成功返回0， 失败返回-1
	 * =====================================================================================
	 */
	var_4 id2str(var_u4 id, const var_1*& str, var_u4& str_len, var_u8& md5)
	{
		if (id >= m_cur_str_num)
			return -1;

		var_1* buf = m_i2s_map[id];
		if(buf == NULL)
			return -1;
		
		assert(id == *(var_u4*)buf);
		
		md5 = *(var_u8*)(buf + 4);
		str_len = *(var_u4*)(buf + 12);
		str = (var_1*)(buf + 16);

		return 0;
	}

private:
	/* ====================  DATA MEMBERS  ======================================= */
	var_u4 m_max_str_num;
	var_u4 m_cur_str_num;

	var_1 m_save_file[256];
	FILE* m_save_fp;

	var_8 m_file_len;

	var_1** m_i2s_map;
	UT_HashSearch<var_u8> m_s2i_map;

	UT_Allocator<var_1> m_alloc;
			
	CP_MUTEXLOCK m_locker;

	UC_MD5 m_md5;
};

#endif	// _UC_StrIDAllocator_H_
