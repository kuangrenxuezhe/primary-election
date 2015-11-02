// UT_Mem_Allocator_NoRecycle.h

#ifndef __UT_MEM_ALLOCATOR_NORECYCLE_H__
#define __UT_MEM_ALLOCATOR_NORECYCLE_H__

#include "UH_Define.h"

#define UT_MANR_DEFAULT_BLOCK_SIZE	1048576

#define UT_MANR_STAT_INFO // open stat information
#define UT_MANR_WARNING // open warning information

template <class T>
struct UT_MANR_NODE
{
	T*    mem_ptr;
	var_8 mem_len;
	
	struct UT_MANR_NODE* next;
};

template <class T>
class UT_Mem_Allocator_NoRecycle
{
public:
	UT_Mem_Allocator_NoRecycle(var_8 default_block_size = UT_MANR_DEFAULT_BLOCK_SIZE)
	{
		if(default_block_size <= 0)
			m_default_block_size = UT_MANR_DEFAULT_BLOCK_SIZE;
		else
			m_default_block_size = default_block_size;

		m_block_idle_normal = NULL;
		m_block_idle_big = NULL;
		
		m_block_lib_normal = NULL;
		m_block_lib_big = NULL;
				
		m_cur_use_size = 0;
		m_cur_use_node = NULL;
		
		m_is_free_memory = 0;
		
		// stat infomation
		m_stat_big_size = 0;
		m_stat_big_size_max = 0;
		m_stat_big_time = 0;
		m_stat_big_time_max = 0;
		
		m_stat_normal_time = 0;
		m_stat_normal_time_max = 0;
		
		m_stat_request_size = 0;
	}

	~UT_Mem_Allocator_NoRecycle()
	{
		reset_memory(1);
		
		while(m_block_idle_normal)
		{
			UT_MANR_NODE<T>* node = m_block_idle_normal;
			m_block_idle_normal = m_block_idle_normal->next;
			
			delete node;
		}
		
		while(m_block_idle_big)
		{
			UT_MANR_NODE<T>* node = m_block_idle_big;
			m_block_idle_big = m_block_idle_big->next;
			
			delete node;
		}
	}

	T* request_memory(var_8 request_size)
	{
		if(request_size <= 0)
			return NULL;
		
		if(request_size > m_default_block_size)
		{
			m_block_lock_big.lock();
			
			UT_MANR_NODE<T>* node = make_node_big(request_size);
			if(node == NULL)
			{
				m_block_lock_big.unlock();
				return NULL;
			}
			
			node->next = m_block_lib_big;
			m_block_lib_big = node;
			
#ifdef UT_MANR_STAT_INFO
			__sync_add_and_fetch(&m_stat_request_size, request_size);
#endif
			
			m_block_lock_big.unlock();
			
			return node->mem_ptr;
		}
		
		m_block_lock_normal.lock();
		
		if(request_size > m_default_block_size - m_cur_use_size || m_cur_use_node == NULL)
		{
			UT_MANR_NODE<T>* node = make_node_normal();
			if(node == NULL)
			{
				m_block_lock_normal.unlock();
				return NULL;
			}
						
			node->next = m_block_lib_normal;
			m_block_lib_normal = node;
			
			m_cur_use_node = node;
			m_cur_use_size = 0;
		}

		T* mem_ptr = m_cur_use_node->mem_ptr + m_cur_use_size;
		m_cur_use_size += request_size;
		
#ifdef UT_MANR_STAT_INFO
		__sync_add_and_fetch(&m_stat_request_size, request_size);
#endif
		
		m_block_lock_normal.unlock();

		return mem_ptr;
	}

	var_8 reset_memory(var_4 is_free_memory = 0)
	{
		m_block_lock_big.lock();
		m_block_lock_normal.lock();
		
		m_is_free_memory = is_free_memory;
		
		UT_MANR_NODE<T>* node = NULL;
		
		if(m_is_free_memory == 1)
			free_idle();
		
		while(m_block_lib_normal)
		{
			node = m_block_lib_normal;
			m_block_lib_normal = m_block_lib_normal->next;
			
			free_node(node);
		}
		
		while(m_block_lib_big)
		{
			node = m_block_lib_big;
			m_block_lib_big = m_block_lib_big->next;
			
			free_node(node);
		}
		
		m_cur_use_node = NULL;
		m_cur_use_size = 0;
		
#ifdef UT_MANR_STAT_INFO
		if(m_stat_big_size > m_stat_big_size_max)
			m_stat_big_size_max = m_stat_big_size;
		
		if(m_stat_big_time > m_stat_big_time_max)
			m_stat_big_time_max = m_stat_big_time;
		
		if(m_stat_normal_time > m_stat_normal_time_max)
			m_stat_normal_time_max = m_stat_normal_time;
		
		m_stat_request_size = 0;
#endif
		
		m_block_lock_normal.unlock();
		m_block_lock_big.unlock();

		return 0;
	}

	// 得到当前版本号
	const var_1* version()
	{
		// v1.000 - 2013.07.18 - 初始版本
		return "v1.000";
	}

private:
	UT_MANR_NODE<T>* make_node_normal()
	{
		T* mem_ptr = NULL;
		UT_MANR_NODE<T>* node = NULL;
		
		if(m_is_free_memory == 1 || m_block_idle_normal == NULL)
		{
			mem_ptr = new(std::nothrow) T[m_default_block_size];
			if(mem_ptr == NULL)
				return NULL;
		}
		
		if(m_block_idle_normal)
		{
			node = m_block_idle_normal;
			m_block_idle_normal = m_block_idle_normal->next;
		}
		else
		{
			node = new(std::nothrow) UT_MANR_NODE<T>;
			if(node == NULL)
			{
				delete[] mem_ptr;
				return NULL;
			}
			
			node->mem_ptr = NULL;
		}
		
		if(node->mem_ptr == NULL)
		{
			node->mem_ptr = mem_ptr;
			node->mem_len = m_default_block_size;
			
#ifdef UT_MANR_STAT_INFO
			m_stat_normal_time++;
#endif
		}

		node->next = NULL;

		return node;
	}
	
	UT_MANR_NODE<T>* make_node_big(var_8 size)
	{
		UT_MANR_NODE<T>* node = NULL;
		
		T* mem_ptr = new(std::nothrow) T[size];
		if(mem_ptr == NULL)
			return NULL;
		
		if(m_block_idle_big)
		{
			node = m_block_idle_big;
			m_block_idle_big =  m_block_idle_big->next;
		}
		else
		{
			node = new(std::nothrow) UT_MANR_NODE<T>;
			if(node == NULL)
			{
				delete[] mem_ptr;
				return NULL;
			}
		}
		
		node->mem_ptr = mem_ptr;
		node->mem_len = size;
		
#ifdef UT_MANR_STAT_INFO
		m_stat_big_size += size;
		m_stat_big_time++;
		
#ifdef UT_MANR_WARNING
		if(m_stat_big_time > m_stat_normal_time)
			printf("WARNING: UT_Mem_Allocator_NoRecycle: big memory too much, check code!\n");
#endif
		
#endif
		
		node->next = NULL;
		
		return node;
	}
	
	void free_node(UT_MANR_NODE<T>* node)
	{
		var_8 mem_len = node->mem_len;
		
		if(m_is_free_memory == 1 || node->mem_len > m_default_block_size)
		{
			delete[] node->mem_ptr;
			node->mem_ptr = NULL;
			node->mem_len = 0;
		}
		
		if(mem_len > m_default_block_size)
		{
			node->next = m_block_idle_big;
			m_block_idle_big = node;
		}
		else
		{
			node->next = m_block_idle_normal;
			m_block_idle_normal = node;
		}
		
		return;
	}
	
	void free_idle()
	{
		UT_MANR_NODE<T>* node = m_block_idle_normal;
		
		while(node)
		{
			if(node->mem_ptr)
			{
				delete[] node->mem_ptr;
				node->mem_ptr = NULL;
				node->mem_len = 0;
			}
			
			node = node->next;
		}
	}
	
private:
	var_8 m_default_block_size;
	
	UT_MANR_NODE<T>* m_block_idle_normal;
	UT_MANR_NODE<T>* m_block_idle_big;
	
	UT_MANR_NODE<T>* m_block_lib_normal;
	UT_MANR_NODE<T>* m_block_lib_big;

	var_8            m_cur_use_size;
	UT_MANR_NODE<T>* m_cur_use_node;
	
	var_4 m_is_free_memory; // zero is no free
	
	CP_MUTEXLOCK m_block_lock_big;
	CP_MUTEXLOCK m_block_lock_normal;

	// stat infomation
	var_8 m_stat_big_size;
	var_8 m_stat_big_size_max;
	var_8 m_stat_big_time;
	var_8 m_stat_big_time_max;
	
	var_8 m_stat_normal_time;
	var_8 m_stat_normal_time_max;
	
	var_8 m_stat_request_size;
};

#endif // __UT_MEM_ALLOCATOR_NORECYCLE_H__
