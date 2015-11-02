/*! @file
********************************************************************************
<PRE>
模块名       : <IndividuationDown>
文件名       : <TemplateQueue.h>
相关文件     : <>
文件实现功能 : <所有接口的引用文件> 
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
