// UC_Mem_Allocator_Recycle.h

#ifndef __UC_MEM_ALLOCATOR_RECYCLE_H__
#define __UC_MEM_ALLOCATOR_RECYCLE_H__

#include "UH_Define.h"

#define UC_MAR_BLOCK_NUM_PER_SEGMENT	1024
#define UC_MAR_BLOCK_NUM_MAX			0

#define UC_MAR_STAT_INFO  // open stat information
#define UC_MAR_WARNING    // open warning information
#define UC_MAR_DEBUG_INFO // open debug information
#define UC_MAR_DEBUG_INFO_USE		1
#define UC_MAR_DEBUG_INFO_NO_USE	0

class UC_Mem_Allocator_Recycle
{	
public:
	UC_Mem_Allocator_Recycle()
	{
        m_init_finish = 0;
	}
	
	~UC_Mem_Allocator_Recycle()
	{
        if(m_init_finish)
            reset_mem(1);
	}

	var_4 init(var_8 block_size, var_8 block_count = UC_MAR_BLOCK_NUM_MAX, var_8 block_num_per_segment = UC_MAR_BLOCK_NUM_PER_SEGMENT)
	{		
		if(block_size <= 0)
		{
			printf("#*#*#*#* UC_Mem_Allocator_Recycle: block size is illegal\n");
			return -1;
		}
		
#ifdef UC_MAR_DEBUG_INFO
		printf("#*#*#*#*  UC_Mem_Allocator_Recycle: debug information is open  #*#*#*#*\n");
#endif

		m_block_size = block_size + sizeof(var_vd*);
		
#ifdef UC_MAR_DEBUG_INFO
		m_block_size += sizeof(var_4) + sizeof(var_4) + sizeof(var_4);
#endif
		
		if(block_count < 0)
		{
			printf("#*#*#*#* UC_Mem_Allocator_Recycle: block count is illegal\n");
			return -1;
		}
		else if(block_count == UC_MAR_BLOCK_NUM_MAX)
			m_block_count_max = 0x7FFFFFFFFFFFFFFF;
		else
			m_block_count_max = block_count;
		
		
		if(block_num_per_segment <= 0)
		{
			printf("#*#*#*#* UC_Mem_Allocator_Recycle: block num per segment is illegal\n");
			return -1;
		}
		else
			m_block_numPs = block_num_per_segment;
        
        m_block_count_use = 0;

		m_recycle_library = NULL;
		m_segment_library = NULL;
		
		m_segment_library_idle = NULL;
		
		m_cur_seg_pos = NULL;
		m_cur_seg_use = 0;

        m_init_finish = 1;
        
		return 0;
	}
	
	var_vd* get_mem()
	{
		m_mem_locker.lock();
		
		if(m_block_count_use >= m_block_count_max)
		{
			m_mem_locker.unlock();
			return NULL;
		}
		
		if(m_recycle_library)
		{
			var_1* buffer = m_recycle_library;
			
#ifdef UC_MAR_DEBUG_INFO
                        buffer -= sizeof(var_4) + sizeof(var_4);
                        
			assert(*(var_4*)buffer == 0xABABABAB);
			assert(*(var_4*)(buffer + m_block_size - sizeof(var_4)) == 0xBABABABA);
			assert(*(var_4*)(buffer + sizeof(var_4)) == UC_MAR_DEBUG_INFO_NO_USE);
			
			*(var_4*)(buffer + sizeof(var_4)) = UC_MAR_DEBUG_INFO_USE;
			
			buffer += sizeof(var_4) + sizeof(var_4);
#endif
			
			m_recycle_library = *(var_1**)buffer;
			buffer += sizeof(var_1*);
						
			m_block_count_use++;
			
			m_mem_locker.unlock();
			return (var_vd*)buffer;
		}
		
		if(m_cur_seg_use >= m_block_numPs || m_cur_seg_pos == NULL)
		{
			var_1* mem_buf = NULL;
			
			if(m_segment_library_idle == NULL)
			{
				mem_buf = new(std::nothrow) var_1[m_block_numPs * m_block_size + sizeof(var_vd*)];
				if(mem_buf == NULL)
				{
					m_mem_locker.unlock();
					return NULL;
				}
			}
			else
			{
				mem_buf = m_segment_library_idle;
				m_segment_library_idle = *(var_1**)m_segment_library_idle;
			}
			
			*(var_1**)mem_buf = m_segment_library;
			m_segment_library = mem_buf;

			m_cur_seg_pos = m_segment_library + sizeof(var_1*);
			m_cur_seg_use = 0;
		}
		
		var_1* buffer = m_cur_seg_pos;
		
		m_cur_seg_pos += m_block_size;
		m_cur_seg_use += 1;
		
#ifdef UC_MAR_DEBUG_INFO
		*(var_4*)buffer = 0xABABABAB;
		*(var_4*)(buffer + m_block_size - sizeof(var_4)) = 0xBABABABA;
		*(var_4*)(buffer + sizeof(var_4)) = UC_MAR_DEBUG_INFO_USE;
		
		buffer += sizeof(var_4) + sizeof(var_4);
#endif
		
		buffer += sizeof(var_1*);

		m_block_count_use++;
		
		m_mem_locker.unlock();
		return (var_vd*)buffer;
	}
	
	var_vd put_mem(var_vd* memory)
	{
		m_mem_locker.lock();
		
		var_1* buffer = (var_1*)memory - sizeof(var_1*);
		
#ifdef UC_MAR_DEBUG_INFO
                buffer -= sizeof(var_4) + sizeof(var_4);
                
		assert(*(var_4*)buffer == 0xABABABAB);
		assert(*(var_4*)(buffer + m_block_size - sizeof(var_4)) == 0xBABABABA);
		assert(*(var_4*)(buffer + sizeof(var_4)) == UC_MAR_DEBUG_INFO_USE);		
		
		*(var_4*)(buffer + sizeof(var_4)) = UC_MAR_DEBUG_INFO_NO_USE;
		
		buffer += sizeof(var_4) + sizeof(var_4);
#endif
		
		*(var_1**)buffer = m_recycle_library;
		m_recycle_library = buffer;
				
		m_block_count_use--;
		
		m_mem_locker.unlock();
	}

	void reset_mem(var_4 is_free_memory = 0)
	{
		for(;;)
		{
			m_mem_locker.lock();
			if(m_block_count_use == 0)
				break;
			m_mem_locker.unlock();
			
			cp_sleep(1);
		}

		if(is_free_memory == 1)
		{
			while(m_segment_library_idle)
			{
				var_1* mem_buf = m_segment_library_idle;
				m_segment_library_idle = *(var_1**)m_segment_library_idle;
				
				delete[] mem_buf;
			}
			
			while(m_segment_library)
			{
				var_1* mem_buf = m_segment_library;
				m_segment_library = *(var_1**)m_segment_library;
				
				delete[] mem_buf;
			}
		}
		else
		{
			while(m_segment_library)
			{
				var_1* mem_buf = m_segment_library;
				m_segment_library = *(var_1**)m_segment_library;
				
				*(var_1**)mem_buf = m_segment_library_idle;
				m_segment_library_idle = mem_buf;
			}
		}
		
		m_recycle_library = NULL;
		
		m_cur_seg_pos = NULL;
		m_cur_seg_use = 0;
		
		m_mem_locker.unlock();
	};

	
	var_8 use_mem_count()
	{
		return m_block_count_use;
	}

	// 得到当前版本号
	const var_1* version()
	{
		// v1.000 - 2013.07.24 - 初始版本
		return "v1.000";
	}

private:
	var_8 m_block_size;
	var_8 m_block_numPs;
	
	var_8 m_block_count_use;
	var_8 m_block_count_max;
	
	var_1* m_recycle_library;
	var_1* m_segment_library;
	var_1* m_segment_library_idle;
	
	var_1* m_cur_seg_pos;
	var_8  m_cur_seg_use;
	
	CP_MUTEXLOCK m_mem_locker;
    
    var_4 m_init_finish;
};

#endif // __UC_MEM_ALLOCATOR_RECYCLE_H__
