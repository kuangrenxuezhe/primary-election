
/*! @file
********************************************************************************
<PRE>
ģ����       : <PageFilter>
�ļ���       : <Mutex_POSIX.h>
����ļ�     : 
�ļ�ʵ�ֹ��� : <> 
����         : <����ǿ>
�汾         : <1.00.00>
--------------------------------------------------------------------------------
��ע         : < A class that simplifies thread synchronization
				 with a mutex.
				The constructor accepts a Mutex and locks it.
				The destructor unlocks the mutex.>
--------------------------------------------------------------------------------
�޸ļ�¼ : 
�� ��        �汾     �޸���              �޸�����
YYYY/MM/DD   X.Y      <���߻��޸�����>    <�޸�����>
</PRE>
*******************************************************************************/


#ifndef _Mutex_POSIX_H_2006_10_21
#define _Mutex_POSIX_H_2006_10_21

#include <pthread.h>
#include <errno.h>

class MutexImpl
{
protected:
	MutexImpl();
	MutexImpl(bool fast);

	~MutexImpl();
	void lockImpl();
	bool tryLockImpl();
	void unlockImpl();
	
private:
	pthread_mutex_t _mutex;
};


//
// inlines
//
inline void MutexImpl::lockImpl()
{
	pthread_mutex_lock(&_mutex);
}


inline bool MutexImpl::tryLockImpl()
{
	int rc = pthread_mutex_trylock(&_mutex);
	if (rc == 0)
		return true;
	else if (rc == 16)   // busy
		return false;
}


inline void MutexImpl::unlockImpl()
{
	pthread_mutex_unlock(&_mutex);
}

#endif 
