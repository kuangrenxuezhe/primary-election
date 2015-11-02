
/*! @file
********************************************************************************
<PRE>
模块名       : <PageFilter>
文件名       : <Mutex_POSIX.h>
相关文件     : 
文件实现功能 : <> 
作者         : <和文强>
版本         : <1.00.00>
--------------------------------------------------------------------------------
备注         : < A class that simplifies thread synchronization
				 with a mutex.
				The constructor accepts a Mutex and locks it.
				The destructor unlocks the mutex.>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
YYYY/MM/DD   X.Y      <作者或修改者名>    <修改内容>
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
