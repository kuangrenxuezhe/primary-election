// UT_Queue.h

#ifndef _UT_QUEUE_EVENT_H_
#define _UT_QUEUE_EVENT_H_

#include "UH_Define.h"

// #define _USE_SPINLOCK_

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
        
#ifdef _USE_SPINLOCK_
        pthread_spin_init(&lock, 0);
#else
        pthread_mutex_init(&lock, NULL);
#endif
	};

	~UT_Queue()
	{
		if(m_tpQueue)
			delete m_tpQueue;
        
#ifdef _USE_SPINLOCK_
        pthread_spin_destroy(&lock);
#else
        pthread_mutex_destroy(&lock);
#endif
	};

	var_4 InitQueue(var_4 lMaxQueueSize)
	{
		m_lMaxQueueSize = lMaxQueueSize;
		m_lCurQueueSize = 0;
		
		m_tpQueue = new T_Key[m_lMaxQueueSize];
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
		for(;;)
		{
#ifdef _USE_SPINLOCK_
            pthread_spin_lock(&lock);
#else
            pthread_mutex_lock(&lock);
#endif
			if(m_lCurQueueSize >= m_lMaxQueueSize)
			{
#ifdef _USE_SPINLOCK_
                pthread_spin_unlock(&lock);
#else
                pthread_mutex_unlock(&lock);
#endif
				cp_sleep(1);
				continue;
			}
			m_tpQueue[m_lEndPos++] = tKey;
			m_lEndPos %= m_lMaxQueueSize;
			m_lCurQueueSize++;			
#ifdef _USE_SPINLOCK_
            pthread_spin_unlock(&lock);
#else
            pthread_mutex_unlock(&lock);
#endif
			break;
		}
	};

	T_Key PopData()
	{
		T_Key tResult;

		for(;;)
		{
#ifdef _USE_SPINLOCK_
            pthread_spin_lock(&lock);
#else
            pthread_mutex_lock(&lock);
#endif			
            if(m_lCurQueueSize <= 0)
            {
#ifdef _USE_SPINLOCK_
                pthread_spin_unlock(&lock);
#else
                pthread_mutex_unlock(&lock);
#endif
				cp_sleep(1);
				continue;
			}
			tResult = m_tpQueue[m_lBegPos++];
			m_lBegPos %= m_lMaxQueueSize;
			m_lCurQueueSize--;
#ifdef _USE_SPINLOCK_
        pthread_spin_unlock(&lock);
#else
        pthread_mutex_unlock(&lock);
#endif
			break;
		}

		return tResult;
	};

	var_4 PushData_NB(T_Key tKey)
	{
#ifdef _USE_SPINLOCK_
        pthread_spin_lock(&lock);
#else
        pthread_mutex_lock(&lock);
#endif
		if(m_lCurQueueSize >= m_lMaxQueueSize)
		{
#ifdef _USE_SPINLOCK_
            pthread_spin_unlock(&lock);
#else
            pthread_mutex_unlock(&lock);
#endif
			return -1;
		}
		m_tpQueue[m_lEndPos++] = tKey;
		m_lEndPos %= m_lMaxQueueSize;
		m_lCurQueueSize++;			
#ifdef _USE_SPINLOCK_
        pthread_spin_unlock(&lock);
#else
        pthread_mutex_unlock(&lock);
#endif
		return 0;
	};

	var_4 PopData_NB(T_Key& tKey)
	{
#ifdef _USE_SPINLOCK_
        pthread_spin_lock(&lock);
#else
        pthread_mutex_lock(&lock);
#endif
		if(m_lCurQueueSize <= 0)
		{
#ifdef _USE_SPINLOCK_
            pthread_spin_unlock(&lock);
#else
            pthread_mutex_unlock(&lock);
#endif
			return -1;
		}
		tKey = m_tpQueue[m_lBegPos++];
		m_lBegPos %= m_lMaxQueueSize;
		m_lCurQueueSize--;
#ifdef _USE_SPINLOCK_
        pthread_spin_unlock(&lock);
#else
        pthread_mutex_unlock(&lock);
#endif
		return 0;
	};

	// 得到当前版本号
	const var_1* GetVersion()
	{
		// v1.000 - 2008.08.26 - 初始版本
		// v1.001 - 2008.12.04 - 增加函数 IsQueueFull(), IsQueueEmpty()
		// v1.100 - 2009.04.10 - 增加跨平台支持
		// v1.101 - 2011.04.27 - 增加非阻塞函数 PushData_NB(), PopData_NB()
		return "v1.100";
	}

private:
	var_4 m_lMaxQueueSize;
	var_4 m_lCurQueueSize;

	T_Key* m_tpQueue;

	var_4 m_lBegPos;
	var_4 m_lEndPos;
    
#ifdef _USE_SPINLOCK_
    pthread_spinlock_t lock;
#else
    pthread_mutex_t lock;
#endif
};

#endif // _UT_QUEUE_EVENT_H_
