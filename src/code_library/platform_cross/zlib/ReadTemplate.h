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