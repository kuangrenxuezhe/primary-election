//
//  UT_LeastHeap.h
//  code_library
//
//  Created by zhanghl on 14-9-30.
//  Copyright (c) 2014年 zhanghl. All rights reserved.
//

#ifndef _UT_LEASTHEAP_H_
#define _UT_LEASTHEAP_H_

template <class T_Key>
class UT_LeastHeap
{
public:
	UT_LeastHeap()
	{
		m_heap_list = NULL;
	}
	~UT_LeastHeap()
	{
		if(m_heap_list)
			delete m_heap_list;
	}
	
	var_4 init(var_4 heap_size)
	{
		m_cur_size = 1;
		m_max_size = heap_size + 1;
		
		m_heap_list = new(std::nothrow) T_Key[heap_size + 1];
		if(m_heap_list == NULL)
			return -1;
		
		return 0;
	}
	
	T_Key top()
	{
		return m_heap_list[1];
	}
	
	var_4 num()
	{
		return m_cur_size - 1;
	}
	
	T_Key* val()
	{
		return m_heap_list + 1;
	}
	
	var_4 add(T_Key key)
	{
		if(m_cur_size >= m_max_size)
			return -1;
		
		m_heap_list[m_cur_size] = key;
		
		var_4 i = m_cur_size;
		var_4 j = i / 2;
		T_Key t = m_heap_list[i];
		
		while(j > 0)
		{
			if(m_heap_list[j] <= t)
				break;
			else
			{
				m_heap_list[i] = m_heap_list[j];
				i = j;
				j = j / 2;
			}
		}
		
		m_heap_list[i] = t;
		
		m_cur_size++;
		
		return 0;
	}
	
	var_vd del()
	{
		m_heap_list[1] = m_heap_list[m_cur_size - 1];
	
		m_cur_size--;
		
		adjust();
	}
	
private:
	var_vd adjust()
	{
		var_4 i = 1;
		var_4 j = i * 2;
		
		T_Key t = m_heap_list[i];
		
		while(j < m_cur_size)
		{
			if(j + 1 < m_cur_size && m_heap_list[j] > m_heap_list[j + 1])
				j++;
			
			if(m_heap_list[j] >= t)
				break;
			else
			{
				m_heap_list[i] = m_heap_list[j];
				i = j;
				j = j * 2;
			}
		}
		
		m_heap_list[i] = t;
	}

private:
	var_4  m_max_size;
	var_4  m_cur_size;
	
	T_Key* m_heap_list;
};

#endif // _UT_LEASTHEAP_H_
