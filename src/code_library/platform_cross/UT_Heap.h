// UT_Heap.h

#ifndef _UT_HEAP_H_
#define _UT_HEAP_H_

#include "UH_Define.h"

template <typename T_Type>
class UT_Heap
{
public:
	UT_Heap()
	{
		max_size = 0;
		cur_size = 0;

		heap_buf =  NULL;
	};

	~UT_Heap()
	{
		if(heap_buf)
			delete heap_buf;
	};

	var_4 init_heap(int max_heap_size)
	{
		max_size = max_heap_size;
		cur_size = 0;

		heap_buf = new T_Type[max_size + 1];
		if(heap_buf == NULL)
			return -1;

		return 0;
	};

	var_4 push_data(T_Type key)
	{
		if(cur_size >= max_size)
			return -1;

		heap_buf[++cur_size] = key;

		var_4 cur_pos = cur_size;
		while(cur_pos > 1)
		{
			if(heap_buf[cur_pos>>1] > heap_buf[cur_pos])
			{
				T_Type tmp = heap_buf[cur_pos]; 
				heap_buf[cur_pos] = heap_buf[cur_pos>>1]; 
				heap_buf[cur_pos>>1] = tmp;
				cur_pos >>= 1;
			}
			else 
				break;
		}
	};

	T_Type pop_data()
	{
		T_Type tmp = heap_buf[1]; 
		heap_buf[1] = heap_buf[cur_size];
		heap_buf[cur_size--] = tmp; 
		
		adjust(1);
		
		return heap_buf[cur_size + 1];
	};

private:
	void adjust(var_4 cur_pos)
	{
		while(cur_pos<<1 <= cur_size)
		{
			var_4 tmp_pos = cur_pos<<1; 
			if((cur_pos<<1) + 1 <= size && heap_buf[(cur_pos<<1) + 1] < heap_buf[tmp_pos])
				tmp_pos = (cur_pos<<1) + 1;

			if(heap_buf[tmp_pos] < heap_buf[cur_pos])
			{
				T_Type tmp = heap_buf[tmp_pos]; 
				heap_buf[tmp_pos] = heap_buf[cur_pos]; 
				heap_buf[cur_pos] = tmp;
				cur_pos = tmp_pos; 
			}
			else 
				break;
		}
	};

private:
	var_4 max_size;
	var_4 cur_size;
	
	T_Type* heap_buf;
};

#endif // _UT_HEAP_H_
