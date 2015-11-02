/*! @file
********************************************************************************
<PRE>
模块名       : <IndividuationDown>
文件名       : <LogWatch.h>
相关文件     : 
文件实现功能 : <> 
作者         : <和文强>
版本         : <1.00.00>
--------------------------------------------------------------------------------
备注         : < 如;个性化新闻下载 
				    [task,begin_tim,runtime,checktime,itemnum,item1,state,over] 
				    task=/zhongsou/news/IndividuationDown/NewsDown 
					;任务名称是NewsDown 
					begin_tim=2006-11-30 9:43
					;任务开始时间：2006-11-30 9:43 
					runtime=10
					;完成任务所需时间（估计值）单位为分钟 
					checktime=30
					;在经过checktime指定的时间后，检测计划任务的启动是否完成 
					itemnum=1 
					;有八项任务 
					item1=摸板过期否 
					;开始检查 
					state=正常
					over=1 
					;======================== >
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
YYYY/MM/DD   X.Y      <作者或修改者名>    <修改内容>
</PRE>
*******************************************************************************/

#ifndef _LOGWATCH_H_2007_01_08
#define _LOGWATCH_H_2007_01_08

#include <stdio.h>
#include <string.h>

class LogWatch  
{
public:
	LogWatch();

	virtual ~LogWatch();

	static int logBegin(int itemNum = 1);

	static int writeItem(const char* msg, int cnt = 1);

	static int logOver();

private:
	static char _fileName[512];
};

#endif 
