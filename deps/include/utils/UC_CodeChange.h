// UC_CodeChange.h

#ifndef _UC_CODE_CHANGE_H_
#define _UC_CODE_CHANGE_H_

#include <windows.h>

typedef unsigned long		u_long;
typedef unsigned char		u_char;
typedef unsigned short		u_short;
typedef unsigned __int64	u_int64;

class UC_CodeChange
{
public:
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
	void UTF_8ToUnicode(char* in_string, char* out_string /* WCHAR* */)
	{		
		out_string[1] = ((in_string[0] & 0x0F) << 4) + ((in_string[1] >> 2) & 0x0F);
		out_string[0] = ((in_string[1] & 0x03) << 6) + (in_string[2] & 0x3F);
	}

	void UnicodeToUTF_8(char* in_string /* WCHAR* */, char* out_string)
	{		
		out_string[0] = (0xE0 | ((in_string[1] & 0xF0) >> 4));
		out_string[1] = (0x80 | ((in_string[1] & 0x0F) << 2)) + ((in_string[0] & 0xC0) >> 6);
		out_string[2] = (0x80 | (in_string[0] & 0x3F));
	}

	void UnicodeToGB2312(WCHAR* in_string, char* out_string)
	{
		WideCharToMultiByte(CP_ACP, NULL, in_string, 1, out_string, sizeof(WCHAR), NULL,NULL);
	}

	void Gb2312ToUnicode(char* in_string, WCHAR* out_string)
	{
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, in_string, 2, out_string, 1);
	}
		
	void GB2312ToUTF_8(char* in_string, long in_strlen, char* out_string)
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
				Gb2312ToUnicode(begpos, (WCHAR*)tmpbuf);
				UnicodeToUTF_8(tmpbuf, despos);
				
				begpos += 2;
				despos += 3;
			}
		}
		
		*despos = 0;		
	}
	
	void UTF_8ToGB2312(char* in_string, long in_strlen, char* out_string)
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
		
		*despos = 0;
	}
};

#endif // _UC_CODE_CHANGE_H_
