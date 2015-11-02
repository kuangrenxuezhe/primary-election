/*! @file
********************************************************************************
<PRE>
ģ����       : <IndividuationDown>
�ļ���       : <StringConvert.h>
����ļ�     : 
�ļ�ʵ�ֹ��� : <> 
����         : <����ǿ>
�汾         : <1.00.00>
--------------------------------------------------------------------------------
��ע         : < A class that simplifies thread synchronization
				 with a mutex.
				The constructor accepts a Mutex and locks it.
				The destructor unlocks the mutex.>
--------------------------------------------------------------------------------
�޸ļ�¼ : 
�� ��        �汾     �޸���              �޸�����
YYYY/MM/DD   X.Y      <���߻��޸�����>    <�޸�����>
</PRE>
*******************************************************************************/
#ifndef _STRINGCONVERT_H
#define _STRINGCONVERT_H

#if defined(_WIN32)
#include <windows.h>
#endif
#include <string>

using namespace std;

class StringConvert  
{
public:
	StringConvert();
	virtual ~StringConvert();

	int UTF_8ToGB2312(const char* pBuffer, int len, char *pResult);

private:
	void UTF_8ToUnicode(const char* pBuffer,wchar_t *pResult);

	void UnicodeToGB2312(char* pOut,unsigned short uData);
};

inline void StringConvert::UTF_8ToUnicode(const char* pBuffer,wchar_t *pResult)
{
	char* uchar = (char *)pResult;
	
	uchar[1] = ((pBuffer[0] & 0x0F) << 4) + ((pBuffer[1] >> 2) & 0x0F);
	uchar[0] = ((pBuffer[1] & 0x03) << 6) + (pBuffer[2] & 0x3F);
}

inline void StringConvert::UnicodeToGB2312(char* pOut,unsigned short uData)
{
#if defined(_WIN32)
	WideCharToMultiByte(CP_ACP,NULL,&uData,1,pOut,sizeof(wchar_t),NULL,NULL);
#endif
}
#endif 