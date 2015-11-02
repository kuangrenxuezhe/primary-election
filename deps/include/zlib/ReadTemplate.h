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
#ifndef _READTEMPLATE_H_2006_08_29
#define _READTEMPLATE_H_2006_08_29

#include <iostream>
#include <string>
#include <stdio.h>
#include "TemplateQueue.h"


using namespace std;

class ReadTemplate  
{
public:
	ReadTemplate(TemplateQueue& queue);
	
	virtual ~ReadTemplate();
	
	bool readListFile(const char* pFileName);

private:
	int openFile(const char* pFileName);
	
	int readToQueue();
	
	void closeFile();

private:
	TemplateQueue& _queue; 
	FILE*          _pFile;
};

#endif 