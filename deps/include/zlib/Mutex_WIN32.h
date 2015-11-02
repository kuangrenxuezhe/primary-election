
/*! @file
********************************************************************************
<PRE>
模块名       : <PageFilter>
文件名       : <Mutex_WIN32.h>
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


#ifndef _Mutex_WIN32_H_2006_10_21
#define _Mutex_WIN32_H_2006_10_21
#if defined(_WIN32)
#include <windows.h>


class MutexImpl
{
protected:
	MutexImpl();
	~MutexImpl();
	void lockImpl();
	bool tryLockImpl();
	void unlockImpl();
	
private:
	CRITICAL_SECTION _cs;
};


//typedef MutexImpl FastMutexImpl;


//
// inlines
//
inline void MutexImpl::lockImpl()
{
	EnterCriticalSection(&_cs);
}


inline bool MutexImpl::tryLockImpl()
{
	//return TryEnterCriticalSection(&_cs) == TRUE;
}


inline void MutexImpl::unlockImpl()
{
	LeaveCriticalSection(&_cs);
}
#endif
#endif 
