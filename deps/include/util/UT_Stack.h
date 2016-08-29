// UT_Stack.h

#ifndef _UT_STACK_H_
#define _UT_STACK_H_

#include "UH_Define.h"

template <class T>
class UT_Stack
{
public:
	UT_Stack()
	{
		max_stack_size = -1;
		cur_stack_size = -1;

		tpstack = NULL;
	};

	~UT_Stack()
	{
		if(tpstack)
			delete tpstack;
	};

	var_4 init(var_4 stack_size)
	{
		max_stack_size = stack_size;
		cur_stack_size = 0;
		
		tpstack = new T[max_stack_size];
		if(tpstack == NULL)
			return -1;

		return 0;
	};

	void clear()
	{
		cur_stack_size = 0;
	};

	var_4 number()
	{
		return cur_stack_size;
	};

	var_4 full()
	{
		if(max_stack_size == cur_stack_size)
			return 1;

		return 0;
	};

	var_4 empty()
	{
		if(cur_stack_size == 0)
			return 1;

		return 0;
	};

	var_4 push(T key)
	{
		if(cur_stack_size == max_stack_size)
			return -1;

		tpstack[cur_stack_size++] = key;

		return 0;
	};

	var_4 pop(T& key)
	{
		if(cur_stack_size == 0)
			return -1;
		
		key = tpstack[--cur_stack_size];

		return 0;
	};

	// 得到当前版本号
	const var_1* get_version()
	{
		// v1.000 - 2010.07.02 - 初始版本
		return "v1.000";
	}

private:
	var_4 max_stack_size;
	var_4 cur_stack_size;
	
	T* tpstack;

	CP_MUTEXLOCK lock;	
};

#endif // _UT_STACK_H_
