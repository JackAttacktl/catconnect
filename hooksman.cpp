#include "hooksman.h"
#include "interfaces.h"
#include "timers.h"
#include "globals.h"
#include "icliententitylist.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include "vgui/IPanel.h"
#include "iviewport.h"
#include "tier1/KeyValues.h"
#include "inputsystem/iinputsystem.h"
#include "catconnect.h"
#include "printers.h"
#include "utils/hud.h"
#include "usermessages.h"
#include "menu/menu.h"
#include "visual/drawer.h"
#include "visual/glow.h"
#include "public/bitbuffix.h"
#include "logger.h"
#include <cstring>
#include <thread>
#include <chrono>

NSCore::CSetting remove_newlines(xorstr_("catconnect.chat.removeunprintable"), xorstr_("1"));

#define OFFSET_CHATPRINTF 19

#define OFFSET_SHOWPANEL1 2
#define OFFSET_SHOWPANEL2 1 //idk why, ida says so https://i.imgur.com/xlEERc2.png
#define OFFSET_FINDPANEL 4

#define OFFSET_PANELUPDATE 4

#define OFFSET_DEATHNOTICE_PAINT 129 //complete offset 152 (129 for vgui::Panel), linux offset 19
#define OFFSET_DEATHNOTICE_GETCOLOR 16 //linux offset 22

CHooksMan::~CHooksMan()
{
	if (!ms_bHooksInited) //this must be called only once
		return;
	//Destroy all hooks here
	for (auto pTable : m_mHookedVtables) delete pTable.second;
	ms_bHooksInited = false;
}

void CHooksMan::InitHooks()
{
	if (ms_bHooksInited) //wotifok?
		return;

	static CHooksMan m_CWatchDog; //once our dll unloaded this will destroy all hooks made here

	//wait for window
	HWND hWindow = nullptr;
	while (!(hWindow = LI_FN(FindWindowA)(xorstr_("Valve001"), nullptr))) { std::this_thread::sleep_for(std::chrono::milliseconds(20)); }
	ms_hWinProc = (WNDPROC)LI_FN(SetWindowLongA)(hWindow, GWLP_WNDPROC, (LONG)&CHooksMan::OnWndProc);

	//now install hooks

	NSUtils::CVirtualMemberTableMan * pTable = nullptr;

	pTable = new NSUtils::CVirtualMemberTableMan(NSInterfaces::g_pClient);
	VMTHookFunctionAutoUnknown(pTable, &CHooksMan::KeyEvent, int, IBaseClientDLL, IN_KeyEvent, VMT_NOATTRIBS);
	VMTHookFunctionAutoUnknown(pTable, &CHooksMan::DispatchUserMessage, bool, IBaseClientDLL, DispatchUserMessage, VMT_NOATTRIBS);
	VMTHookFunctionAutoUnknown(pTable, &CHooksMan::LevelInitPostEntity, void, IBaseClientDLL, LevelInitPostEntity, VMT_NOATTRIBS);
	VMTHookFunctionAutoUnknown(pTable, &CHooksMan::LevelShutdown, void, IBaseClientDLL, LevelShutdown, VMT_NOATTRIBS);
	m_mHookedVtables[VMT_CLIENT] = pTable;

	pTable = new NSUtils::CVirtualMemberTableMan(NSInterfaces::g_pEngineClient);
	VMTHookFunctionAutoUnknown(pTable, &CHooksMan::ClientCmd, void, IVEngineClient, ClientCmd, VMT_NOATTRIBS);
	VMTHookFunctionAutoUnknown(pTable, &CHooksMan::ClientCmd_Unrestricted, void, IVEngineClient, ClientCmd_Unrestricted, VMT_NOATTRIBS);
	VMTHookFunctionAutoUnknown(pTable, &CHooksMan::ExecuteClientCmd, void, IVEngineClient, ExecuteClientCmd, VMT_NOATTRIBS);
	m_mHookedVtables[VMT_CLIENTENGINE] = pTable;
	
	pTable = new NSUtils::CVirtualMemberTableMan(NSInterfaces::g_pClientModeShared);
	VMTHookFunctionAutoUnknown(pTable, &CHooksMan::CreateMove, bool, IClientMode, CreateMove, VMT_NOATTRIBS);
	m_mHookedVtables[VMT_CLIENTMODE] = pTable;
	
	pTable = new NSUtils::CVirtualMemberTableMan(NSInterfaces::g_pVGUIPanel);
	VMTHookFunctionAutoUnknown(pTable, &CHooksMan::PaintTraverse, void, vgui::IPanel, PaintTraverse, VMT_NOATTRIBS);
	m_mHookedVtables[VMT_PANEL] = pTable;
	if (!!NSInterfaces::g_pViewPortInterface)
		InitViewportHooks();
	
	void * pHudDeathNotice = NSInterfaces::g_pHud->FindElement(xorstr_("CTFHudDeathNotice"));

	pTable = new NSUtils::CVirtualMemberTableMan(pHudDeathNotice);
	pTable->HookFunction(&CHooksMan::OnDeathNoticeGetTeamColor, OFFSET_DEATHNOTICE_GETCOLOR);
	m_mHookedVtables[VMT_DEATHNOTICE] = pTable;

	void * pCastedToPanel = NSUtils::CVirtualMemberTableMan::DoDynamicCast((void ***)pHudDeathNotice, xorstr_("vgui::Panel"));

	pTable = new NSUtils::CVirtualMemberTableMan(pCastedToPanel);
	pTable->HookFunction(&CHooksMan::OnDeathNoticePaint, OFFSET_DEATHNOTICE_PAINT);
	m_mHookedVtables[VMT_DEATHNOTICE_PANEL] = pTable;

	ms_bHooksInited = true;
}

void CHooksMan::InitViewportHooks()
{
	if (m_mHookedVtables.find(VMT_VIEWPORT) != m_mHookedVtables.end())
		return;
	
	//now install hooks

	NSUtils::CVirtualMemberTableMan * pTable = new NSUtils::CVirtualMemberTableMan(NSInterfaces::g_pViewPortInterface);
	//pTable->HookFunction(&CHooksMan::ShowPanel1, OFFSET_SHOWPANEL1); //this actually called from ShowPanel2
	pTable->HookFunction(&CHooksMan::ShowPanel2, OFFSET_SHOWPANEL2);
	pTable->HookFunction(&CHooksMan::FindPanelByName, OFFSET_FINDPANEL);
	m_mHookedVtables[VMT_VIEWPORT] = pTable;
}

void CHooksMan::ShowPanel1(IViewPort * pThis, void * pDummyArg, const char * pName, bool bState)
{
	static auto pOriginalFunc = m_mHookedVtables[VMT_VIEWPORT]->GetOriginalFunction<decltype(&CHooksMan::ShowPanel1)>(OFFSET_SHOWPANEL1);

	if (!strcmp(pName, xorstr_("scores")))
	{
		//let hook in FindPanelByName() do this
		NSInterfaces::g_pViewPortInterface->FindPanelByName(pName);
	}
	
	pOriginalFunc(pThis, pDummyArg, pName, bState);
}

void CHooksMan::ShowPanel2(IViewPort * pThis, void * pDummyArg, IViewPortPanel * pPanel, bool bState)
{
	static auto pOriginalFunc = m_mHookedVtables[VMT_VIEWPORT]->GetOriginalFunction<decltype(&CHooksMan::ShowPanel2)>(OFFSET_SHOWPANEL2);

	if (!strcmp(pPanel->GetName(), xorstr_("scores")))
	{
		//let hook in FindPanelByName() do this
		NSInterfaces::g_pViewPortInterface->FindPanelByName(pPanel->GetName());
	}

	pOriginalFunc(pThis, pDummyArg, pPanel, bState);
}

IViewPortPanel * CHooksMan::FindPanelByName(IViewPort * pThis, void * pDummyArg, const char * pPanelName)
{
	static auto pOriginalFunc = m_mHookedVtables[VMT_VIEWPORT]->GetOriginalFunction<decltype(&CHooksMan::FindPanelByName)>(OFFSET_FINDPANEL);

	if (!strcmp(pPanelName, xorstr_("scores")))
	{
		IViewPortPanel * pResult = pOriginalFunc(pThis, pDummyArg, pPanelName);
		//check if our hook installed on correct vtable
		if (!CheckScoreBoardVMT(pResult))
			HookScoreBoard(pResult);
		return pResult;
	}

	return pOriginalFunc(pThis, pDummyArg, pPanelName);
}

void CHooksMan::UpdateScoreBoard(IViewPortPanel * pThis, void * pDumbArg)
{
	//static not needed here!
	auto pOriginalFunc = m_mHookedVtables[VMT_SCOREBOARD]->GetOriginalFunction<decltype(&CHooksMan::UpdateScoreBoard)>(OFFSET_PANELUPDATE);

	//first call original function
	pOriginalFunc(pThis, pDumbArg);

	//now do our stuff
	CCatConnect::OnScoreBoardUpdate(pThis);
}

int CHooksMan::KeyEvent(IBaseClientDLL * pThis, void * pDumbArg, int iCode, int iKey, const char * pBindedTo)
{
	static auto pOriginalFunc = (decltype(&CHooksMan::KeyEvent))VMTGetOriginalFunctionAutoUnknown(m_mHookedVtables[VMT_CLIENT], int, IBaseClientDLL, IN_KeyEvent, VMT_NOATTRIBS);

	/*if (iCode == 1)
	{
		if (KEY_HOME == iKey)
			CCatConnect::Trigger();
	}*/

	return pOriginalFunc(pThis, pDumbArg, iCode, iKey, pBindedTo);
}

bool CHooksMan::CreateMove(IClientMode * pThis, void * pDumbArg, float flInputSample, CUserCmd * pCmd)
{
	static auto pOriginalFunc = (decltype(&CHooksMan::CreateMove))VMTGetOriginalFunctionAutoUnknown(m_mHookedVtables[VMT_CLIENTMODE], bool, IClientMode, CreateMove, VMT_NOATTRIBS);
	NSGlobals::g_pLocalPlayer = (CBasePlayer *)NSInterfaces::g_pClientEntityList->GetClientEntity(NSInterfaces::g_pEngineClient->GetLocalPlayer());
	CCatConnect::CreateMove(flInputSample, pCmd);
	return pOriginalFunc(pThis, pDumbArg, flInputSample, pCmd);
}

void CHooksMan::PaintTraverse(IPanel * pThis, void * pDumbArg, vgui::VPANEL iVGUIPanel, bool bForceRepaint, bool bAllowForce)
{
	static auto pOriginalFunc = (decltype(&CHooksMan::PaintTraverse))VMTGetOriginalFunctionAutoUnknown(m_mHookedVtables[VMT_PANEL], void, vgui::IPanel, PaintTraverse, VMT_NOATTRIBS);

	if (NSGlobals::g_bCatConnectInited)
	{
		//also update timers here
		g_CTimerMan.Update();
	}

	pOriginalFunc(pThis, pDumbArg, iVGUIPanel, bForceRepaint, bAllowForce);

	static vgui::VPANEL iFocusOverlayPanel = 0;

	if (!iFocusOverlayPanel)
	{
		const char * pName = NSInterfaces::g_pVGUIPanel->GetName(iVGUIPanel);
		if (pName && !strncmp(pName, xorstr_("FocusOverlayPanel"), 17))
		{
			iFocusOverlayPanel = iVGUIPanel;
			//init it here
			CCatConnect::Init();
		}
	}

	if (iFocusOverlayPanel == iVGUIPanel)
	{
		NSInterfaces::g_pVGUIPanel->SetTopmostPopup(iFocusOverlayPanel, true);
		int iTempW, iTempH;
		NSInterfaces::g_pEngineClient->GetScreenSize(iTempW, iTempH);
		if (iTempW != NSCore::NSDrawUtils::g_iScreenWidth || iTempH != NSCore::NSDrawUtils::g_iScreenHeight) //don't ask I have autism
		{
			NSCore::NSDrawUtils::g_iScreenWidth = iTempW;
			NSCore::NSDrawUtils::g_iScreenHeight = iTempH;
			NSCore::CDrawer::Reload();
		}
		int iCode = 0;
		ButtonCode_t iKey = BUTTON_CODE_NONE;
		GetLastPressedKey(iCode, iKey);
		if (iCode == 1)
		{
			if (iKey == KEY_HOME)
				NSMenu::g_CMenu.ToggleEnabled();
		}
		NSMenu::g_CMenu.GetInput(iKey);
		NSMenu::g_CMenu.Draw();
		NSInterfaces::g_pVGUIPanel->SetMouseInputEnabled(iFocusOverlayPanel, NSMenu::g_CMenu.IsEnabled());
		NSMenu::g_CMenu.EndInput();
	}
}

bool CHooksMan::DispatchUserMessage(IBaseClientDLL * pThis, void * pDumbArg, int iMessageID, old_bf_read * pBFMessage)
{
	static auto pOriginalFunc = (decltype(&CHooksMan::DispatchUserMessage))VMTGetOriginalFunctionAutoUnknown(m_mHookedVtables[VMT_CLIENT], void, IBaseClientDLL, DispatchUserMessage, VMT_NOATTRIBS);

	if (!pBFMessage || pBFMessage->IsOverflowed())
		return pOriginalFunc(pThis, pDumbArg, iMessageID, pBFMessage);

	bool bCallOriginal = true;

	switch (iMessageID)
	{
		case 46:
		{
			int iTeam = pBFMessage->ReadByte();
			int iCaller = pBFMessage->ReadByte();
			char cReason[64], cDummyName[64];
			pBFMessage->ReadString(cReason, 64, false, nullptr);
			pBFMessage->ReadString(cDummyName, 64, false, nullptr);
			int iTarget = (int)(((unsigned char)pBFMessage->ReadByte()) >> 1);
			CCatConnect::OnPotentialVoteKickStarted(iTeam, iCaller, iTarget, cReason);
			pBFMessage->Seek(0);
			break;
		}
		case 47:
		case 48:
		{
			CCatConnect::SetVoteState();
			break;
		}
		case 3: //SayText
		case 4: //SayText2
		{
			if (!remove_newlines.GetBool())
				break;

			int iClient = pBFMessage->ReadByte();
			bool bWantsToChat = true;
			if (iMessageID == 4)
				bWantsToChat = !!pBFMessage->ReadByte();

			char cString[2048];
			char cAdditionalBufs[4][256]{ {0}, {0}, {0}, {0} };

			pBFMessage->ReadString(cString, sizeof(cString));
			std::string sNewString = CCatConnect::OnChatMessage(iClient, 0, cString);

			if (remove_newlines.GetInt() == 2 && !sNewString.length())
			{
				pBFMessage->Seek(0);
				bCallOriginal = false;
				break;
			}

			if (iMessageID == 4)
			{
				for (int i = 0; i < 4; i++)
				{
					pBFMessage->ReadString(cAdditionalBufs[i], sizeof(cAdditionalBufs[0]));
					if (cAdditionalBufs[i][0])
					{
						std::string sNewMessage = CCatConnect::OnChatMessage(iClient, 0, cAdditionalBufs[i]);
						strcpy(cAdditionalBufs[i], sNewMessage.data());
						//0 - nickname, 1 - message
						if (i == 1)
						{
							//NSUtils::CLogger::Log(NSUtils::Log_Debug, xorstr_("Message (len %i): %s"), sNewMessage.length(), sNewMessage.c_str());
							if (remove_newlines.GetInt() == 2 && !sNewMessage.length())
							{
								pBFMessage->Seek(0);
								bCallOriginal = false;
								break;
							}
						}
					}
				}
				if (!bCallOriginal)
					break;
			}

			auto * pBFWrite = (old_bf_write *)pBFMessage;
			pBFWrite->Reset();
			pBFWrite->WriteByte(iClient);
			if (iMessageID == 4)
				pBFWrite->WriteByte(bWantsToChat);
			pBFWrite->WriteString(cString);

			if (iMessageID == 4)
				for (int i = 0; i < 4; i++)
					if (cAdditionalBufs[i][0])
						pBFWrite->WriteString(cAdditionalBufs[i]);

			pBFMessage->m_nDataBits = pBFWrite->GetNumBitsWritten();
			pBFMessage->m_nDataBytes = pBFWrite->GetNumBytesWritten();
			pBFMessage->Seek(0);

			break;
		}
		default:
			break;
	}

	if (bCallOriginal)
		return pOriginalFunc(pThis, pDumbArg, iMessageID, pBFMessage);
	return true;
}

void CHooksMan::LevelInitPostEntity(IBaseClientDLL * pThis, void * pDumbArg)
{
	static auto pOriginalFunc = (decltype(&CHooksMan::LevelInitPostEntity))VMTGetOriginalFunctionAutoUnknown(m_mHookedVtables[VMT_CLIENT], void, IBaseClientDLL, LevelInitPostEntity, VMT_NOATTRIBS);
	pOriginalFunc(pThis, pDumbArg);
	CCatConnect::OnMapStart();
}

void CHooksMan::LevelShutdown(IBaseClientDLL * pThis, void * pDumbArg)
{
	static auto pOriginalFunc = (decltype(&CHooksMan::LevelShutdown))VMTGetOriginalFunctionAutoUnknown(m_mHookedVtables[VMT_CLIENT], void, IBaseClientDLL, LevelShutdown, VMT_NOATTRIBS);
	CCatConnect::OnMapEnd();
	pOriginalFunc(pThis, pDumbArg);
}

void CHooksMan::ClientCmd(IVEngineClient * pThis, void * pDummyArg, const char * pCmd)
{
	static auto pOriginalFunc = (decltype(&CHooksMan::ClientCmd))VMTGetOriginalFunctionAutoUnknown(m_mHookedVtables[VMT_CLIENTENGINE], void, IVEngineClient, ClientCmd, VMT_NOATTRIBS);
	if (!CCatConnect::OnClientCommand(pCmd))
		pOriginalFunc(pThis, pDummyArg, pCmd);
}

void CHooksMan::ClientCmd_Unrestricted(IVEngineClient * pThis, void * pDummyArg, const char * pCmd)
{
	static auto pOriginalFunc = (decltype(&CHooksMan::ClientCmd_Unrestricted))VMTGetOriginalFunctionAutoUnknown(m_mHookedVtables[VMT_CLIENTENGINE], void, IVEngineClient, ClientCmd_Unrestricted, VMT_NOATTRIBS);
	if (!CCatConnect::OnClientCommand(pCmd))
		pOriginalFunc(pThis, pDummyArg, pCmd);
}

void CHooksMan::ExecuteClientCmd(IVEngineClient * pThis, void * pDummyArg, const char * pCmd)
{
	static auto pOriginalFunc = (decltype(&CHooksMan::ExecuteClientCmd))VMTGetOriginalFunctionAutoUnknown(m_mHookedVtables[VMT_CLIENTENGINE], void, IVEngineClient, ExecuteClientCmd, VMT_NOATTRIBS);
	if (!CCatConnect::OnClientCommand(pCmd))
		pOriginalFunc(pThis, pDummyArg, pCmd);
}

void CHooksMan::OnDeathNoticePaint(void * pThis, void * pDumbArg)
{
	auto pOriginalFunc = m_mHookedVtables[VMT_DEATHNOTICE_PANEL]->GetOriginalFunction<decltype(&CHooksMan::OnDeathNoticePaint)>(OFFSET_DEATHNOTICE_PAINT);
	CCatConnect::OnDeathNoticePaintPre(pThis);
	return pOriginalFunc(pThis, pDumbArg);
}

Color * CHooksMan::OnDeathNoticeGetTeamColor(void * pThis, void * pDumbArg, Color * pClr, int iTeam, bool bLocalPlayer) //I love msvc optimizations, thanks for __userpurge and useless Color argument
{
	auto pOriginalFunc = m_mHookedVtables[VMT_DEATHNOTICE]->GetOriginalFunction<decltype(&CHooksMan::OnDeathNoticeGetTeamColor)>(OFFSET_DEATHNOTICE_GETCOLOR);
	static Color cClr;
	cClr = CCatConnect::GetDeathNoticeColorFromStack();
	if (cClr.a())
	{
		*pClr = cClr;
		return &cClr;
	}
	pOriginalFunc(pThis, pDumbArg, &cClr, iTeam, bLocalPlayer);
	*pClr = cClr;
	return &cClr;
}

LRESULT CHooksMan::OnWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_KEYDOWN)
	{
		if (wParam <= 255)
		{
			ButtonCode_t iKey = NSInterfaces::g_pInputSystem->VirtualKeyToButtonCode(wParam);
			if (ms_iLastKeyPressed != iKey)
				ms_iKeyState = 1;
			ms_iLastKeyPressed = iKey;
		}
		else
		{
			ms_iLastKeyPressed = BUTTON_CODE_NONE;
			ms_iKeyState = 0;
		}
	}
	else if (uMsg == WM_KEYUP)
	{
		if (wParam <= 255)
		{
			ButtonCode_t iKey = NSInterfaces::g_pInputSystem->VirtualKeyToButtonCode(wParam);
			if (ms_iLastKeyPressed == iKey)
			{
				ms_iKeyState = 0;
				ms_iLastKeyPressed = BUTTON_CODE_NONE;
			}
		}
	}

	return LI_FN(CallWindowProcA)(ms_hWinProc, hWnd, uMsg, wParam, lParam);
}

bool CHooksMan::CheckScoreBoardVMT(void * pThis)
{
	if (m_mHookedVtables.find(VMT_SCOREBOARD) == m_mHookedVtables.end())
		return false;
	auto pTable = m_mHookedVtables[VMT_SCOREBOARD];
	if ((void ***)pThis == pTable->GetThis())
		return true;
	pTable->DestroyNoRestore();
	m_mHookedVtables.erase(VMT_SCOREBOARD);
	return false;
}

void CHooksMan::HookScoreBoard(void * pThis)
{
	if (!pThis) return;
	auto pTable = new NSUtils::CVirtualMemberTableMan(pThis);
	pTable->HookFunction(&UpdateScoreBoard, OFFSET_PANELUPDATE);
	m_mHookedVtables[VMT_SCOREBOARD] = pTable;
}

void CHooksMan::GetLastPressedKey(int &iCode, ButtonCode_t &iKey)
{
	int iKeyState = ms_iKeyState;
	if (ms_iKeyState == 1)
		ms_iKeyState = 2;
	if (!iKeyState)
		iKey = BUTTON_CODE_NONE;
	else
		iKey = ms_iLastKeyPressed;
	iCode = iKeyState;
}

bool CHooksMan::ms_bHooksInited = false;
std::map<VMTHookNum, NSUtils::CVirtualMemberTableMan *> CHooksMan::m_mHookedVtables;
WNDPROC CHooksMan::ms_hWinProc = nullptr;
ButtonCode_t CHooksMan::ms_iLastKeyPressed = BUTTON_CODE_NONE;
int CHooksMan::ms_iKeyState = 0;