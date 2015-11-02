// UC_CodePageConversion.h

#ifndef __UC_CODE_PAGE_CONVERSION__
#define __UC_CODE_PAGE_CONVERSION__

#include "UH_Define.h"

class UC_CodePageConversion
{	
public:

#ifdef _WIN32_ENV_
	#define CP_GB2312 20936
/*		
	var_4 UTF_8ToGB2312(var_1* in_buf, var_4 in_len, var_1* out_buf, var_4 out_len, var_1* mid_buf = NULL)
	{
		var_4 release_flg = 1;
		if(tmp_buf)
			release_flg = 0;
		else
		{
			WCHAR* tmp_buf = (WCHAR*)mid_buf;
			if(tmp_buf == NULL)
				return -1;
		}		

		var_4 tmp_len = MultiByteToWideChar(CP_UTF8, 0, in_buf, in_len, tmp_buf, in_len);  
		if(tmp_len == 0)
		{
			delete tmp_buf;
			return -1;
		}

		var_4 ret_len = WideCharToMultiByte(CP_GB2312, 0, tmp_buf, tmp_len, out_buf, out_len, NULL, NULL);
		if(ret_len == 0)
		{
			delete tmp_buf;
			return -1;
		}

		if(release_flg)
			delete tmp_buf;

		return ret_len;
	}

	var_4 GB2312ToUTF_8(var_1* in_buf, var_4 in_len, var_1* out_buf, var_4 out_len, var_1* mid_buf = NULL)
	{
		var_4 release_flg = 1;
		if(tmp_buf)
			release_flg = 0;
		else
		{
			WCHAR* tmp_buf = (WCHAR*)mid_buf;
			if(tmp_buf == NULL)
				return -1;
		}
				
		var_4 tmp_len = MultiByteToWideChar(CP_GB2312, MB_PRECOMPOSED, in_buf, in_len, tmp_buf, in_len);
		if(tmp_len == 0)
		{
			delete tmp_buf;
			return -1;
		}

		var_4 ret_len = WideCharToMultiByte(CP_UTF8, 0, tmp_buf, tmp_len, out_buf, out_len, NULL, NULL);
		if(ret_len == 0)
		{
			delete tmp_buf;
			return -1;
		}

		if(release_flg)
			delete tmp_buf;

		return ret_len;
	}
*/
	void UTF_8ToUnicode(var_1* in_buf, var_1* out_buf)
	{
		out_buf[1] = ((in_buf[0] & 0x0F) << 4) + ((in_buf[1] >> 2) & 0x0F);
		out_buf[0] = ((in_buf[1] & 0x03) << 6) + (in_buf[2] & 0x3F);
	}

	void UnicodeToUTF_8(var_1* in_string, var_1* out_string)
	{
		out_string[0] = (0xE0 | ((in_string[1] & 0xF0) >> 4));
		out_string[1] = (0x80 | ((in_string[1] & 0x0F) << 2)) + ((in_string[0] & 0xC0) >> 6);
		out_string[2] = (0x80 | (in_string[0] & 0x3F));
	}

	void UnicodeToGB2312(WCHAR* in_string, char* out_string)
	{
		WideCharToMultiByte(CP_ACP, NULL, in_string, 1, out_string, sizeof(WCHAR), NULL,NULL);
	}

	void GB2312ToUnicode(char* in_string, WCHAR* out_string)
	{
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, in_string, 2, out_string, 1);
	}
		
	int GB2312ToUTF_8(char* in_string, long in_strlen, char* out_string)
	{
		char  tmpbuf[32];

		char* begpos = in_string;
		char* endpos = in_string + in_strlen;
		char* despos = out_string;

		while(begpos < endpos)
		{
			if(*begpos > 0)
				*despos++ = *begpos++;
			else
			{				
				GB2312ToUnicode(begpos, (WCHAR*)tmpbuf);
				UnicodeToUTF_8(tmpbuf, despos);
				
				begpos += 2;
				despos += 3;
			}
		}
		
		return despos - out_string;
	}
	
	var_4 UTF_8ToGB2312(char* in_string, var_4 in_strlen, char* out_string, var_4 out_strlen)
	{
		char  tmpbuf[32];

		char* begpos = in_string;
		char* endpos = in_string + in_strlen;
		char* despos = out_string;

		while(begpos < endpos)
		{
			if(*begpos > 0)
				*despos++ = *begpos++;
			else                 
			{
				UTF_8ToUnicode(begpos, tmpbuf);
				UnicodeToGB2312((WCHAR*)tmpbuf, despos);

				begpos += 3;
				despos += 2;
			}
		}
		
		return despos - out_string;
	}
#else
private:
	var_4 code_page_convert(const var_1* src_charset, const var_1* des_charset, var_1* in_buf, size_t in_len, var_1* out_buf, size_t out_len)
	{
		iconv_t ivt = iconv_open(des_charset, src_charset); 
		if(ivt == (iconv_t)(-1))
			return -1;

		var_4 out_buf_len = (var_4)out_len;

		if(iconv(ivt, (var_1**)&in_buf, (size_t*)&in_len, (var_1**)&out_buf, (size_t*)&out_len) == -1)
        {
            iconv_close(ivt);
            return -1;
        }
			
		iconv_close(ivt);

		if(in_len != 0)
			return -1;
		
		return out_buf_len - (var_4)out_len;
	} 
	
public:
	var_4 UnicodeToUTF_8(var_1* in_buf, size_t in_len, var_1* out_buf, size_t out_len)
	{
		return code_page_convert("UNICODE", "UTF_8", in_buf, in_len, out_buf, out_len);
	}
	var_4 UTF_8ToUnicode(var_1* in_buf, size_t in_len, var_1* out_buf, size_t out_len)
	{
		return code_page_convert("UTF_8", "UNICODE", in_buf, in_len, out_buf, out_len);
	}

	var_4 UnicodeToGB2312(var_1* in_buf, size_t in_len, var_1* out_buf, size_t out_len)
	{
		return code_page_convert("UNICODE", "GB2312", in_buf, in_len, out_buf, out_len);
	}
	var_4 GB2312ToUnicode(var_1* in_buf, size_t in_len, var_1* out_buf, size_t out_len)
	{
		return code_page_convert("GB2312", "UNICODE", in_buf, in_len, out_buf, out_len);
	}

	var_4 UTF_8ToGB2312(var_1* in_buf, size_t in_len, var_1* out_buf, size_t out_len)
	{
		return code_page_convert("UTF-8", "GB2312", in_buf, in_len, out_buf, out_len);
	}
	var_4 GB2312ToUTF_8(var_1* in_buf, size_t in_len, var_1* out_buf, size_t out_len)
	{
		return code_page_convert("GB2312", "UTF-8", in_buf, in_len, out_buf, out_len);
	}

	var_4 UTF_8ToGBK(var_1* in_buf, size_t in_len, var_1* out_buf, size_t out_len)
	{
		return code_page_convert("UTF-8", "GBK", in_buf, in_len, out_buf, out_len);
	}
	var_4 GBKToUTF_8(var_1* in_buf, size_t in_len, var_1* out_buf, size_t out_len)
	{
		return code_page_convert("GBK", "UTF-8", in_buf, in_len, out_buf, out_len);
	}

	var_4 UTF_8ToGB18030(var_1* in_buf, size_t in_len, var_1* out_buf, size_t out_len)
	{
		return code_page_convert("UTF-8", "GB18030", in_buf, in_len, out_buf, out_len);
	}
	var_4 GB18030ToUTF_8(var_1* in_buf, size_t in_len, var_1* out_buf, size_t out_len)
	{
		return code_page_convert("GB18030", "UTF-8", in_buf, in_len, out_buf, out_len);
	}
	
	var_4 UnicodeToGBK(var_1* in_buf, size_t in_len, var_1* out_buf, size_t out_len)
	{
		return code_page_convert("UNICODE", "GBK", in_buf, in_len, out_buf, out_len);
	}
	var_4 GBKToUnicode(var_1* in_buf, size_t in_len, var_1* out_buf, size_t out_len)
	{
		return code_page_convert("GBK", "UNICODE", in_buf, in_len, out_buf, out_len);
	}
#endif

};

#endif // __UC_CODE_PAGE_CONVERSION__

/*
	long MByteToWChar(char* in_string, char* out_string, long out_strlen)
	{
		DWORD dwMinSize = MultiByteToWideChar(CP_OEMCP, 0, in_string, -1, NULL, 0);
		if((u_long)out_strlen < dwMinSize)
			return -1;		
		
		MultiByteToWideChar(CP_OEMCP, 0, in_string, -1, (WCHAR*)out_string, out_strlen);  
		
		return 0;
	}

	long WCharToMByte(char* in_string, char* out_string, long out_strlen)
	{
		DWORD dwMinSize = WideCharToMultiByte(CP_OEMCP, NULL, (WCHAR*)in_string, -1, NULL, 0, NULL, FALSE);
		if((u_long)out_strlen < dwMinSize)
			return -1;

		WideCharToMultiByte(CP_OEMCP, NULL, (WCHAR*)in_string, -1, out_string, out_strlen,NULL,FALSE);
		
		return 0;
	}
*/
