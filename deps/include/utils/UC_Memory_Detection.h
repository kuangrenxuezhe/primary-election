//
//  UC_Memory_Detection.h
//  code_library
//
//  Created by zhanghl on 13-6-19.
//  Copyright (c) 2013年 zhanghl. All rights reserved.
//

#ifndef __UC_MEMORY_DETECTION_H__
#define __UC_MEMORY_DETECTION_H__

void* operator new(size_t size, char* file, int line);
void* operator new[](size_t size, char* file, int line);

#define new new(__FILE__, __LINE__)

/*
#define new(x) new(x, __FILE__, __LINE__)
*/

#endif // __UC_MEMORY_DETECTION_H__
