/*! @file
********************************************************************************
<PRE>
模块名       : <IndividuationDown>
文件名       : <ReadTemplate.h>
相关文件     : 
文件实现功能 : <> 
作者         : <和文强>
版本         : <1.00.00>
--------------------------------------------------------------------------------
备注         : <>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
YYYY/MM/DD   X.Y      <作者或修改者名>    <修改内容>
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
	static int   _interval;  //运行间隔
	static int   _threads;
	static int   _saveMode;  //保存方式
};

#endif 