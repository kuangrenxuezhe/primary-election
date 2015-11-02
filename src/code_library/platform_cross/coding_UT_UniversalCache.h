// UT_UniversalCache.h

#ifndef _UT_UNIVERSAL_CACHE_H_
#define _UT_UNIVERSAL_CACHE_H_

#include <direct.h>

#include <windows.h>

#define CACHE_SAVE		"cache_save.dat"
#define CACHE_TEMP		"cache_temp.dat"

#define CREATEFILENAME(x, y, z) (sprintf(x, "%s//cache_file.%.3d", y, z))

template <class T_Key>
class UT_UniversalCache
{
public:
	// 初始化Cache
	long InitCache(long lDataLen, char* szpPath, long lFileNum, long lFileSize = (2<<30))
	{
		if(lFileSize > (2<<30))
			return -1;
		if(access(szpPath, 1) == 0)
		{
			if(mkdir(szpPath) == 1)
				return -1;
		}

		
	}

	long 

	// 得到当前版本号
	char* GetVersion()
	{
		// 初始版本 - v1.000 - 2008.08.26
		return "v1.000";
	}

private:
	long m_lMaxCacheNum;

	long m_lDataLen;
	long m_lFileNum;
	long m_lFileSize;
	char m_szaPath[256];
}

#endif // _UT_UNIVERSAL_CACHE_H_