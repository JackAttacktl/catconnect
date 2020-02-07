#include "timers.h"
#include <cstdint>
#include <iterator>

NSUtils::ITimer * NSUtils::CTimerMan::CreateTimer(void * pData, float flInterval)
{
	CTimer * pTimer = new CTimer(pData, flInterval);
	m_vTimers.push_back(pTimer);
	return pTimer;
}

void NSUtils::CTimerMan::Update()
{
	//if (m_flLastCheck == 0.0)
	//{
		//m_flLastCheck = NSInterfaces::g_pGlobals->m_flCurTime;
		//return;
	//}
	//float flDelta = abs(NSInterfaces::g_pGlobals->m_flCurTime - m_flLastCheck);
	auto tcNow = std::chrono::high_resolution_clock::now();
	float flDelta = abs(float(double(std::chrono::duration_cast<std::chrono::milliseconds>(tcNow - m_ctLastTime).count()) / 1000.0));
	m_ctLastTime = tcNow;
	if (flDelta > 1.0) //umm something is abnormal...
		return;
	//m_flLastCheck = NSInterfaces::g_pGlobals->m_flCurTime;
	for (auto pTimer : m_vTimers)
		if (!pTimer->IsSeparate())
			pTimer->Update(flDelta);
}

bool NSUtils::CTimerMan::KillTimer(ITimer * pTimer)
{
	auto vIter = std::find(m_vTimers.begin(), m_vTimers.end(), pTimer);
	if (vIter == m_vTimers.end()) //timer is already an invalid
		return false;
	m_vTimers.erase(vIter);
	delete pTimer;
	return true;
}

void NSUtils::CTimerMan::TriggerTimer(NSUtils::ITimer * pTimer)
{
	auto vIter = std::find(m_vTimers.begin(), m_vTimers.end(), pTimer);
	if (m_vTimers.end() == vIter) //invalid timer???
		return;
	pTimer->Trigger();
}

std::vector<NSUtils::ITimer *> NSUtils::CTimerMan::m_vTimers;
//float NSUtils::CTimerMan::m_flLastCheck;
std::chrono::time_point<std::chrono::high_resolution_clock> NSUtils::CTimerMan::m_ctLastTime;
NSUtils::CTimerMan g_CTimerMan;