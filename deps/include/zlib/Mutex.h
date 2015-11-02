
/*! @file
********************************************************************************
<PRE>
模块名       : <IndividuationDown>
文件名       : <Mutex.h>
相关文件     : 
文件实现功能 : <> 
作者         : <和文强>
版本         : <1.00.00>
--------------------------------------------------------------------------------
备注         : <只能允许一个进程和线程访问
				而Semaphore用于控制一定数量的线程访问>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
YYYY/MM/DD   X.Y      <作者或修改者名>    <修改内容>
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
	
	///加锁防止其他线程使用
	void lock();
	
	///尝试加锁,如果被其他线程使用返回 false
	///否则返回 true
	bool tryLock();
	
	///解除互斥锁
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