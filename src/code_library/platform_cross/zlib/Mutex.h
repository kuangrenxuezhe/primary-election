
/*! @file
********************************************************************************
<PRE>
ģ����       : <IndividuationDown>
�ļ���       : <Mutex.h>
����ļ�     : 
�ļ�ʵ�ֹ��� : <> 
����         : <����ǿ>
�汾         : <1.00.00>
--------------------------------------------------------------------------------
��ע         : <ֻ������һ�����̺��̷߳���
				��Semaphore���ڿ���һ���������̷߳���>
--------------------------------------------------------------------------------
�޸ļ�¼ : 
�� ��        �汾     �޸���              �޸�����
YYYY/MM/DD   X.Y      <���߻��޸�����>    <�޸�����>
</PRE>
*******************************************************************************/

#ifndef _SYSTEM_MUTEX_H_2006_08_25
#define _SYSTEM_MUTEX_H_2006_08_25

#if defined(_WIN32)
#include "Mutex_WIN32.h"
#else
#include "Mutex_POSIX.h"
#endif

#include "ScopedLock.h"


class Mutex : private MutexImpl 
{
public:
	typedef ScopedLock<Mutex> ScopedLock;
	
	Mutex();
	
	virtual ~Mutex();
	
	///������ֹ�����߳�ʹ��
	void lock();
	
	///���Լ���,����������߳�ʹ�÷��� false
	///���򷵻� true
	bool tryLock();
	
	///���������
	void unlock();
	
private:
	Mutex(const Mutex&);
	
	Mutex& operator = (const Mutex&);
	
};

//
// inlines
//
inline void Mutex::lock()
{
	lockImpl();
}

inline bool Mutex::tryLock()
{
	return tryLockImpl();
}

inline void Mutex::unlock()
{
	unlockImpl();
}


#endif 