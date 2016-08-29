// UT_Queue.h

#ifndef _UT_QUEUE_H_
#define _UT_QUEUE_H_

#include "UH_Define.h"

template <class T_Key>
class UT_Queue
{
public:
	UT_Queue()
	{
		m_lMaxQueueSize = -1;
		m_lCurQueueSize = -1;

		m_tpQueue = NULL;

		m_lBegPos = -1;
		m_lEndPos = -1;
		
		pthread_mutex_init(&m_lock, NULL);
		pthread_cond_init(&m_cond, NULL);
	};

	~UT_Queue()
	{
		if(m_tpQueue)
			delete m_tpQueue;
		
		pthread_mutex_destroy(&m_lock);
		pthread_cond_destroy(&m_cond);
	};

	var_4 InitQueue(var_4 lMaxQueueSize)
	{
		m_lMaxQueueSize = lMaxQueueSize;
		m_lCurQueueSize = 0;
/*
		__asm__("bsr %0,%%eax; mov %%eax, %1":"=r"(m_lNowQueueSize):"r"(lMaxQueueSize):"%eax");
		m_lNowQueueSize = 1<<m_lNowQueueSize;
		m_lNowQueueSize = lMaxQueueSize > m_lNowQueueSize ? (m_lNowQueueSize<<1) : m_lNowQueueSize;
*/
		m_lNowQueueSize = m_lMaxQueueSize;
		
		m_tpQueue = new T_Key[m_lNowQueueSize];
		if(m_tpQueue == NULL)
			return -1;
		
		m_lBegPos = 0;
		m_lEndPos = 0;

		return 0;
	};

	void ResetQueue()
	{
		m_lCurQueueSize = 0;

		m_lBegPos = 0;
		m_lEndPos = 0;	
	};

	void ClearQueue()
	{
		if(m_tpQueue)
		{
			delete m_tpQueue;
			m_tpQueue = NULL;
		}

		m_lMaxQueueSize = -1;
		m_lCurQueueSize = -1;

		m_lBegPos = -1;
		m_lEndPos = -1;
	};

	var_4 IsQueueFull()
	{
		if(m_lCurQueueSize == m_lMaxQueueSize)
			return 1;

		return 0;
	};

	var_4 IsQueueEmpty()
	{
		if(m_lCurQueueSize == 0)
			return 1;

		return 0;
	};

	var_4 GetQueueSize()
	{
		return m_lCurQueueSize;
	}

	void PushData(T_Key tKey)
	{
		pthread_mutex_lock(&m_lock);
		
		while(m_lCurQueueSize == m_lMaxQueueSize)		
			pthread_cond_wait(&m_cond, &m_lock);
		
		m_tpQueue[m_lEndPos++] = tKey;
		m_lEndPos %= m_lNowQueueSize;
		m_lCurQueueSize++;
		
		if (m_lCurQueueSize == 1)
			pthread_cond_broadcast(&m_cond);

		pthread_mutex_unlock(&m_lock);		
	};
	
	T_Key PopData()
	{
		pthread_mutex_lock(&m_lock);
		
		while(m_lCurQueueSize == 0)
			pthread_cond_wait(&m_cond, &m_lock);
		
		T_Key tResult = m_tpQueue[m_lBegPos++];
		m_lBegPos %= m_lNowQueueSize;
		m_lCurQueueSize--;

		if (m_lCurQueueSize + 1 == m_lMaxQueueSize)
			pthread_cond_broadcast(&m_cond);

		pthread_mutex_unlock(&m_lock);
		
		return tResult;
	}

	var_4 PushData_NB(T_Key tKey)
	{
		if(m_lCurQueueSize == m_lMaxQueueSize)
			return -1;

		pthread_mutex_lock(&m_lock);
		
		if(m_lCurQueueSize == m_lMaxQueueSize)
		{
			pthread_mutex_unlock(&m_lock);
			return -1;
		}
		
		m_tpQueue[m_lEndPos++] = tKey;
		m_lEndPos %= m_lNowQueueSize;
		m_lCurQueueSize++;
		
		pthread_mutex_unlock(&m_lock);

		return 0;
	};

	var_4 PopData_NB(T_Key& tKey)
	{
		if(m_lCurQueueSize == 0)
			return -1;

		pthread_mutex_lock(&m_lock);

		if(m_lCurQueueSize == 0)
		{
			pthread_mutex_unlock(&m_lock);
			return -1;
		}
		
		tKey = m_tpQueue[m_lBegPos++];
		m_lBegPos %= m_lNowQueueSize;
		m_lCurQueueSize--;
		
		pthread_mutex_unlock(&m_lock);

		return 0;
	};

	const var_1* GetVersion()
	{
		// v1.000 - 2008.08.26 - 初始版本
		// v1.001 - 2008.12.04 - 增加函数 IsQueueFull(), IsQueueEmpty()
		// v1.100 - 2009.04.10 - 增加跨平台支持
		// v1.101 - 2011.04.27 - 增加非阻塞函数 PushData_NB(), PopData_NB()
		// v2.000 - 2013.06.05 - 修改sleep等待, 改为使用条件变量, 现版本暂不支持 windows 平台
		return "v2.000";
	}

private:
	var_4 m_lNowQueueSize;
	
	var_4 m_lMaxQueueSize;
	var_4 m_lCurQueueSize;

	T_Key* m_tpQueue;

	var_4 m_lBegPos;
	var_4 m_lEndPos;

	pthread_mutex_t m_lock;
	pthread_cond_t  m_cond;
};

#endif // _UT_QUEUE_EVENT_H_
