// UC_IDAllocator_Recycle.h

#ifndef _UC_IDAllocator_Recycle_H_
#define _UC_IDAllocator_Recycle_H_

#include "UH_Define.h"
#include "UT_HashSearch.h"

/*
 * =====================================================================================
 *        Class:  ID_Allocater
 *  Description:  8字节DocID与4字节PageID映射与PageID的回收循环利用
 * =====================================================================================
 */
class UC_IDAllocator_Recycle
{
public:
	UC_IDAllocator_Recycle(){}
	~UC_IDAllocator_Recycle(){}
	
	UC_IDAllocator_Recycle(const UC_IDAllocator_Recycle &other);		/* 函数声明不实现，禁止拷贝 */
	UC_IDAllocator_Recycle& operator= (const UC_IDAllocator_Recycle &other);	/* 函数声明不实现，禁止赋值 */

	/* 
	 * ===  FUNCTION  ======================================================================
	 *         Name:  init
	 *  Description:  初始化映射表，加载落地文件，并分配资源
	 *   Parameters:  max_id_num：允许分配的PageID最大值
	 *                save_file：保存映射表落地文件的文件名
	 *  ReturnValue:  正确返回0，错误返回-1
	 * =====================================================================================
	 */
	var_4 init(var_u4 max_id_num, const var_1* save_file)
	{
		if(max_id_num == 0 || save_file == NULL)
			return -1;
		
		if (strlen(save_file) > 200)
			return -1;

		m_max_size = max_id_num;
		m_max_barrier = 0;

		sprintf(m_save_file, "%s", save_file);
		sprintf(m_temp_file, "%s.tmp", save_file);

		m_p2d_map = new(std::nothrow) var_u8[m_max_size];
		if (m_p2d_map == NULL) {
			perror("UC_IDAllocator_Recycle::init() error");
			return -1;
		}
		memset(m_p2d_map, 0, m_max_size<<3);

		m_bm_flag = new(std::nothrow) var_1[BM_LEN_1BYTE(m_max_size)];
		if (m_p2d_map == NULL) {
			perror("UC_IDAllocator_Recycle::init() error");
			return -1;
		}
		memset(m_bm_flag, 0xff, BM_LEN_1BYTE(m_max_size));

		var_4 hash_size = (m_max_size / 2) + 1;
		if (m_d2p_map.InitHashSearch(hash_size, 4)) {
			fprintf(stderr, "UC_IDAllocator_Recycle::init() error: InitHashSearch failure\n");
			return -1;
		}

		if (access(m_save_file, 0) == 0) {
			m_save_fp = fopen(m_save_file, "rb+");
		} else {
			if (cp_recovery_file(m_save_file) == -1)
				return -1;
			m_save_fp = fopen(m_save_file, "wb+");
		}
		if (m_save_fp == NULL){
			perror("UC_IDAllocator_Recycle::init() error");
			return -1;
		}

		m_file_len = cp_get_file_size(m_save_file);
		
		var_8 diff = m_file_len % 12;
		if (diff != 0) {
			printf("UC_IDAllocator_Recycle::init() warning: id table file length error, try fix it\n");
			m_file_len -= diff;
		}
		
		if (m_file_len == 0) {

			m_cur_pos = 0;
			m_cur_size = 0;
			
			for (var_u4 i = 0; i < m_max_size; i++)
				m_p2d_map[i] = i + 1;

			return 0;
		}

		// 重建 m_p2d_map 和 m_d2p_map 映射表
		
		var_4 num = (var_4)(m_file_len / 12);
		
		for(var_4 i = 0; i < num; i++)
		{
			var_1 buffer[12];
			
			if (fread(buffer, 12, 1, m_save_fp) != 1)
			{
				fclose(m_save_fp);
				return -1;
			}

			var_u8 doc_id = *(var_u8*)buffer;
			var_u4 page_id = *(var_u4*)(buffer + 8);

			if (page_id >= m_max_size) {
				fprintf(stderr, "UC_IDAllocator_Recycle::init() error: id out of range\n");
				return -1;
			}

			if (doc_id == 0) {
				if (BM_GET_BIT(m_bm_flag, page_id) == 0 && m_d2p_map.DeleteKey_FL(m_p2d_map[page_id]))
				{
					fprintf(stderr, "UC_IDAllocator_Recycle::init() error: id delete hash error\n");
					return -1;
				}
				
				BM_SET_BIT(m_bm_flag, page_id);

				continue;
			}

			var_vd* ret_buf = NULL;
			
			var_4 ret_val = m_d2p_map.AddKey_FL(doc_id, &page_id, &ret_buf);
			
			if(ret_val < 0) {
				fprintf(stderr, "UC_IDAllocator_Recycle::init() error: id add hash error\n");
				return -1;
			}
			
			if (ret_val == 1 && *(var_u4*)ret_buf != page_id) {
				fprintf(stderr, "UC_IDAllocator_Recycle::init() error: repeat add hash error\n");
				return -1;
			}
			
			m_p2d_map[page_id] = doc_id;
			
			BM_SET_BIT_ZERO(m_bm_flag, page_id);
		}

		m_cur_pos = m_max_size;
		m_cur_size = 0;

		for(var_u4 i = 0; i < m_max_size; i++)
		{
			var_u4 n = m_max_size - 1 - i;
			if(BM_GET_BIT(m_bm_flag, n) == 0)
			{
				m_cur_size++;

				if (n + 1 > m_max_barrier)
					m_max_barrier = n + 1;

				continue;
			}

			m_p2d_map[n] = m_cur_pos;
			m_cur_pos = n;
		}

		return 0;
	}
	
	/* 
	 * ===  FUNCTION  ======================================================================
	 *         Name:  doc2page
	 *  Description:  通过给定的8字节DocID，获得PageID
	 *   Parameters:  doc_id: 给定的DocID
	 *                page_id: 用参数的形式返回page_id
	 *  ReturnValue:  找到已存在的PageId返回1，分配新的PageID返回0，错误返回-1 
	 * =====================================================================================
	 */
	var_4 doc2page(var_u8 doc_id, var_u4 &page_id)
	{
		m_locker.lock();
		
		var_vd *val = NULL;
		if (m_d2p_map.SearchKey_FL(doc_id, &val) == 0) {
			page_id = *(var_u4*)val;
			
			m_locker.unlock();
			
			return 1;
		}

		if (m_cur_size >= m_max_size) {
			m_locker.unlock();
			
			fprintf(stderr, "UC_IDAllocator_Recycle::doc2page() error: there is no more PageID to allocate\n");
			return -1;
		}
		
		page_id = m_cur_pos;
		
		if (save_in_file(doc_id, m_cur_pos) == -1) {
			m_locker.unlock();
			
			fprintf(stderr, "UC_IDAllocator_Recycle::doc2page() error: save in file error\n");
			return -1;
		}

		if (m_d2p_map.AddKey_FL(doc_id, (var_vd*)&page_id)) {
			m_locker.unlock();
			
			fprintf(stderr, "UC_IDAllocator_Recycle::doc2page() error: Add Doc2Page mapping failure\n");
			return -1;
		}

		m_cur_pos = static_cast<var_u4>(m_p2d_map[m_cur_pos]);
		m_p2d_map[page_id] = doc_id;

		BM_SET_BIT_ZERO(m_bm_flag, page_id);

		if (page_id + 1 > m_max_barrier)
			m_max_barrier = page_id + 1;

		m_cur_size++;
		
		m_locker.unlock();

		return 0;
	}		/* -----  end of function doc2page  ----- */

	/* 
	 * ===  FUNCTION  ======================================================================
	 *         Name:  doc2page_find
	 *  Description:  查找DocID对应的PageID，PageID不存在也不进行分配 
	 *   Parameters:  doc_id: 给定的DocID
	 *                page_id: 用参数的形式返回page_id
	 *  ReturnValue:  找到已存在的PageId返回0，错误返回-1 
	 * =====================================================================================
	 */
	var_4 doc2page_find(var_u8 doc_id, var_u4 &page_id)
	{
		m_locker.lock();
		
		var_vd *val = NULL;
		if (m_d2p_map.SearchKey_FL(doc_id, &val) == 0) {
			page_id = *(var_u4*)val;
			
			m_locker.unlock();
			
			return 0;
		}

		m_locker.unlock();

		return -1;
	}		/* -----  end of function doc2page_find  ----- */

	/* 
	 * ===  FUNCTION  ======================================================================
	 *         Name:  page2doc
	 *  Description:  通过给定的4字节PageID，获得8字节DocID
	 *   Parameters:  page_id: 给定的PageID
	 *                doc_id: 用参数的形式返回doc_id
	 *  ReturnValue:  成功返回0，失败返回-1
	 * =====================================================================================
	 */
	var_4 page2doc(var_u4 page_id, var_u8 &doc_id)
	{
		if(page_id >= m_max_size)
			return -1;
		
		if (BM_GET_BIT(m_bm_flag, page_id))
			return -1;
		
		doc_id = m_p2d_map[page_id];
		
		return 0;
	}
	
	var_u8* get_page2doc_map()
	{
		return m_p2d_map;
	}

	void get_bm_and_barrier(var_1* &bm, var_u4 &max_barrier)
	{
		bm = m_bm_flag;
		max_barrier = m_max_barrier;
	}		/* -----  end of function get_used_and_bm  ----- */

	/* 
	 * ===  FUNCTION  ======================================================================
	 *         Name:  recycle_page_id
	 *  Description:  通过DocID删除映射表项，并回收PageID
	 *   Parameters:  需要删除的DocID
	 *  ReturnValue:  成功返回0，ID不存在返回1，失败返回-1
	 * =====================================================================================
	 */
	var_4 recycle_page_id(var_u8 doc_id)
	{
		m_locker.lock();

		var_vd* val = NULL;
		if (m_d2p_map.SearchKey_FL(doc_id, &val) != 0) {
			m_locker.unlock();
			
			fprintf(stderr, "UC_IDAllocator_Recycle::recycle_page_id(did) error: no such DocID in mapping table\n");
			return 1;
		}

		var_u4 page_id = *(var_u4*)val;
		
		if (save_in_file(0LU, page_id) == -1) {
			m_locker.unlock();
			
			fprintf(stderr, "UC_IDAllocator_Recycle::recycle_page_id(did) error: save in file error\n");
			return -1;
		}
		
		if (m_d2p_map.DeleteKey_FL(doc_id)) {
			m_locker.unlock();
			
			fprintf(stderr, "UC_IDAllocator_Recycle::recycle_page_id(did) error: can't delete hash node\n");
			return -1;
		}

		m_p2d_map[page_id] = m_cur_pos;
		m_cur_pos = page_id;

		BM_SET_BIT(m_bm_flag, page_id);

		if (page_id + 1 == m_max_barrier) {
			var_u4 i;
			for (i=page_id-1; i!=0xffffffff; --i) {
				if (BM_GET_BIT(m_bm_flag, i) == 0)
					break;
			}
			m_max_barrier = i + 1;
		}

		m_cur_size--;

		m_locker.unlock();

		return 0;
	}

	/* 
	 * ===  FUNCTION  ======================================================================
	 *         Name:  recycle_page_id
	 *  Description:  通过PageID删除映射表项，并回收PageID
	 *   Parameters:  需要回收的PageID
	 *  ReturnValue:  成功返回0，ID不存在返回1，失败返回-1
	 * =====================================================================================
	 */
	var_4 recycle_page_id(var_u4 page_id)
	{
		if(page_id >= m_max_size)
			return -1;

		m_locker.lock();
		
		if (BM_GET_BIT(m_bm_flag, page_id)) {
			m_locker.unlock();

			fprintf(stderr, "UC_IDAllocator_Recycle::recycle_page_id(pid) error: no such PageID in mapping table\n");
			return 1;
		}

		var_u8 doc_id = m_p2d_map[page_id];
		
		if (save_in_file(0LU, page_id) == -1) {
			m_locker.unlock();
			
			fprintf(stderr, "UC_IDAllocator_Recycle::recycle_page_id(pid) error: save in file error\n");
			return -1;
		}
		
		if (m_d2p_map.DeleteKey_FL(doc_id)) {
			m_locker.unlock();
			
			fprintf(stderr, "UC_IDAllocator_Recycle::recycle_page_id(pid) error: can't delete hash node\n");
			return -1;
		}

		m_p2d_map[page_id] = m_cur_pos;
		m_cur_pos = page_id;

		BM_SET_BIT(m_bm_flag, page_id);

		m_cur_size--;

		if (page_id + 1 == m_max_barrier) {
			var_u4 i;
			for (i=page_id-1; i!=0xffffffff; --i) {
				if (BM_GET_BIT(m_bm_flag, i) == 0)
					break;
			}
			m_max_barrier = i + 1;
		}

		m_locker.unlock();

		return 0;
	}

	/* 
	 * ===  FUNCTION  ======================================================================
	 *         Name:  get_current_size
	 *  Description:  获得当前已分配的PageID个数
	 *   Parameters:  无
	 *  ReturnValue:  返回当前已分配的PageID个数
	 * =====================================================================================
	 */
	inline var_u4 get_current_size()
	{
		return m_cur_size;
	}

	/* 
	 * ===  FUNCTION  ======================================================================
	 *         Name:  organize_mapping_file
	 *  Description:  整理当前的映射文件
	 *   Parameters:  无
	 *  ReturnValue:  成功返回0，失败返回-1
	 * =====================================================================================
	 */
	var_4 organize_mapping_file()
	{
		FILE* m_temp_fp = fopen(m_temp_file, "wb");
		if (m_temp_fp == NULL)
		{
			perror("UC_IDAllocator_Recycle::organize_mapping_file() error");
			return -1;
		}

		m_locker.lock();

		var_u8 file_len = 0;
		
		for (var_u4 i = 0; i < m_max_size; i++)
		{
			if (BM_GET_BIT(m_bm_flag, i))
				continue;

			var_u8 doc_id = m_p2d_map[i];
			
			var_1 buffer[12];
			
			*(var_u8*)buffer = doc_id;
			*(var_u4*)(buffer + 8) = i;

			if (fwrite(buffer, 12, 1, m_temp_fp) != 1)
			{
				fclose(m_temp_fp);
				
				m_locker.unlock();
				
				perror("UC_IDAllocator_Recycle::organize_mapping_file() write file error");
				return -1;
			}
			
			file_len += 12;
		}

		fclose(m_temp_fp);
		
		fclose(m_save_fp);
		
		cp_swap_file(m_temp_file, m_save_file);
		
		for(;;)
		{
			m_save_fp = fopen(m_save_file, "rb+");
			if(m_save_fp)
				break;
			
			perror("UC_IDAllocator_Recycle::organize_mapping_file() open file error");
			cp_sleep(5000);
		};
				
		m_file_len = file_len;
		while(fseek(m_save_fp, m_file_len, SEEK_SET))
		{
			perror("UC_IDAllocator_Recycle::organize_mapping_file() seek file error");
			cp_sleep(5000);
		}
		
		m_locker.unlock();

		return 0;
	}

private:
	/* ==================== PRIVETE METHOD ======================================= */
	/* 
	 * ===  FUNCTION  ======================================================================
	 *         Name:  save_in_file
	 *  Description:  将DocID和PageID保存到文件，8字节DocID在前，4字节PageID在后
	 *   Parameters:  略
	 *  ReturnValue:  成功返回0，失败返回-1
	 * =====================================================================================
	 */
	var_4 save_in_file(var_u8 doc_id, var_u4 page_id)
	{
		var_1 buf[12];
		*(var_u8*)buf = doc_id;
		*(var_u4*)(buf + 8) = page_id;

		if (fwrite(buf, 12, 1, m_save_fp) != 1) {
			cp_change_file_size(m_save_fp, m_file_len);
			return -1;
		}

		if (fflush(m_save_fp)) {
			cp_change_file_size(m_save_fp, m_file_len);
			return -1;
		}

		m_file_len += 12;

		return 0;
	}

private:
	/* ====================  DATA MEMBERS  ======================================= */
	var_1 m_save_file[256];
	var_1 m_temp_file[256];	// 整理文件时使用的临时文件
	FILE* m_save_fp;
	
	CP_MUTEXLOCK m_locker;
	
	var_8  m_file_len;
	
	var_u4 m_max_size; // 最大库容量
	var_u4 m_cur_size; // 当前库容量
	var_u4 m_max_barrier; // 当前有效ID的最大值
	
	var_u8* m_p2d_map;	// pageID到DocID映射表的地址，0下标空闲不使用
	var_u4  m_cur_pos;	// 当前待分配的PageID，没有使用0下标保存cur_id主要式考虑到cache命中

	UT_HashSearch<var_u8> m_d2p_map;	// DocID到PageID的映射表
	
	var_1* m_bm_flag;
};

#endif	// _UC_IDAllocator_Recycle_H_
