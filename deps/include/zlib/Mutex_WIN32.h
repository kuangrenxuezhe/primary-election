
/*! @file
********************************************************************************
<PRE>
ģ����       : <PageFilter>
�ļ���       : <Mutex_WIN32.h>
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
