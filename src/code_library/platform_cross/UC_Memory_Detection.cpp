//
//  UC_Memory_Detection.cpp
//  code_library
//
//  Created by zhanghl on 13-6-19.
//  Copyright (c) 2013年 zhanghl. All rights reserved.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

static FILE* cl_ucmd_out_handle = NULL;
static pthread_mutex_t cl_ucmd_out_mutex = PTHREAD_MUTEX_INITIALIZER;

inline void* operator new(size_t size, char* file, int line)
{
	pthread_mutex_lock(&cl_ucmd_out_mutex);
	
	if(cl_ucmd_out_handle == NULL)
		cl_ucmd_out_handle = fopen("cl_ucmd_out_file.txt", "wb");
	
	fprintf(cl_ucmd_out_handle, "%s:%d:%zd\n", file, line, size);
	fflush(cl_ucmd_out_handle);
	
	printf("%s:%d:%zd\n", file, line, size);
	
	pthread_mutex_unlock(&cl_ucmd_out_mutex);
	
	return ::operator new(size);
}

inline void* operator new[](size_t size, char* file, int line)
{
	pthread_mutex_lock(&cl_ucmd_out_mutex);
	
	if(cl_ucmd_out_handle == NULL)
		cl_ucmd_out_handle = fopen("cl_ucmd_out_file.txt", "wb");
		
		
	fprintf(cl_ucmd_out_handle, "%s:%d:%zd\n", file, line, size);
	fflush(cl_ucmd_out_handle);
		
	printf("%s:%d:%zd\n", file, line, size);
		
	pthread_mutex_unlock(&cl_ucmd_out_mutex);
	
	return ::operator new[](size);
}

/*
inline void* operator new(size_t size, void* ptr, char* file, int line)
{
	return ::operator new(size, ptr);
}

inline void* operator new[](size_t size, void* ptr, char* file, int line)
{
	return ::operator new[](size, ptr);
}
*/

/*
inline void operator delete(void* __p)
{
	::operator delete(__p);
}

inline void operator delete[](void* __p)
{
	::operator delete[](__p);
}
*/
