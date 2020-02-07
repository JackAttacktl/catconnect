#ifndef _CATCONNECT_INC_
#define _CATCONNECT_INC_

#include <map>
#include <string>
#include <stdint.h>
#include "defs.h"
#include "igameevents.h"
#include "timers.h"
#include "icatconnect.h"

class CUserCmd;

class CCatConnect
{
public:
	static void Init();
	static void Destroy();
	static void Trigger();
	static void FASTERCALL OnScoreBoardUpdate(void * pThis);
	static std::string FASTERCALL OnChatMessage(int iClient, int iFilter, const char * pMessage);
	static void FASTERCALL OnPotentialVoteKickStarted(int iTeam, int iCaller, int iTarget, const char * pReason);
	static void OnMapStart();
	static void OnMapEnd();
	static bool OnClientCommand(const char * pCmdLine);
	static void FASTERCALL CreateMove(float flInputSample, CUserCmd * pCmd);
	static ECatState FASTERCALL GetClientState(int iClient);

private:
	class CAchievementListener : public IGameEventListener2
	{
	public:
		virtual	~CAchievementListener(void) {}
		virtual void FireGameEvent(IGameEvent * pEvent);
	};

	class CPlayerListener : public IGameEventListener2
	{
	public:
		virtual	~CPlayerListener(void) {}
		virtual void FireGameEvent(IGameEvent * pEvent);
	};

	class CVoteListener : public IGameEventListener2
	{
	public:
		virtual	~CVoteListener(void) {}
		virtual void FireGameEvent(IGameEvent * pEvent);
	};
	
	static bool OnAuthTimer(NSUtils::ITimer * pTimer, void * pData);
	static bool OnFakeAuthTimer(NSUtils::ITimer * pTimer, void * pData);
	static bool OnVoteTimer(NSUtils::ITimer * pTimer, void * pData);
	static void FASTERCALL SendCatMessage(int iMessage);
	static uint8_t FASTERCALL IsCat(int iIndex);
	static bool FASTERCALL InSameParty(int iIndex);

	static void LoadSavedCats();
	static void SaveCats();

private:
	static NSUtils::ITimer * ms_pCheckTimer;
	static CAchievementListener ms_GameEventAchievementListener;
	static CPlayerListener ms_GameEventPlayerListener;
	static CVoteListener ms_GameEventVoteCastListener;
	static bool ms_bJustJoined;
	static std::map<uint32_t, uint8_t> ms_mIsCat;
};

class CCatConnectExpose : public ICatConnect
{
public:
	virtual ~CCatConnectExpose() {}

	virtual ECatState GetClientState(int iClient) { return CCatConnect::GetClientState(iClient); }
	virtual void Trigger() { CCatConnect::Trigger(); }
};

extern ICatConnect * g_pCatConnectIface;

#endif