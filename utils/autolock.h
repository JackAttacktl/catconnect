#ifndef _AUTOLOCK_MUTEX_
#define _AUTOLOCK_MUTEX_

namespace NSUtils
{
	template<class TLockableMutex>
	class CAutoLocker
	{
	public:
		inline explicit CAutoLocker(TLockableMutex * pMutex) : m_pMutex(pMutex) { m_pMutex->lock(); }
		inline ~CAutoLocker() { m_pMutex->unlock(); }
	private:
		TLockableMutex * m_pMutex;
	};
};

#endif