// UC_Queue_VL.h

#ifndef _UC_QUEUE_VL_H_
#define _UC_QUEUE_VL_H_

#include "UH_Define.h"

class UC_Queue_VL
{
public:
	UC_Queue_VL()
	{
        m_lOneSize = 0;
        
		m_lMaxQueueSize = -1;
		m_lCurQueueSize = -1;

		m_tpQueue = NULL;

		m_lBegPos = -1;
		m_lEndPos = -1;
		
		pthread_mutex_init(&m_lock, NULL);
		pthread_cond_init(&m_cond, NULL);
	};

	~UC_Queue_VL()
	{
		if(m_tpQueue)
			delete m_tpQueue;
		
		pthread_mutex_destroy(&m_lock);
		pthread_cond_destroy(&m_cond);
	};

	var_4 InitQueue(var_4 lMaxQueueSize, var_4 lOneSize)
	{
        m_lOneSize = lOneSize;
        
		m_lMaxQueueSize = lMaxQueueSize;
		m_lCurQueueSize = 0;

		m_lNowQueueSize = m_lMaxQueueSize;
		
        var_8 all_size = lOneSize * m_lMaxQueueSize;
        
		m_tpQueue = new var_1[all_size];
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

	var_vd PushData(var_vd* data)
	{
		pthread_mutex_lock(&m_lock);
		
		while(m_lCurQueueSize == m_lMaxQueueSize)		
			pthread_cond_wait(&m_cond, &m_lock);
		
        memcpy(m_tpQueue + m_lEndPos * m_lOneSize, data, m_lOneSize);
        
		++m_lEndPos %= m_lNowQueueSize;
		m_lCurQueueSize++;
		
		if (m_lCurQueueSize == 1)
			pthread_cond_broadcast(&m_cond);

		pthread_mutex_unlock(&m_lock);		
	}
	
	var_vd PopData(var_vd* data)
	{
		pthread_mutex_lock(&m_lock);
		
		while(m_lCurQueueSize == 0)
			pthread_cond_wait(&m_cond, &m_lock);
		
        memcpy(data, m_tpQueue + m_lBegPos * m_lOneSize, m_lOneSize);
        
		++m_lBegPos %= m_lNowQueueSize;
		m_lCurQueueSize--;

		if (m_lCurQueueSize + 1 == m_lMaxQueueSize)
			pthread_cond_broadcast(&m_cond);

		pthread_mutex_unlock(&m_lock);
	}

	var_4 PushData_NB(var_vd* data)
	{
		if(m_lCurQueueSize == m_lMaxQueueSize)
			return -1;

		pthread_mutex_lock(&m_lock);
		
		if(m_lCurQueueSize == m_lMaxQueueSize)
		{
			pthread_mutex_unlock(&m_lock);
			return -1;
		}
		
        memcpy(m_tpQueue + m_lEndPos * m_lOneSize, data, m_lOneSize);

		++m_lEndPos %= m_lNowQueueSize;
		m_lCurQueueSize++;
		
		pthread_mutex_unlock(&m_lock);

		return 0;
	};

	var_4 PopData_NB(var_vd* data)
	{
		if(m_lCurQueueSize == 0)
			return -1;

		pthread_mutex_lock(&m_lock);

		if(m_lCurQueueSize == 0)
		{
			pthread_mutex_unlock(&m_lock);
			return -1;
		}
		
        memcpy(data, m_tpQueue + m_lBegPos * m_lOneSize, m_lOneSize);

		++m_lBegPos %= m_lNowQueueSize;
		m_lCurQueueSize--;
		
		pthread_mutex_unlock(&m_lock);

		return 0;
	};

	const var_1* GetVersion()
	{
		// v1.000 - 2014.11.28 - ≥ı º∞Ê±æ
		return "v1.000";
	}

private:
    var_4 m_lOneSize;
    
	var_4 m_lNowQueueSize;
	
	var_4 m_lMaxQueueSize;
	var_4 m_lCurQueueSize;

	var_1* m_tpQueue;

	var_4 m_lBegPos;
	var_4 m_lEndPos;

	pthread_mutex_t m_lock;
	pthread_cond_t  m_cond;
};

#endif // _UC_QUEUE_VL_H_
