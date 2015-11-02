/*! @file
********************************************************************************
<PRE>
ģ����       : <IndividuationDown>
�ļ���       : <LogWatch.h>
����ļ�     : 
�ļ�ʵ�ֹ��� : <> 
����         : <����ǿ>
�汾         : <1.00.00>
--------------------------------------------------------------------------------
��ע         : < ��;���Ի��������� 
				    [task,begin_tim,runtime,checktime,itemnum,item1,state,over] 
				    task=/zhongsou/news/IndividuationDown/NewsDown 
					;����������NewsDown 
					begin_tim=2006-11-30 9:43
					;����ʼʱ�䣺2006-11-30 9:43 
					runtime=10
					;�����������ʱ�䣨����ֵ����λΪ���� 
					checktime=30
					;�ھ���checktimeָ����ʱ��󣬼��ƻ�����������Ƿ���� 
					itemnum=1 
					;�а������� 
					item1=������ڷ� 
					;��ʼ��� 
					state=����
					over=1 
					;======================== >
--------------------------------------------------------------------------------
�޸ļ�¼ : 
�� ��        �汾     �޸���              �޸�����
YYYY/MM/DD   X.Y      <���߻��޸�����>    <�޸�����>
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
