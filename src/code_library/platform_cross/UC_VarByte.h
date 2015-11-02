#ifndef __VARBYTE_H__
#define __VARBYTE_H__

#include "UH_Define.h"

class UC_VarByte
{
public:
	// compress 32bits
	static inline var_u1* encode32_ptr(var_u4 v, var_u1*& target)
	{
		while((v & ~0x7F) != 0) 
		{
			*(var_u1*)(target++) = ((var_u1)((v & 0x7F) | 0x80));
			v >>= 7; 
		}
		*(var_u1*)(target++) = (var_u1)v;

		return target;
	};
	static inline var_4 encode32_len(var_u4 v, var_u1* target)
	{
		var_u1* t = target;

		while ((v & ~0x7F) != 0) 
		{
			*(var_u1*)(t++) = ((var_u1)((v & 0x7F) | 0x80));
			v >>= 7; 
		}
		*(var_u1*)(t++) = (var_u1)v;

		return (var_4)(t - target);
	}; 
	static inline var_4 encode32(var_u4* src, var_4 num, var_u1* des)
	{
		var_u1* t = des;

		for(var_4 i = 0; i < num; i++)
		{
			var_u4 v = src[i];
			while((v & ~0x7F) != 0) 
			{
				*(var_u1*)(t++) = ((var_u1)((v & 0x7F) | 0x80));
				v >>= 7; 
			}
			*(var_u1*)(t++) = (var_u1)v;
		}

		return (var_4)(t - des);
	};
	// compress 64bits
	static inline var_u1* encode64_ptr(var_u8 v, var_u1*& target)
	{
		while ((v & ~0x7F) != 0) 
		{
			*(var_u1*)(target++) = ((var_u1)((v & 0x7F) | 0x80));
			v >>= 7; 
		}
		*(var_u1*)(target++) = (var_u1)v;

		return target;
	};
	static inline var_4 encode64_len(var_u8 v, var_u1* target)
	{
		var_u1* t = target;

		while ((v & ~0x7F) != 0) 
		{
			*(var_u1*)(t++) = ((var_u1)((v & 0x7F) | 0x80));
			v >>= 7; 
		}
		*(var_u1*)(t++) = (var_u1)v;

		return (var_4)(t - target);
	}; 
	static var_4 inline encode64(var_u8* src, var_4 num, var_u1* des)
	{
		var_u1* t = des;

		for(var_4 i = 0; i < num; i++)
		{
			var_u8 v = src[i];
			while((v & ~0x7F) != 0) 
			{
				*(var_u1*)(t++) = ((var_u1)((v & 0x7F) | 0x80));
				v >>= 7; 
			}
			*(var_u1*)(t++) = (var_u1)v;
		}

		return (var_4)(t - des);
	};
	// uncompress 32Bit
	static var_u4 decode32_ptr(var_u1*& data)
	{
		var_u1 b = *(var_u1*)data++;
		var_u4 v = b & 0x7F;
		for (var_4 shift = 7; (b & 0x80) != 0; shift += 7) 
		{
			b = *(var_u1*) (data ++);
			v |= (b & 0x7F) << shift;
		}

		return v;
	};
	static var_4 decode32_len(var_u1* data, var_u4& v)
	{
		var_u1* t = data;

		var_u1 b = *(var_u1*)t++;
		v = b & 0x7F;
		for (var_4 shift = 7; (b & 0x80) != 0; shift += 7) 
		{
			b = *(var_u1*)(t++);
			v |= (b & 0x7F) << shift;
		}

		return (var_4)(t - data);
	};
	static var_4 decode32(var_u1* src, var_4 num, var_u4* des)
	{
		var_u1* t = src;

		for(var_4 i = 0; i < num; i++)
		{
			var_u1 b = *(var_u1*)t++;
			des[i] = b & 0x7F;
			for (var_4 shift = 7; (b & 0x80) != 0; shift += 7) 
			{
				b = *(var_u1*)(t++);
				des[i] |= (b & 0x7F) << shift;
			}
		}

		return (var_4)(t - src);
	};
	// uncompress 64Bit
	static var_u8 decode64_ptr(var_u1*& data)
	{
		var_u1 b = *(var_u1*)data++;
		var_u8 v = b & 0x7F;
		for (var_4 shift = 7; (b & 0x80) != 0; shift += 7) 
		{
			b = *(var_u1*) (data ++);
			v |= (b & 0x7FLL) << shift;
		}

		return v;
	};
	static var_4 decode64_len(var_u1* data, var_u8& v)
	{
		var_u1* t = data;

		var_u1 b = *(var_u1*)t++;
		v = b & 0x7F;
		for (var_4 shift = 7; (b & 0x80) != 0; shift += 7) 
		{
			b = *(var_u1*)(t++);
			v |= (b & 0x7FLL) << shift;
		}

		return (var_4)(t - data);
	};
	static var_4 decode64(var_u1* src, var_4 num, var_u8* des)
	{
		var_u1* t = src;

		for(var_4 i = 0; i < num; i++)
		{
			var_u1 b = *(var_u1*)t++;
			des[i] = b & 0x7F;
			for (var_4 shift = 7; (b & 0x80) != 0; shift += 7) 
			{
				b = *(var_u1*)(t++);
				des[i] |= (b & 0x7FLL) << shift;
			}
		}

		return (var_4)(t - src);
	};
};

#endif // __VARBYTE_H__
