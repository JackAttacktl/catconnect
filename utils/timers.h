#ifndef __TIMERS_INC_
#define __TIMERS_INC_

#include "interfaces.h"
#include "defs.h"
#include <vector>
#include <chrono>

#define TIMER_REPEAT (1<<0)

namespace NSUtils
{
	class CTimerMan;
	class ITimer;

	typedef bool (__cdecl * TimerCallbackFn)(ITimer * pTimer, void * pData);

	class ITimer
	{
	public:
		virtual ~ITimer() {}

		virtual void SetCallback(TimerCallbackFn pFunc) = 0;
		virtual void SetFlags(uint32_t iFlags) = 0;
		virtual void ResetTime(float flTime) = 0;
		virtual void Trigger() = 0;
		virtual bool IsSeparate() const = 0;
	private:
		friend class CTimerMan;
		virtual void Update(float flTime) = 0;
	};

	class CTimerMan
	{
	public:
		CTimerMan() { /*m_flLastCheck = 0.0;*/ m_ctLastTime = std::chrono::high_resolution_clock::now(); } //constructor must be called once!
		~CTimerMan() { for (auto pTimer : m_vTimers) delete pTimer; }
		static ITimer * FASTERCALL CreateTimer(void * pData, float flInterval);
		static bool FASTERCALL KillTimer(ITimer * pTimer);
		static void FASTERCALL TriggerTimer(ITimer * pTimer);
		static void FASTERCALL Update(); //Don't call this, it's called automatically in CreateMove

	private:
		static std::vector<ITimer *> m_vTimers;
		//static float m_flLastCheck;
		static std::chrono::time_point<std::chrono::high_resolution_clock> m_ctLastTime;
	};

	class CTimer : public ITimer
	{
	public:
		FORCEINLINE CTimer(void * pData, float flInterval) : m_iFlags(0), m_pCallback(nullptr), m_pData(pData), m_flInterval(flInterval), m_flTimeLeft(flInterval), m_bSeparate(false) {}
		FORCEINLINE virtual ~CTimer() {}

		virtual void SetCallback(TimerCallbackFn pFunc) { m_pCallback = pFunc; }
		virtual void SetFlags(uint32_t iFlags) { m_iFlags = iFlags; }
		virtual void ResetTime(float flTime){ m_flInterval = flTime; m_flTimeLeft = flTime; }
		virtual void Trigger() { m_flTimeLeft = 0.0f; Update(0.1f); }
		virtual bool IsSeparate() const { return m_bSeparate; }

	private:
		FORCEINLINE bool CallCallback() { if (m_pCallback) return m_pCallback(this, m_pData); return false; }
		virtual void Update(float flDelta)
		{
			m_flTimeLeft -= flDelta;
			if (m_flTimeLeft <= 0.0)
			{
				m_flTimeLeft = m_flInterval;
				if (CallCallback())
				{
					if (m_iFlags & TIMER_REPEAT)
						return;
				}
				CTimerMan::KillTimer(this);
			}
		}

	private:
		uint32_t m_iFlags;
		TimerCallbackFn m_pCallback;
		void * m_pData;
		float m_flInterval;
		float m_flTimeLeft;
		bool m_bSeparate;
		//CTimerMan * m_pMan;
	};
};

extern NSUtils::CTimerMan g_CTimerMan;

#endif