#include "catconnect.h"
#include "tier1/KeyValues.h"
#include "catfiles.h"
#include "printers.h"
#include "ctfpartyclient.h"
#include "cbaseentity.h"
#include "globals.h"
#include "isteamfriends.h"
#include "menu/menu.h"
#include "visual/drawer.h"
#include "visual/glow.h"
#include <boost/algorithm/string.hpp>
#include "cmdwrapper.h"

#include <optional>
#include <vector>

#define CAT_IDENTIFY 0xCA7
#define CAT_REPLY 0xCA8
#define CAT_FAKE 0xCA9

#define CHAT_FILTER_NONE 0
#define CHAT_FILTER_JOINLEAVE 0x1
#define CHAT_FILTER_NAMECHANGE 0x2
#define CHAT_FILTER_PUBLICCHAT 0x4
#define CHAT_FILTER_SERVERMSG 0x8
#define CHAT_FILTER_TEAMCHANGE 0x10
#define CHAT_FILTER_ACHIEVEMENT 0x20

NSCore::CSetting notify_partychat(xorstr_("catconnect.partychat.notify.bots"), xorstr_("1"));
NSCore::CSetting notify_gamechat(xorstr_("catconnect.gamechat.notify.bots"), xorstr_("1"));
NSCore::CSetting notify_saychat(xorstr_("catconnect.chat.notify.bots"), xorstr_("0"));
NSCore::CSetting votekicks_manage(xorstr_("catconnect.votekicks.manage"), xorstr_("1"));
NSCore::CSetting votekicks_showvoters(xorstr_("catconnect.votekicks.partychat.notifyvoters"), xorstr_("1"));
NSCore::CSetting scoreboard_showcats(xorstr_("catconnect.scoreboard.showcats"), xorstr_("1"));
NSCore::CSetting scoreboard_showfriends(xorstr_("catconnect.scoreboard.showfriends"), xorstr_("1"));
NSCore::CSetting debug_show(xorstr_("catconnect.showdebug"), xorstr_("0"));

struct SSavedCat
{
	uint32_t m_iFriendID;
	uint8_t m_iIsCat; //1 = cat, 2 = fakecat
};

void CCatConnect::Init()
{
	//also install hook to listen for catbots

	NSInterfaces::g_pGameEventsMan2->AddListener(&ms_GameEventAchievementListener, xorstr_("achievement_earned"), false);
	NSInterfaces::g_pGameEventsMan2->AddListener(&ms_GameEventPlayerListener, xorstr_("player_connect_client"), false);
	NSInterfaces::g_pGameEventsMan2->AddListener(&ms_GameEventVoteCastListener, xorstr_("vote_cast"), false);

	//load saved data
	LoadSavedCats();

	//init gui
	NSCore::CDrawer::Init();
	NSMenu::g_CMenu.CreateGUI();

	//and glow
	NSCore::GlowInit();

	//and interface
	static CCatConnectExpose CCatConnectExtern;
	g_pCatConnectIface = &CCatConnectExtern;
	NSGlobals::g_mMyInterfaces[xorstr_(CATCONNECT_IFACE_VERSION)] = g_pCatConnectIface;

	//print message about successful injection
	NSUtils::PrintToClientConsole(Color{ 0, 255, 0, 255 }, xorstr_("[CatConnect] Successfuly injected!\n"));
	NSGlobals::g_bCatConnectInited = true;

	//Handle late injection
	if (NSInterfaces::g_pEngineClient->IsInGame())
		OnMapStart();

	//and don't forget about timer!
	ms_pCheckTimer = g_CTimerMan.CreateTimer(nullptr, 120.0);
	ms_pCheckTimer->SetCallback(NSUtils::TimerCallbackFn(&CCatConnect::OnAuthTimer));
	ms_pCheckTimer->SetFlags(TIMER_REPEAT);
	ms_pCheckTimer->Trigger(); //trigger timer first time
}

void CCatConnect::Destroy()
{
	NSInterfaces::g_pGameEventsMan2->RemoveListener(&ms_GameEventAchievementListener);
	NSInterfaces::g_pGameEventsMan2->RemoveListener(&ms_GameEventPlayerListener);
	NSInterfaces::g_pGameEventsMan2->RemoveListener(&ms_GameEventVoteCastListener);
	//save cats
	SaveCats();
	CCatFiles::SaveData();
	//we destroyed
	NSGlobals::g_bCatConnectInited = false;
}

void CCatConnect::Trigger()
{
	if (!NSInterfaces::g_pEngineClient->IsInGame())
	{
		if (debug_show.GetBool())
			NSUtils::PrintToClientConsole(Color{ 255, 0, 0, 255 }, xorstr_("[CatConnect] Cannot force auth while not in game!\n"));
		return;
	}
	if (ms_pCheckTimer)
		ms_pCheckTimer->Trigger();
	//NSUtils::PrintToClientChat(xorstr_("\x07%06X[CatConnect]\x07%06X Successfuly sent CAT auth message to the server!"), 0xE05938, 0x00AA21);
}

void CCatConnect::OnScoreBoardUpdate(void * pThis)
{
	void * pRealThis = (void *)NSUtils::CVirtualMemberTableMan::FindCompleteThisPointer((void ***)pThis);
	void * pPlayerListBlue = *(void **)((char *)pRealThis + 0x234);
	void * pPlayerListRed = *(void **)((char *)pRealThis + 0x238); //0x234 && 0x238 are offsets relative to this pointer

	//223 - vtable offset of AddItem
	//224 - vtable offset of ModifyItem
	//225 - vtable offset of RemoveItem
	//228 - vtable offset of SetItemFgColor
	//229 - vtable offset of SetItemBgColor
	//240 - vtable offset of GetSelectedItem
	//243 - vtable offset of GetItemData
	//246 - vtable offset of IsItemIDValid
	//248 - vtable offset of GetItemCount
	//249 - vtable offset of GetItemIDFromRow
	//250 - vtable offset of GetRowFromItemID

	for (uint8_t iList = 0; iList < 2; iList++)
	{
		void * pCurrentList = (!iList ? pPlayerListBlue : pPlayerListRed);
		for (int iNumItems = CALL_VFUNC_OFFS(int (__thiscall *)(void * pThis), pCurrentList, 248)(pCurrentList) - 1; iNumItems >= 0; iNumItems--) //GetItemCount
		{
			int iItemID = CALL_VFUNC_OFFS(int (__thiscall *)(void * pThis, int), pCurrentList, 249)(pCurrentList, iNumItems); //GetItemIDFromRow
			if (!CALL_VFUNC_OFFS(bool (__thiscall *)(void * pThis, int), pCurrentList, 246)(pCurrentList, iItemID)) //IsItemIDValid
				continue;
			KeyValues * pKV = CALL_VFUNC_OFFS(KeyValues * (__thiscall *)(void * pThis, int), pCurrentList, 243)(pCurrentList, iItemID); //GetItemData
			if (!pKV)
				continue;
			int iClient = pKV->GetInt(xorstr_("playerIndex"));
			int iConnected = pKV->GetInt(xorstr_("connected"));
			//check if client actually connected. 0 - not connected, 1 - lost connection / connecting, 2 - connected
			if (iConnected != 2)
				continue; //we don't care
			ECatState eClientState = GetClientState(iClient);
			if (eClientState != CatState_Default)
			{
				if (!(scoreboard_showcats.GetBool() && (eClientState == CatState_Cat || eClientState == CatState_FakeCat) ||
					scoreboard_showfriends.GetBool() && (eClientState == CatState_Friend || eClientState == CatState_Party)))
					continue;

				//if he is a CAT, then set his color to green on scoreboard and show his class

				Color cScoreBoardColor = Color(255, 255, 255, 255);
				switch (eClientState)
				{
				case CatState_Cat:
					cScoreBoardColor = Color(0, 255, 0, 255);
					break;
				case CatState_Friend:
				case CatState_Party:
					cScoreBoardColor = Color(0, 213, 153, 255);
					break;
				case CatState_FakeCat:
					cScoreBoardColor = Color(0, 0, 255, 255);
					break;
				}

				IClientNetworkable * pNetworkable = NSInterfaces::g_pClientEntityList->GetClientNetworkable(iClient);
				IClientUnknown * pUnknown = nullptr;
				if (pNetworkable && (pUnknown = pNetworkable->GetIClientUnknown()))
				{
					NSReclass::CBaseEntity * pClientEnt = (NSReclass::CBaseEntity *)pUnknown->GetBaseEntity();
					if (pClientEnt)
					{
						//this method is proper, but kinda retarded.
						//TODO: Better retrieve this info from color. If prev set color alpha != 255 => player is dead
						bool bAlive = pClientEnt->IsPlayerAlive(); //better get this from CTFPlayerResource?
						if (!bAlive)
							cScoreBoardColor[3] /= 3;
						NSReclass::CBaseEntity * pMySelf = (NSReclass::CBaseEntity *)NSGlobals::g_pLocalPlayer;
						if (eClientState == CatState_Cat && pMySelf && pMySelf->GetTeam() != pClientEnt->GetTeam() && pClientEnt->GetTeam() > 1) //2 == red, 3 == blue
						{
							int iClass = pClientEnt->GetClass();
							int iClassEmblem = 0;
							if (bAlive)
								iClassEmblem = *(int *)((char *)pRealThis + 0x360 + iClass * 4);
							else
								iClassEmblem = *(int *)((char *)pRealThis + 0x384 + iClass * 4);
							//It's not a cheat! Show class of CATs only, not fake cats!
							pKV->SetInt(xorstr_("class"), iClassEmblem);
							//we need to create new instance of KeyValues cuz old will be deleted
							KeyValues * pNewKV = pKV->MakeCopy();
							CALL_VFUNC_OFFS(bool (__thiscall *)(void * pThis, int iItem, int iSection, const KeyValues * pKV), pCurrentList, 224)(pCurrentList, iItemID, 0, pNewKV); //ModifyItem
							pNewKV->deleteThis(); //it was copied by called function. Now delete this to prevent memory leak
						}
					}
				}

				CALL_VFUNC_OFFS(void (__thiscall *)(void * pThis, int, Color), pCurrentList, 228)(pCurrentList, iItemID, cScoreBoardColor); //SetItemFgColor
			}
		}
	}
}

std::string CCatConnect::OnChatMessage(int iClient, int iFilter, const char * pMessage)
{
	std::string sOriginalMsg = pMessage;
	if (iClient > 0)
	{
		player_info_t sInfo;
		if (!NSInterfaces::g_pEngineClient->GetPlayerInfo(iClient, &sInfo) || IsCat(sInfo.friendsID) != 1)
			return sOriginalMsg;
		if (iFilter == CHAT_FILTER_NONE || (iFilter & CHAT_FILTER_PUBLICCHAT))
		{
			//remove newlines spammed by cats from chat
			boost::erase_all(sOriginalMsg, xorstr_("\r"));
			boost::erase_all(sOriginalMsg, xorstr_("\n"));
		}
	}
	return sOriginalMsg;
}

void CCatConnect::OnMapStart()
{
	//reinit it here to fix gay black glow
	NSCore::GlowCreate();
	//we just joined
	ms_bJustJoined = true;
}

void CCatConnect::OnMapEnd()
{
	NSCore::GlowDestroy();
}

bool CCatConnect::OnClientCommand(const char * pCmdLine) { return NSCore::CCmdWrapper::OnStringCommand(pCmdLine); }

void CCatConnect::CreateMove(float flInputSample, CUserCmd * pCmd)
{
	if (ms_bJustJoined && NSInterfaces::g_pEngineClient->IsInGame())
	{
		//create another auth timer here
		auto * pTimer = g_CTimerMan.CreateTimer(nullptr, 5.0);
		pTimer->SetCallback(NSUtils::TimerCallbackFn(&CCatConnect::OnAuthTimer));
		ms_bJustJoined = false;
	}
}

void CCatConnect::OnPotentialVoteKickStarted(int iTeam, int iCaller, int iTarget, const char * pReason)
{
	if (!votekicks_manage.GetBool())
		return;

	player_info_t sInfoTarget, sInfoCaller;
	auto pMySelf = ((NSReclass::CBaseEntity *)NSGlobals::g_pLocalPlayer);

	if (iCaller <= 0 || iTarget <= 0 || !pMySelf || iTeam != pMySelf->GetTeam() || !NSInterfaces::g_pEngineClient->GetPlayerInfo(iTarget, &sInfoTarget) || !NSInterfaces::g_pEngineClient->GetPlayerInfo(iCaller, &sInfoCaller))
		return;

	if (iCaller == NSInterfaces::g_pEngineClient->GetLocalPlayer())
		return; //we don't care

	char * pVoteOption = new char[32];
	*pVoteOption = 0;

	if (iTarget != NSInterfaces::g_pEngineClient->GetLocalPlayer())
	{
		ECatState eTargetState = GetClientState(iTarget);
		ECatState eCallerState = GetClientState(iCaller);

		switch (eTargetState)
		{
			case CatState_Cat:
			{
				NSUtils::PrintToPartyChat(xorstr_("[CatConnect] Player %s ([U:1:%u]) trying to kick CAT %s ([U:1:%u])! Voting no."), sInfoCaller.name, sInfoCaller.friendsID, sInfoTarget.name, sInfoTarget.friendsID);
				strcpy(pVoteOption, xorstr_("vote option2"));
				NSUtils::ITimer * pTimer = g_CTimerMan.CreateTimer(pVoteOption, 0.5);
				pTimer->SetCallback(&CCatConnect::OnVoteTimer);
				break;
			}
			case CatState_Friend:
			{
				NSUtils::PrintToPartyChat(xorstr_("[CatConnect] Player %s ([U:1:%u]) trying to kick friend %s ([U:1:%u])! Voting no."), sInfoCaller.name, sInfoCaller.friendsID, sInfoTarget.name, sInfoTarget.friendsID);
				strcpy(pVoteOption, xorstr_("vote option2"));
				NSUtils::ITimer * pTimer = g_CTimerMan.CreateTimer(pVoteOption, 0.5);
				pTimer->SetCallback(&CCatConnect::OnVoteTimer);
				break;
			}
			case CatState_Party:
			{
				switch (eCallerState)
				{
					case CatState_Friend:
					{
						if (InSameParty(sInfoCaller.friendsID))
						{
							NSUtils::PrintToPartyChat(xorstr_("[CatConnect] Friend %s ([U:1:%u]) trying to kick party member %s ([U:1:%u])! Voting yes."), sInfoCaller.name, sInfoCaller.friendsID, sInfoTarget.name, sInfoTarget.friendsID);
							strcpy(pVoteOption, xorstr_("vote option1"));
						}
						else
						{
							NSUtils::PrintToPartyChat(xorstr_("[CatConnect] Friend %s ([U:1:%u]) trying to kick party member %s ([U:1:%u])! But friend not in party, voting no."), sInfoCaller.name, sInfoCaller.friendsID, sInfoTarget.name, sInfoTarget.friendsID);
							strcpy(pVoteOption, xorstr_("vote option2"));
						}
						NSUtils::ITimer * pTimer = g_CTimerMan.CreateTimer(pVoteOption, 0.5);
						pTimer->SetCallback(&CCatConnect::OnVoteTimer);
						break;
					}
					case CatState_Party:
					{
						NSUtils::PrintToPartyChat(xorstr_("[CatConnect] Party member %s ([U:1:%u]) trying to kick party member %s ([U:1:%u])! Voting no."), sInfoCaller.name, sInfoCaller.friendsID, sInfoTarget.name, sInfoTarget.friendsID);
						strcpy(pVoteOption, xorstr_("vote option2"));
						NSUtils::ITimer * pTimer = g_CTimerMan.CreateTimer(pVoteOption, 0.5);
						pTimer->SetCallback(&CCatConnect::OnVoteTimer);
						break;
					}
					case CatState_FakeCat:
					case CatState_Cat:
					default:
					{
						NSUtils::PrintToPartyChat(xorstr_("[CatConnect] %s %s ([U:1:%u]) trying to kick party member %s ([U:1:%u])! Voting no."), (eCallerState == CatState_Cat ? xorstr_("CAT") : (eCallerState == CatState_FakeCat ? xorstr_("Fake CAT") : xorstr_("Player"))), sInfoCaller.name, sInfoCaller.friendsID, sInfoTarget.name, sInfoTarget.friendsID);
						strcpy(pVoteOption, xorstr_("vote option2"));
						NSUtils::ITimer * pTimer = g_CTimerMan.CreateTimer(pVoteOption, 0.5);
						pTimer->SetCallback(&CCatConnect::OnVoteTimer);
						break;
					}
				}
				break;
			}
			case CatState_FakeCat:
			default:
			{
				switch (eCallerState)
				{
					case CatState_Friend:
					{
						NSUtils::PrintToPartyChat(xorstr_("[CatConnect] Friend %s ([U:1:%u]) trying to kick %s %s ([U:1:%u])! Voting yes."), sInfoCaller.name, sInfoCaller.friendsID, (eTargetState == CatState_FakeCat ? xorstr_("fake CAT") : xorstr_("player")), sInfoTarget.name, sInfoTarget.friendsID);
						strcpy(pVoteOption, xorstr_("vote option1"));
						NSUtils::ITimer * pTimer = g_CTimerMan.CreateTimer(pVoteOption, 0.5);
						pTimer->SetCallback(&CCatConnect::OnVoteTimer);
						break;
					}
					case CatState_Party:
					{
						NSUtils::PrintToPartyChat(xorstr_("[CatConnect] Party member %s ([U:1:%u]) trying to kick %s %s ([U:1:%u])! Voting yes."), sInfoCaller.name, sInfoCaller.friendsID, (eTargetState == CatState_FakeCat ? xorstr_("fake CAT") : xorstr_("player")), sInfoTarget.name, sInfoTarget.friendsID);
						strcpy(pVoteOption, xorstr_("vote option1"));
						NSUtils::ITimer * pTimer = g_CTimerMan.CreateTimer(pVoteOption, 0.5);
						pTimer->SetCallback(&CCatConnect::OnVoteTimer);
						break;
					}
					case CatState_Cat:
					{
						NSUtils::PrintToPartyChat(xorstr_("[CatConnect] CAT %s ([U:1:%u]) trying to kick %s %s ([U:1:%u])! Voting yes."), sInfoCaller.name, sInfoCaller.friendsID, (eTargetState == CatState_FakeCat ? xorstr_("fake CAT") : xorstr_("player")), sInfoTarget.name, sInfoTarget.friendsID);
						strcpy(pVoteOption, xorstr_("vote option1"));
						NSUtils::ITimer * pTimer = g_CTimerMan.CreateTimer(pVoteOption, 0.5);
						pTimer->SetCallback(&CCatConnect::OnVoteTimer);
						break;
					}
					case CatState_FakeCat:
					default:
					{
						NSUtils::PrintToPartyChat(xorstr_("[CatConnect] %s %s ([U:1:%u]) trying to kick %s %s ([U:1:%u])! Voting no."), (eCallerState == CatState_FakeCat ? xorstr_("Fake CAT") : xorstr_("Player")), sInfoCaller.name, sInfoCaller.friendsID, (eTargetState == CatState_FakeCat ? xorstr_("fake CAT") : xorstr_("player")), sInfoTarget.name, sInfoTarget.friendsID);
						strcpy(pVoteOption, xorstr_("vote option2"));
						NSUtils::ITimer * pTimer = g_CTimerMan.CreateTimer(pVoteOption, 0.5);
						pTimer->SetCallback(&CCatConnect::OnVoteTimer);
						break;
					}
				}
				break;
			}
		}
	}
	else
	{
		NSUtils::PrintToPartyChat(xorstr_("[CatConnect] Player %s ([U:1:%u]) trying to kick me! Voting no."), sInfoCaller.name, sInfoCaller.friendsID);
		strcpy(pVoteOption, xorstr_("vote option2")); //vote no if we didn't vote yet
		NSUtils::ITimer * pTimer = g_CTimerMan.CreateTimer(pVoteOption, 0.5);
		pTimer->SetCallback(&CCatConnect::OnVoteTimer);
	}

	if (!*pVoteOption)
		delete[] pVoteOption; //it was unused
}

void CCatConnect::LoadSavedCats()
{
	auto oCats = CCatFiles::GetSectionByID(Section_Cats);
	if (!oCats.has_value())
		return; //should never happen
	auto &vCats = oCats.value();
	if (!(*vCats)->size())
		return; //there are no cats. Is this first run?
	const char * pActualData = (*vCats)->data();
	uint32_t iCountOfCats = uint32_t((*vCats)->size() / sizeof(SSavedCat));
	for (uint32_t iCat = 0; iCat < iCountOfCats; iCat++)
	{
		SSavedCat * pCat = (SSavedCat *)((char *)pActualData + sizeof(SSavedCat) * iCat);
		ms_mIsCat[pCat->m_iFriendID] = pCat->m_iIsCat;
	}
}

void CCatConnect::SaveCats()
{
	auto oCats = CCatFiles::GetSectionByID(Section_Cats);
	if (!oCats.has_value())
	{
		//should we log this?
		return; //should never happen
	}
	auto &vCats = oCats.value();
	(*vCats)->clear();
	SSavedCat sCat;
	for (auto iCat : ms_mIsCat)
	{
		sCat.m_iFriendID = iCat.first;
		sCat.m_iIsCat = iCat.second;
		(*vCats)->insert((*vCats)->end(), (char *)&sCat, (char *)&sCat + sizeof(SSavedCat));
	}
}

bool CCatConnect::OnFakeAuthTimer(NSUtils::ITimer * pTimer, void * pData)
{
	if (!NSInterfaces::g_pEngineClient->IsInGame())
		return false;

	SendCatMessage(CAT_FAKE);
	return false;
}

bool CCatConnect::OnAuthTimer(NSUtils::ITimer * pTimer, void * pData)
{
	//return true if success, false if timer should be stopped
	//we are not in-game, so we can't send messages to cats :c
	if (!NSInterfaces::g_pEngineClient->IsInGame()) return true;

	//oh yeah! We are in game. Now send message to all cats!

	if (debug_show.GetBool())
		NSUtils::PrintToClientConsole(Color{ 0, 0, 255, 255 }, xorstr_("[CatConnect] Sending CAT auth message to the server...\n"));

	SendCatMessage(CAT_IDENTIFY);

	auto * pTimer2 = g_CTimerMan.CreateTimer(nullptr, 2.0);
	pTimer2->SetCallback(NSUtils::TimerCallbackFn(&CCatConnect::OnFakeAuthTimer));

	return true;
}

bool CCatConnect::OnVoteTimer(NSUtils::ITimer * pTimer, void * pData)
{
	const char * pVoteOption = (const char *)pData;
	if (pVoteOption)
	{
		if (*pVoteOption)
		{
			NSInterfaces::g_pEngineClient->ClientCmd_Unrestricted(pVoteOption);
			memset(pData, 0, strlen(pVoteOption));
		}
		delete[] pVoteOption;
	}
	return false;
}

void CCatConnect::SendCatMessage(int iMessage)
{
	//static IAchievementMgr * pAchivMan = NSInterfaces::g_pEngineClient->GetAchievementMgr();
	//pAchivMan->AwardAchievement(iMessage);

	KeyValues * pKV = new KeyValues(xorstr_("AchievementEarned"));
	pKV->SetInt(xorstr_("achievementID"), iMessage);
	NSInterfaces::g_pEngineClient->ServerCmdKeyValues(pKV);
}

uint8_t CCatConnect::IsCat(int iIndex)
{
	if (ms_mIsCat.find(iIndex) == ms_mIsCat.end()) return 0;
	return ms_mIsCat[iIndex];
}

void CCatConnect::CAchievementListener::FireGameEvent(IGameEvent * pEvent)
{
	if (!pEvent) return; //wtf?

	//now process event
	int iClient = pEvent->GetInt(xorstr_("player"), 0xDEAD);
	int iAchv = pEvent->GetInt(xorstr_("achievement"), 0xDEAD);

	if (iClient != 0xDEAD && (iAchv == CAT_IDENTIFY || iAchv == CAT_REPLY || iAchv == CAT_FAKE))
	{
		//if so, then it's cathook user or catbot. Let'say to him we are catbot too :P
		bool bReply = (iAchv == CAT_IDENTIFY);

		player_info_t sInfo;
		if (!NSInterfaces::g_pEngineClient->GetPlayerInfo(iClient, &sInfo))
			return;

		if (iClient != NSInterfaces::g_pEngineClient->GetLocalPlayer())
		{
			uint8_t iIsCat = IsCat(sInfo.friendsID);
			if (bReply)
			{
				//send reply
				if (!iIsCat || debug_show.GetBool()) //print message only if it's unknown cat
					NSUtils::PrintToClientConsole(Color{ 0, 255, 0, 255 }, xorstr_("[CatConnect] Received CAT auth message from player %s ([U:1:%u])! Sending response and marking him as CAT.\n"), sInfo.name, sInfo.friendsID);
				SendCatMessage(CAT_REPLY);
			}
			else if (!iIsCat || debug_show.GetBool())
				NSUtils::PrintToClientConsole(Color{ 0, 255, 0, 255 }, xorstr_("[CatConnect] Received CAT %s message from player %s ([U:1:%u])! Marking him as CAT.\n"), (iAchv == CAT_FAKE ? xorstr_("fake") : xorstr_("reply")), sInfo.name, sInfo.friendsID);

			if (!iIsCat && iAchv != CAT_FAKE)
			{
				ms_mIsCat[sInfo.friendsID] = 1;
				if (notify_partychat.GetBool())
					NSUtils::PrintToPartyChat(xorstr_("[CatConnect] Player %s ([U:1:%u]) marked as CAT!"), sInfo.name, sInfo.friendsID);
				if (notify_saychat.GetBool())
					NSUtils::PrintToChatAll(!!(notify_saychat.GetInt() - 1), xorstr_("[CatConnect] Player %s ([U:1:%u]) marked as CAT!"), sInfo.name, sInfo.friendsID);
				if (notify_gamechat.GetBool())
					NSUtils::PrintToClientChat(xorstr_("\x07%06X[CatConnect]\x07%06X Player %s ([U:1:%u]) marked as CAT (catbot or cathook user). Most likely it's a catbot. Don't kill him or you will pay!"), 0xE05938, 0xCD71E1, sInfo.name, sInfo.friendsID);
				SaveCats();
				CCatFiles::SaveData();
			}
			else if (iIsCat == 1 && iAchv == CAT_FAKE)
			{
				ms_mIsCat[sInfo.friendsID] = 2;
				if (notify_partychat.GetBool())
					NSUtils::PrintToPartyChat(xorstr_("[CatConnect] Player %s ([U:1:%u]) remarked as fake CAT (CatConnect user)!"), sInfo.name, sInfo.friendsID);
				if (notify_saychat.GetBool())
					NSUtils::PrintToChatAll(!!(notify_saychat.GetInt() - 1), xorstr_("[CatConnect] Player %s ([U:1:%u]) remarked fake CAT (CatConnect user)!"), sInfo.name, sInfo.friendsID);
				if (notify_gamechat.GetBool())
					NSUtils::PrintToClientChat(xorstr_("\x07%06X[CatConnect]\x07%06X Player %s ([U:1:%u]) remarked as fake CAT (CatConnect user)!"), 0xE05938, 0xCD71E1, sInfo.name, sInfo.friendsID);
				SaveCats();
				CCatFiles::SaveData();
			}
			else if (!iIsCat && iAchv == CAT_FAKE)
			{
				ms_mIsCat[sInfo.friendsID] = 2;
				if (notify_partychat.GetBool())
					NSUtils::PrintToPartyChat(xorstr_("[CatConnect] Player %s ([U:1:%u]) marked as fake CAT (CatConnect user)!"), sInfo.name, sInfo.friendsID);
				if (notify_saychat.GetBool())
					NSUtils::PrintToChatAll(!!(notify_saychat.GetInt() - 1), xorstr_("[CatConnect] Player %s ([U:1:%u]) marked fake CAT (CatConnect user)!"), sInfo.name, sInfo.friendsID);
				if (notify_gamechat.GetBool())
					NSUtils::PrintToClientChat(xorstr_("\x07%06X[CatConnect]\x07%06X Player %s ([U:1:%u]) marked as fake CAT (CatConnect user)!"), 0xE05938, 0xCD71E1, sInfo.name, sInfo.friendsID);
				SaveCats();
				CCatFiles::SaveData();
			}
		}
	}
}

void CCatConnect::CPlayerListener::FireGameEvent(IGameEvent * pEvent)
{
	if (!pEvent) return;

	const char * pEventName = pEvent->GetName();

	if (!pEventName || !*pEventName)
		return;

	if (!strcmp(pEventName, xorstr_("player_connect_client")))
	{
		const char * pNetID = pEvent->GetString(xorstr_("networkid"));
		if (strlen(pNetID) < 6)
			return;
		int iFriendID = atoi(pNetID + 5);
		uint8_t iCat = IsCat(iFriendID);
		if (iCat == 1)
		{
			const char * pName = pEvent->GetString(xorstr_("name"));
			if (notify_partychat.GetBool())
				NSUtils::PrintToPartyChat(xorstr_("[CatConnect] CAT %s (%s) joined the game."), pName, pNetID);
			if (notify_saychat.GetBool())
				NSUtils::PrintToChatAll(!!(notify_saychat.GetInt() - 1), xorstr_("[CatConnect] CAT %s (%s) joined the game."), pName, pNetID);
			if (notify_gamechat.GetBool())
				NSUtils::PrintToClientChat(xorstr_("\x07%06X[CatConnect]\x07%06X CAT %s (%s) joined the game. Most likely it's a catbot. Don't kill him or you will pay!"), 0xE05938, 0xCD71E1, pName, pNetID);
		}
		else if (iCat == 2)
		{
			const char * pName = pEvent->GetString(xorstr_("name"));
			if (notify_partychat.GetBool())
				NSUtils::PrintToPartyChat(xorstr_("[CatConnect] Fake CAT %s (%s) joined the game."), pName, pNetID);
			if (notify_saychat.GetBool())
				NSUtils::PrintToChatAll(!!(notify_saychat.GetInt() - 1), xorstr_("[CatConnect] Fake CAT %s (%s) joined the game."), pName, pNetID);
			if (notify_gamechat.GetBool())
				NSUtils::PrintToClientChat(xorstr_("\x07%06X[CatConnect]\x07%06X Fake CAT %s (%s) joined the game."), 0xE05938, 0xCD71E1, pName, pNetID);
		}
	}
}

void CCatConnect::CVoteListener::FireGameEvent(IGameEvent * pEvent)
{
	if (!votekicks_showvoters.GetBool())
		return;

	if (!strcmp(pEvent->GetName(), xorstr_("vote_cast")))
	{
		int iClient = pEvent->GetInt(xorstr_("entityid"));
		int iTeam = pEvent->GetInt(xorstr_("team"));
		player_info_t sInfo;
		if (iClient == NSInterfaces::g_pEngineClient->GetLocalPlayer() || iTeam != ((NSReclass::CBaseEntity *)NSGlobals::g_pLocalPlayer)->GetTeam() || !NSInterfaces::g_pEngineClient->GetPlayerInfo(iClient, &sInfo))
			return;

		bool bVoteOption = !pEvent->GetInt(xorstr_("vote_option"));

		NSUtils::PrintToPartyChat(xorstr_("[CatConnect] Player %s ([U:1:%u]) voted %s."), sInfo.name, sInfo.friendsID, bVoteOption ? xorstr_("yes") : xorstr_("no"));
	}
}

ECatState CCatConnect::GetClientState(int iClient)
{
	if (iClient <= 0 /*|| iClient > playerhelpers->GetMaxClients()*/)
		return CatState_Default;

	if (iClient == NSInterfaces::g_pEngineClient->GetLocalPlayer())
		return CatState_Friend;

	player_info_t sInfo;
	if (!NSInterfaces::g_pEngineClient->GetPlayerInfo(iClient, &sInfo))
		return CatState_Default;

	uint8_t iCat = IsCat(sInfo.friendsID);

	if (iCat == 1)
		return CatState_Cat;

	if (NSInterfaces::g_pSteamFriends)
	{
		CSteamID cSteamID = CSteamID(sInfo.friendsID, k_EUniversePublic, k_EAccountTypeIndividual);
		if (NSInterfaces::g_pSteamFriends->HasFriend(cSteamID, k_EFriendFlagImmediate))
			return CatState_Friend;
	}
	else
		NSUtils::PrintToClientConsole(Color{ 255, 0, 0, 255 }, xorstr_("[CatConnect] Error: ISteamFriends interface in null!\n"));

	std::vector<unsigned int> vPartyMembers = NSReclass::CTFPartyClient::GTFPartyClient()->GetPartySteamIDs();

	for (auto iFriend : vPartyMembers) //don't use std::find
		if (sInfo.friendsID == iFriend)
			return CatState_Party;

	if (iCat == 2)
		return CatState_FakeCat;

	return CatState_Default;
}

bool CCatConnect::InSameParty(int iIndex)
{
	std::vector<unsigned int> vPartyMembers = NSReclass::CTFPartyClient::GTFPartyClient()->GetPartySteamIDs();

	for (auto iFriend : vPartyMembers) //don't use std::find
		if (iFriend == iIndex)
			return true;
	
	return false;
}

ICatConnect * g_pCatConnectIface = nullptr;

NSUtils::ITimer * CCatConnect::ms_pCheckTimer;
CCatConnect::CAchievementListener CCatConnect::ms_GameEventAchievementListener;
CCatConnect::CPlayerListener CCatConnect::ms_GameEventPlayerListener;
CCatConnect::CVoteListener CCatConnect::ms_GameEventVoteCastListener;
bool CCatConnect::ms_bJustJoined = false;
std::map<uint32_t, uint8_t> CCatConnect::ms_mIsCat;