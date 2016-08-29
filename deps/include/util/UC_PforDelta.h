#ifndef __PFORDELTA_H__
#define __PFORDELTA_H__

#include "UC_VarByte.h"

template<typename T>
var_4 HIGH_BIT(T number) 
{
	var_4 idx = 0;
	while (number > 0) 
	{
		++ idx;
		number = number >> 1;
	}
	if (idx == 0) 
		idx = 1;
	return idx;
};

const var_u4 BIT32_MASK[] = {
	0,
	0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f, 0xff, 
	0x1ff, 0x3ff, 0x7ff, 0xfff, 0x1fff, 0x3fff, 0x7fff, 0xffff, 
	0x1ffff, 0x3ffff, 0x7ffff, 0xfffff, 0x1fffff, 0x3fffff, 0x7fffff, 0xffffff, 
	0x1ffffff, 0x3ffffff, 0x7ffffff, 0xfffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff,
};

class UC_PforDelta
{
public:
	static var_4 encode32_out(var_u4* in_src, var_4 in_num, var_u1* out_des)
	{
		if(in_num <= 0)
			return 0;

		var_u4* ptr_src = in_src;
		var_u1* ptr_des = out_des;

		if(1 == in_num)
		{
			UC_VarByte::encode32_ptr(ptr_src[0], ptr_des);
			return (var_4)(ptr_des - out_des);
		}
		
		// 1) 统计数据的位数范围
		var_4 num_bit_array[33] = {0};  
		for(var_4 i = 0; i < in_num; i++) 
		{
			num_bit_array[HIGH_BIT(*ptr_src)]++;
			ptr_src++;
		}
		// 2) 找到数据的bit数
		var_4 accumulator = 0;
		var_4 bit_of_num = 0;
		var_4 partLength = (var_4)(in_num * 0.9);
		for (var_4 i=0; i <= 32; i++) 
		{
			accumulator += num_bit_array[i];
			// 找到的90%数据的位数
			if (accumulator >= partLength) 
			{
				bit_of_num = i;
				break;
			}
		}
		// 3) 构造控制头
		*(var_u1*)ptr_des++ = (var_u1)bit_of_num;		
		UC_VarByte::encode32_ptr(in_num - accumulator, ptr_des);
		// 4) 确定数据块的长度. 整理成整数型，否则可能出现异常数据和块数据重叠的可能
		var_4 size_of_data_block = ((in_num * bit_of_num + 32 - 1) / 32) * 4;
		// 5) 写数据
		var_u1* ptr_exception_des = ptr_des + size_of_data_block;
		var_u4* ptr_block_des = (var_u4*) ptr_des;
		memset(ptr_block_des, 0, size_of_data_block);
		var_u4 max_num = (1 << bit_of_num) - 1;
		var_u4 exception_pos = 0;
		var_u4 bit = 0;
		ptr_src = in_src;
		ptr_block_des[0] = 0;
		for(var_4 i = 0, j = 0; i < in_num; ++i)
		{ 
			if(ptr_src[i] > max_num) 
			{
				// 异常数据
				// 5.1) 写异常位置
				UC_VarByte::encode32_ptr(i - exception_pos, ptr_exception_des);
				// 5.2) 写异常数据
				UC_VarByte::encode32_ptr(ptr_src[i] >> bit_of_num, ptr_exception_des);
				exception_pos = i;
			}
			// 5.3) 写块数据
			ptr_block_des[j] |= ((ptr_src[i] & BIT32_MASK[bit_of_num]) << bit);
			bit += bit_of_num; 
			if(bit >= 32)
			{
				j++;
				bit -= 32;
				if (bit > 0)
					ptr_block_des[j] |= ((ptr_src[i] & BIT32_MASK[bit_of_num]) >> (bit_of_num - bit));
			}
		}    
		return (var_4)(ptr_exception_des - out_des);
	};

	static var_4 encode32_in(var_u4* in_src, var_4 in_num, var_u1* out_des)
	{
		var_u4* ptr_src = in_src;
		var_u1* ptr_des = out_des;

		UC_VarByte::encode32_ptr(in_num, ptr_des);

		if(in_num <= 0)
			return (var_4)(ptr_des - out_des);

		if(1 == in_num)
		{
			UC_VarByte::encode32_ptr(ptr_src[0], ptr_des);
			return (var_4)(ptr_des - out_des);
		}

		// 1) 统计数据的位数范围
		var_4 num_bit_array[33] = {0};  
		for(var_4 i = 0; i < in_num; i++) 
		{
			num_bit_array[HIGH_BIT(*ptr_src)]++;
			ptr_src++;
		}
		// 2) 找到数据的bit数
		var_4 accumulator = 0;
		var_4 bit_of_num = 0;
		var_4 partLength = (var_4)(in_num * 0.9);
		for (var_4 i=0; i <= 32; i++) 
		{
			accumulator += num_bit_array[i];
			// 找到的90%数据的位数
			if (accumulator >= partLength) 
			{
				bit_of_num = i;
				break;
			}
		}
		// 3) 构造控制头
		*(var_u1*)ptr_des++ = (var_u1)bit_of_num;		
		UC_VarByte::encode32_ptr(in_num - accumulator, ptr_des);
		// 4) 确定数据块的长度. 整理成整数型，否则可能出现异常数据和块数据重叠的可能
		var_4 size_of_data_block = ((in_num * bit_of_num + 32 - 1) / 32) * 4;
		// 5) 写数据
		var_u1* ptr_exception_des = ptr_des + size_of_data_block;
		var_u4* ptr_block_des = (var_u4*) ptr_des;
		memset(ptr_block_des, 0, size_of_data_block);
		var_u4 max_num = (1 << bit_of_num) - 1;
		var_u4 exception_pos = 0;
		var_u4 bit = 0;
		ptr_src = in_src;
		ptr_block_des[0] = 0;
		for(var_4 i = 0, j = 0; i < in_num; ++i)
		{ 
			if(ptr_src[i] > max_num) 
			{
				// 异常数据
				// 5.1) 写异常位置
				UC_VarByte::encode32_ptr(i - exception_pos, ptr_exception_des);
				// 5.2) 写异常数据
				UC_VarByte::encode32_ptr(ptr_src[i] >> bit_of_num, ptr_exception_des);
				exception_pos = i;
			}
			// 5.3) 写块数据
			ptr_block_des[j] |= ((ptr_src[i] & BIT32_MASK[bit_of_num]) << bit);
			bit += bit_of_num; 
			if(bit >= 32)
			{
				j++;
				bit -= 32;
				if (bit > 0)
					ptr_block_des[j] |= ((ptr_src[i] & BIT32_MASK[bit_of_num]) >> (bit_of_num - bit));
			}
		}    
		return (var_4)(ptr_exception_des - out_des);
	};

    static var_4 decode32_out(var_u1* in_src, var_u4* out_des, var_4 in_num)
	{
		if(in_num <= 0)
			return 0;

		var_u1* ptr_src = in_src;
		var_u4* ptr_des = out_des;

		if (1 == in_num)
		{
			ptr_des[0] = UC_VarByte::decode32_ptr(ptr_src);
			return (var_4)(ptr_src - in_src);
		};

		// 1) 读控制头
		var_4 bit_of_num = *(var_u1*) ptr_src;
		++ ptr_src; 
		var_4 exception_num = UC_VarByte::decode32_ptr(ptr_src);
		// 2) 定位异常数据位置
		var_4 size_of_data_block = ((in_num * bit_of_num + 32 - 1) / 32) * 4;
		var_u1* ptr_exception = ptr_src + size_of_data_block;
		// 3) 读块数据
		var_u4* ptr_block = (var_u4*) ptr_src;
		var_4 bit = 0;
		for(var_4 i = 0, j = 0; i < in_num; i++)
		{
			// 初始化
			ptr_des[i] = 0;
			//  3.1) 读数据
			ptr_des[i] = ((ptr_block[j] >> bit) & BIT32_MASK[bit_of_num]);
			bit += bit_of_num;
			if(bit >= 32)
			{
				j++;
				bit -= 32;
				//  3.2) 补齐数据
				if(bit > 0)
					ptr_des[i] |= (ptr_block[j] & BIT32_MASK[bit]) << (bit_of_num - bit);
			} 
		}
		// 4) 读异常数据
		var_u4 exception_pos = 0;
		var_u4 exception_value = 0;
		for(var_4 i = 0; i < exception_num; i++)
		{
			exception_pos += UC_VarByte::decode32_ptr(ptr_exception);
			exception_value = UC_VarByte::decode32_ptr(ptr_exception);
			ptr_des[exception_pos] = ptr_des[exception_pos] | (exception_value << bit_of_num);
		}
		return (var_4)(ptr_exception - in_src);
	};

	static var_4 decode32_in(var_u1* in_src, var_u4* out_des, var_4& out_num)
	{
		var_u1* ptr_src = in_src;
		var_u4* ptr_des = out_des;		

		out_num = UC_VarByte::decode32_ptr(ptr_src);

		if(out_num <= 0)
			return (var_4)(ptr_src - in_src);

		if(1 == out_num)
		{
			ptr_des[0] = UC_VarByte::decode32_ptr(ptr_src);
			return (var_4)(ptr_src - in_src);
		};
		
		// 1) 读控制头
		var_4 bit_of_num = *(var_u1*) ptr_src;
		++ ptr_src; 
		var_4 exception_num = UC_VarByte::decode32_ptr(ptr_src);
		// 2) 定位异常数据位置
		var_4 size_of_data_block = ((out_num * bit_of_num + 32 - 1) / 32) * 4;
		var_u1* ptr_exception = ptr_src + size_of_data_block;
		// 3) 读块数据
		var_u4* ptr_block = (var_u4*) ptr_src;
		var_4 bit = 0;
		for(var_4 i = 0, j = 0; i < out_num; i++)
		{
			// 初始化
			ptr_des[i] = 0;
			//  3.1) 读数据
			ptr_des[i] = ((ptr_block[j] >> bit) & BIT32_MASK[bit_of_num]);
			bit += bit_of_num;
			if(bit >= 32)
			{
				j++;
				bit -= 32;
				//  3.2) 补齐数据
				if(bit > 0)
					ptr_des[i] |= (ptr_block[j] & BIT32_MASK[bit]) << (bit_of_num - bit);
			} 
		}
		// 4) 读异常数据
		var_u4 exception_pos = 0;
		var_u4 exception_value = 0;
		for(var_4 i = 0; i < exception_num; i++)
		{
			exception_pos += UC_VarByte::decode32_ptr(ptr_exception);
			exception_value = UC_VarByte::decode32_ptr(ptr_exception);
			ptr_des[exception_pos] = ptr_des[exception_pos] | (exception_value << bit_of_num);
		}
		return (var_4)(ptr_exception - in_src);
	};
};

#endif // __PFORDELTA_H__
