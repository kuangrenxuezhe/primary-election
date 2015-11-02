/*! @file
********************************************************************************
<PRE>
ģ����       : <IndividuationDown>
�ļ���       : <TemplateQueue.h>
����ļ�     : <>
�ļ�ʵ�ֹ��� : <���нӿڵ������ļ�> 
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
#ifndef _TEMPLATEQUEUE_H_2006_08_29
#define _TEMPLATEQUEUE_H_2006_08_29

#include "Mutex.h"
#include "LinkQueue.h"

typedef LinkQueue TQueue;

class TemplateQueue  
{
public:
	void addUrl(Template*& data);
	
	static Template* getUrl();
	
	static void clear();
	
	static bool isEmpty();

private:
	static TQueue _data;
	static Mutex _mutex;
};

#endif 
