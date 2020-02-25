#ifndef __HOOKS_MAN_INC_
#define __HOOKS_MAN_INC_

#include <Windows.h>
#include <map>
#include <stdint.h>
#include "vmt.h"
#include "defs.h"
#include "inputsystem/buttoncode.h"
#include "filesystem.h"

class CUserCmd;
class IBaseClientDLL;
class IVEngineClient;
class IClientMode;
class IPanel;
class IViewPort;
class IViewPortPanel;
class CViewSetup;
class CHudBaseChat;
class IFileSystem;
class old_bf_read;

enum VMTHookNum : uint8_t
{
	VMT_CLIENT = 0,
	VMT_CLIENTENGINE,
	VMT_CLIENTMODE,
	VMT_PANEL,
	VMT_VIEWPORT,
	VMT_SCOREBOARD,
	VMT_DEATHNOTICE,
	VMT_DEATHNOTICE_PANEL,
	VMT_FILESYSTEM
};

namespace vgui { typedef unsigned int VPANEL; }

//this class manages only global and commonly used hooks!
class CHooksMan
{
	//constructor and destructor are private
	CHooksMan() {}
	~CHooksMan();

public:
	//THIS MUST BE CALLED ONLY AFTER INTERFACES INIT!
	static void InitHooks();
	static void InitViewportHooks();
	inline static NSUtils::CVirtualMemberTableMan * GetVMT(VMTHookNum iNum) { if (!ms_bHooksInited || m_mHookedVtables.find(iNum) == m_mHookedVtables.end()) return nullptr; return m_mHookedVtables[iNum]; }

private:

	//this actually __thiscall function, but we can't use __thiscall on static member functions! So we will use __fastcall instead cuz it very similar to __thiscall
	//https://docs.microsoft.com/en-us/cpp/cpp/thiscall?view=vs-2019
	static int __fastcall KeyEvent(IBaseClientDLL * pThis, void * pDumbArg, int iCode, int iKey, const char * pBindedTo);
	static bool __fastcall CreateMove(IClientMode * pThis, void * pDumbArg, float flInputSample, CUserCmd * pCmd);
	static void __fastcall LevelInitPostEntity(IBaseClientDLL * pThis, void * pDumbArg);
	static void __fastcall LevelShutdown(IBaseClientDLL * pThis, void * pDumbArg);
	static void __fastcall PaintTraverse(IPanel * pThis, void * pDumbArg, vgui::VPANEL iVGUIPanel, bool bForceRepaint, bool bAllowForce);
	static void __fastcall ShowPanel1(IViewPort * pThis, void * pDummyArg, const char * pName, bool bState);
	static void __fastcall ShowPanel2(IViewPort * pThis, void * pDummyArg, IViewPortPanel * pPanel, bool bState);
	static IViewPortPanel * __fastcall FindPanelByName(IViewPort * pThis, void * pDummyArg, const char * pPanelName);
	static bool FASTERCALL CheckScoreBoardVMT(void * pThis);
	static void FASTERCALL HookScoreBoard(void * pThis);
	static void FASTERCALL GetLastPressedKey(int &iCode, ButtonCode_t &iKey);
	static void __fastcall UpdateScoreBoard(IViewPortPanel * pThis, void * pDumbArg);
	static void __fastcall ClientCmd(IVEngineClient * pThis, void * pDummyArg, const char * pCmd);
	static void __fastcall ClientCmd_Unrestricted(IVEngineClient * pThis, void * pDummyArg, const char * pCmd);
	static void __fastcall ExecuteClientCmd(IVEngineClient * pThis, void * pDummyArg, const char * pCmd);
	static bool __fastcall DispatchUserMessage(IBaseClientDLL * pThis, void * pDumbArg, int iMessageID, old_bf_read * pBFMessage);
	static void __fastcall OnDeathNoticePaint(void * pThis, void * pDumbArg);
	static Color * __fastcall OnDeathNoticeGetTeamColor(void * pThis, void * pDumbArg, Color * pClr, int iTeam, bool bLocalPlayer);
	static bool __fastcall IsPlayingDemo(IClientMode * pThis, void * pDumbArg);
	static bool __fastcall DoPostScreenSpaceEffects(IClientMode * pThis, void * pDumbArg, const CViewSetup * pSetup);
	static LRESULT __stdcall OnWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	static bool ms_bHooksInited;
	static std::map<VMTHookNum, NSUtils::CVirtualMemberTableMan *> m_mHookedVtables; //we will use this instead of std::unique_ptr cuz it more compact
	static WNDPROC ms_hWinProc;
	static ButtonCode_t ms_iLastKeyPressed;
	static int ms_iKeyState;
};

#endif