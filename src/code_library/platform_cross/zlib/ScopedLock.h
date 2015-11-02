/*! @file
********************************************************************************
<PRE>
模块名       : <IndividuationDown>
文件名       : <ScopedLock.h>
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

#ifndef _SCOPEDLOCK_H_2006_08_25
#define _SCOPEDLOCK_H_2006_08_25

template <class M>
class ScopedLock	
{
public:
	inline ScopedLock(M& mutex): _mutex(mutex)
	{
		_mutex.lock();
	}
	
	inline ~ScopedLock()
	{
		_mutex.unlock();
	}
	
private:
	
	M& _mutex;
	
	ScopedLock();
	
	ScopedLock(const ScopedLock&);
	
	ScopedLock& operator = (const ScopedLock&);
	
};

#endif 