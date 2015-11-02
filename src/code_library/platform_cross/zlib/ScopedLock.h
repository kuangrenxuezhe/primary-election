/*! @file
********************************************************************************
<PRE>
ģ����       : <IndividuationDown>
�ļ���       : <ScopedLock.h>
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