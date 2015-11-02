/*! @file
********************************************************************************
<PRE>
ģ����       : <IndividuationDown>
�ļ���       : <ReadTemplate.h>
����ļ�     : 
�ļ�ʵ�ֹ��� : <> 
����         : <����ǿ>
�汾         : <1.00.00>
--------------------------------------------------------------------------------
��ע         : <>
--------------------------------------------------------------------------------
�޸ļ�¼ : 
�� ��        �汾     �޸���              �޸�����
YYYY/MM/DD   X.Y      <���߻��޸�����>    <�޸�����>
</PRE>
*******************************************************************************/
#ifndef _READCONFIGFILE_H_2006_08_31
#define _READCONFIGFILE_H_2006_08_31

#include <iostream>
#include <string.h>
#include <stdio.h> 
#include <stdlib.h>
//#include "Exception.h"

using namespace std;

class ReadConfigFile  
{
public:
	ReadConfigFile();
	~ReadConfigFile();
	static int readMessage(const char* pFileName);

	static char* getWebList();

	static char* getSysLog();

	static char* getDataPath();

	static char* getHost();

	static char* getName();

	static char* getPwd();

	static char* getDB();
	
	static int getThreads();

	static int getInterval();

	static int getSaveMode();

private:

	static char* _pWebList;
	static char* _pSysLog;
	static char* _pDataPath;

	static char* _pHost;
	static char* _pName;
	static char* _pPwd;
	static char* _pDB;
	
	static int   _buflen;
	static int   _interval;  //���м��
	static int   _threads;
	static int   _saveMode;  //���淽ʽ
};

#endif 