#include "interfaces.h"
#include "interface.h"
#include "vmt.h"
#include "netvars.h"
#include "ctfplayerresource.h"
#include "isteamclient.h"
#include "isteamfriends.h"
#include "isteamuser.h"
#include "public/imaterialsystemfixed.h"
#include "materialsystem/imaterialsystem.h"
#include "igameevents.h"
#include "cdll_int.h"
#include "iclientmode.h"
#include "icliententitylist.h"
#include "ienginevgui.h"
#include "ivrenderview.h"
#include "ivmodelrender.h"
#include "inputsystem/iinputsystem.h"
#include "filesystem.h"
#include "e8call.h"
#include "settings/settings.h"
#include "vgui/ISurfaceV30.h"
#include "vgui/ipanel.h"
#include "catfiles.h"
#include "utils/hud.h" //explicit include, we must use utils/ here
#include "lazy.h"
#include "xorstr.h"
#include <thread>
#include <chrono>

//these defined in glow.cpp!
class IScreenSpaceEffectManager;
class CScreenSpaceEffectRegistration;

extern IScreenSpaceEffectManager * g_pScreenSpaceEffects;
extern CScreenSpaceEffectRegistration ** g_ppScreenSpaceRegistrationHead;

#pragma comment (lib, "Kernel32.lib")

#define WAIT_FOR_SIMPLE_INIT(object, tempvar) while (!(tempvar = (object))) { std::this_thread::sleep_for(std::chrono::milliseconds(20)); }

void FASTERCALL NSInterfaces::WaitForModule(const char * pModuleName)
{
	while (LI_FN(GetModuleHandleA)(pModuleName) == nullptr) { std::this_thread::sleep_for(std::chrono::milliseconds(20)); }
}

void * FASTERCALL NSInterfaces::WaitForExternal(const char * pModuleName, const char * pExternalName)
{
	volatile void * pRet = nullptr; //we use volatile to prevent aggressive compiler optimization
	while (!(pRet = LI_FN(GetProcAddress)(LI_FN(GetModuleHandleA)(pModuleName), pExternalName))) { std::this_thread::sleep_for(std::chrono::milliseconds(20)); }
	return const_cast<void *>(pRet);
}

void * FASTERCALL NSInterfaces::WaitForInterface(CreateInterfaceFn pFactory, const char * pInterfaceName)
{
	volatile void * pRet = nullptr;
	while (!pRet)
	{
		void * pPotentialIFace = pFactory(pInterfaceName, nullptr);
		pRet = WaitForObjectAllocationAndInitialization(&pPotentialIFace, true);
		if (!pRet)
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
	return const_cast<void *>(pRet);
}

void * FASTERCALL NSInterfaces::WaitForObjectAllocationAndInitialization(void ** pObject, bool bRTTIObject)
{
	volatile void * pRet = nullptr;
	while (!pRet)
	{
		__try { pRet = *pObject; }
		__except (EXCEPTION_EXECUTE_HANDLER) { /*continue;*/ }
		//if this check passed, then check memory pointer points to for being valid

		if (!pRet) continue; //lmao

		MEMORY_BASIC_INFORMATION sInfo;
		SIZE_T iSize = LI_FN(VirtualQuery)(const_cast<void *>(pRet), &sInfo, sizeof(MEMORY_BASIC_INFORMATION));

		if (!iSize || (sInfo.Protect & (PAGE_NOACCESS | PAGE_GUARD)) || (sInfo.State & MEM_FREE))
		{
			//this is bad pointer!
			pRet = nullptr;
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			continue;
		}

		if (bRTTIObject)
		{
			//if this is RTTI object, then we can check vtable and rtti information
			volatile void ** vTable = nullptr;
			__try { vTable = *(volatile void ***)pRet; }
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				//oke, pointer is bad
				pRet = nullptr;
				std::this_thread::sleep_for(std::chrono::milliseconds(20));
				continue;
			}

			if (vTable)
			{
				//now check rtti info
				const char * pClass = NSUtils::CVirtualMemberTableMan::GetRTTIClass(const_cast<void ***>((volatile void ***)pRet));
				if (!pClass || !*pClass)
				{
					//no class on rtti object?
					//this is bad pointer
					pRet = nullptr;
				}
			}
			else
				pRet = nullptr;
		}

		if (!pRet)
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
	return const_cast<void *>(pRet);
}

void NSInterfaces::InitInterfaces()
{
	CreateInterfaceFn pClientFactory = nullptr;
	CreateInterfaceFn pEngineFactory = nullptr;
	CreateInterfaceFn pVGUIFactory = nullptr;
	CreateInterfaceFn pMatSurfaceFactory = nullptr;
	CreateInterfaceFn pValveSTDLibFactory = nullptr;
	CreateInterfaceFn pInputSysFactory = nullptr;
	CreateInterfaceFn pMatSystemFactory = nullptr;
	CreateInterfaceFn pFileSystemFactory = nullptr;
	{
		WaitForModule(xorstr_("client.dll"));
		WaitForModule(xorstr_("engine.dll"));
		WaitForModule(xorstr_("vgui2.dll"));
		WaitForModule(xorstr_("vstdlib.dll"));
		WaitForModule(xorstr_("steam_api.dll"));
		WaitForModule(xorstr_("steamclient.dll"));
		WaitForModule(xorstr_("vguimatsurface.dll"));
		WaitForModule(xorstr_("inputsystem.dll"));
		WaitForModule(xorstr_("FileSystem_Stdio.dll"));
		WaitForModule(xorstr_("MaterialSystem.dll"));
		WaitForModule(xorstr_("GameUI.dll"));
	}
	{
		auto sCrIface = xorstr("CreateInterface");
		pClientFactory = (CreateInterfaceFn)(WaitForExternal(xorstr_("client.dll"), sCrIface.crypt_get()));
		pEngineFactory = (CreateInterfaceFn)(WaitForExternal(xorstr_("engine.dll"), sCrIface.crypt_get()));
		pVGUIFactory = (CreateInterfaceFn)(WaitForExternal(xorstr_("vgui2.dll"), sCrIface.crypt_get()));
		pMatSurfaceFactory = (CreateInterfaceFn)(WaitForExternal(xorstr_("vguimatsurface.dll"), sCrIface.crypt_get()));
		pValveSTDLibFactory = (CreateInterfaceFn)(WaitForExternal(xorstr_("vstdlib.dll"), sCrIface.crypt_get()));
		pInputSysFactory = (CreateInterfaceFn)(WaitForExternal(xorstr_("inputsystem.dll"), sCrIface.crypt_get()));
		pMatSystemFactory = (CreateInterfaceFn)(WaitForExternal(xorstr_("MaterialSystem.dll"), sCrIface.crypt_get()));
		pFileSystemFactory = (CreateInterfaceFn)(WaitForExternal(xorstr_("FileSystem_Stdio.dll"), sCrIface.crypt_get()));
	}
	
	g_pClient = (IBaseClientDLL *)(WaitForInterface(pClientFactory, xorstr_(CLIENT_DLL_INTERFACE_VERSION)));
	g_pClientEntityList = (IClientEntityList *)(WaitForInterface(pClientFactory, xorstr_(VCLIENTENTITYLIST_INTERFACE_VERSION)));

	g_pEngineClient = (IVEngineClient *)(WaitForInterface(pEngineFactory, xorstr_(VENGINE_CLIENT_INTERFACE_VERSION)));
	g_pGameEventsMan2 = (IGameEventManager2 *)(WaitForInterface(pEngineFactory, xorstr_(INTERFACEVERSION_GAMEEVENTSMANAGER2)));
	g_pEngineVGUI = (IEngineVGui *)(WaitForInterface(pEngineFactory, xorstr_(VENGINE_VGUI_VERSION)));
	g_pVGUIPanel = (vgui::IPanel *)(WaitForInterface(pVGUIFactory, xorstr_(VGUI_PANEL_INTERFACE_VERSION)));
	g_pSurface = (SurfaceV30::ISurfaceFixed *)(WaitForInterface(pMatSurfaceFactory, xorstr_(VGUI_SURFACE_INTERFACE_VERSION_30)));
	g_pCVar = (ICvar *)(WaitForInterface(pValveSTDLibFactory, xorstr_(CVAR_INTERFACE_VERSION)));
	g_pInputSystem = (IInputSystem *)(WaitForInterface(pInputSysFactory, xorstr_(INPUTSYSTEM_INTERFACE_VERSION)));
	g_pMaterialSystemFixed = (IMaterialSystemFixed *)(WaitForInterface(pMatSystemFactory, xorstr_(MATERIAL_SYSTEM_INTERFACE_VERSION_81)));
	g_pRenderView = (IVRenderView *)(WaitForInterface(pEngineFactory, xorstr_(VENGINE_RENDERVIEW_INTERFACE_VERSION)));
	g_pModelRender = (IVModelRender *)(WaitForInterface(pEngineFactory, xorstr_(VENGINE_HUDMODEL_INTERFACE_VERSION)));
	g_pFileSystem = (IFileSystem *)(WaitForInterface(pFileSystemFactory, xorstr_(FILESYSTEM_INTERFACE_VERSION)));
	
	g_pClientModeShared = (IClientMode *)WaitForObjectAllocationAndInitialization((void **)*(IClientMode ***)((*(int **)g_pClient)[10] + 0x5), true); //probably get it from signature?
	g_pHud = (NSReclass::CHud *)WaitForObjectAllocationAndInitialization((void **)(NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x8B\x01\x8B\x40\x24\xFF\xD0\x8B\xC8\xE8\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xB9\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x85\xC0"), 31) + 20), false);
	g_pGlobals = (CGlobalVarsBase *)WaitForObjectAllocationAndInitialization((void **)(NSUtils::Sigscan(xorstr_("engine.dll"), xorstr_("\xA1\x2A\x2A\x2A\x2A\x8B\x11\x68"), 8) + 8), false);

	bool bDummy = false;
	WAIT_FOR_SIMPLE_INIT(g_pHud->m_bHudTexturesLoaded, bDummy); //wait for hud init
	void * pDummy = nullptr;
	WAIT_FOR_SIMPLE_INIT(g_pHud->FindElement(xorstr_("CTFHudDeathNotice")), pDummy);
	
	//WAIT_FOR_SIMPLE_INIT(((CUserMessages * (__cdecl*)())NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x83\x3D\x2A\x2A\x2A\x2A\x00\x75\x2A\x6A\x24\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04"), 19))(), g_pUserMessages);
	g_pUserMessages = (CUserMessages *)WaitForObjectAllocationAndInitialization((void **)(NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x83\x3D\x2A\x2A\x2A\x2A\x00\x75\x2A\x6A\x24\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04"), 19) + 2), false); //this signature also used in reclass
	
	//wait until client's hud will be fully initialized
	void * pTFViewPort = nullptr;
	WAIT_FOR_SIMPLE_INIT(g_pClientModeShared->GetViewport(), pTFViewPort);
	NSInterfaces::g_pViewPortInterface = (IViewPort *)NSUtils::CVirtualMemberTableMan::FindInterfaceThisPointer((void ***)pTFViewPort, xorstr_("IViewPort")); //This is same as: NSInterfaces::g_pViewPortInterface = (IViewPort *)((char *)pTFViewPort + 396); where 396 (388 in linux) offset of IViewPort vtable in bytes. But we get this dynamically so we don't care about these offsets

	g_pScreenSpaceEffects = (IScreenSpaceEffectManager *)WaitForObjectAllocationAndInitialization(*(void ***)(NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x55\x8B\xEC\x53\x8B\xD9\x8B\x03\xFF\x50\x08\x8B\x4D\x08\x3B"
																																								"\x48\x14\x74\x2A\xFF\x75\x0C\x51\x8B\xCB\xE8\x2A\x2A\x2A\x2A"
																																								"\x5B\x5D\xC2\x08\x00\x8B\x4D\x0C\x56\x8B\x51\x08\x8B\xC2\x8B"
																																								"\x71\x0C\x2B\xC6\x83\xF8\x08\x7D\x2A\x89\x51\x0C\xE8\x2A\x2A"
																																								"\x2A\x2A\x33\xC0\xEB\x2A\x8B\x41\x0C\x57\x8B\xFE\xC1\xFE\x05"
																																								"\x83\xE7\x1F\x8D\x50\x07\x83\xC0\x08\x89\x41\x0C\x8B\x01\xB9"
																																								"\x20\x00\x00\x00\xC1\xFA\x05\x2B\xCF\x8B\x14\x90\x8B\x04\xB0"
																																								"\xD3\xE2\x8B\xCF\xD3\xE8\x0B\xD0\x0F\xB6\xC2\x5F\x83\xE8\x00"), 120) + 247), true);
	g_ppScreenSpaceRegistrationHead = (CScreenSpaceEffectRegistration **)WaitForObjectAllocationAndInitialization((void **)(NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x56\x8B\x35\x2A\x2A\x2A\x2A\x85\xF6\x74\x2A\xEB\x2A\x8D\x49\x00\x8B\x4E\x04\x85\xC9\x74\x2A\x8B\x01\x6A\x00"), 27) + 3), false);

	//now load steamclient and init it

	typedef ISteamClient *(__cdecl * GetSteamClientFn)();
	WAIT_FOR_SIMPLE_INIT(((GetSteamClientFn)e8call(NSUtils::Sigscan(xorstr_("engine.dll"), xorstr_("\x55\x8B\xEC\x83\xEC\x30\x8D\x4D\xF8\x56\x8B\x75\x08\x56\xE8\x2A\x2A\x2A\x2A\x80\x3D\x2A\x2A\x2A\x2A\x00\x0F\x85\x2A\x2A\x2A\x2A\xC6\x05\x2A\x2A\x2A\x2A\x01\xE8"), 40) + 40))(), g_pSteamClient);
	g_pSteamFriends = (ISteamFriends *)WaitForObjectAllocationAndInitialization((void **)((char *)g_pSteamClient + 8), true);
	g_pSteamClient = nullptr;
	WAIT_FOR_SIMPLE_INIT((ISteamClient *)LI_FN(SteamInternal_CreateInterface).in(LI_MODULE("steam_api.dll").cached())(xorstr_(STEAMCLIENT_INTERFACE_VERSION)), g_pSteamClient);
	HSteamUser hUser = LI_FN(SteamAPI_GetHSteamUser).in(LI_MODULE("steam_api.dll").cached())();
	WAIT_FOR_SIMPLE_INIT((ISteamUser *)LI_FN(SteamInternal_FindOrCreateUserInterface).in(LI_MODULE("steam_api.dll").cached())(hUser, xorstr_(STEAMUSER_INTERFACE_VERSION)), g_pSteamUser);

	//also init some globals here
	//load saved data
	CCatFiles::LoadData();

	//init this
	NSUtils::g_CNetvars.Init();
	NSCore::CSettingsCollector::Init();
	NSReclass::g_pCTFPlayerResource = new NSReclass::CTFPlayerResource();
}

void NSInterfaces::Destroy()
{
	delete NSReclass::g_pCTFPlayerResource;
}

IBaseClientDLL * NSInterfaces::g_pClient = nullptr;
IClientEntityList * NSInterfaces::g_pClientEntityList = nullptr;
IClientMode * NSInterfaces::g_pClientModeShared = nullptr;
IVEngineClient * NSInterfaces::g_pEngineClient = nullptr;
vgui::IPanel * NSInterfaces::g_pVGUIPanel = nullptr;
IGameEventManager2 * NSInterfaces::g_pGameEventsMan2 = nullptr;
IEngineVGui * NSInterfaces::g_pEngineVGUI = nullptr;
ICvar * NSInterfaces::g_pCVar = nullptr;
IInputSystem * NSInterfaces::g_pInputSystem = nullptr;
NSReclass::CHud * NSInterfaces::g_pHud = nullptr;
CUserMessages * NSInterfaces::g_pUserMessages = nullptr;
IViewPort * NSInterfaces::g_pViewPortInterface = nullptr;
ISteamClient * NSInterfaces::g_pSteamClient = nullptr;
ISteamFriends * NSInterfaces::g_pSteamFriends = nullptr;
ISteamUser * NSInterfaces::g_pSteamUser = nullptr;
SurfaceV30::ISurfaceFixed * NSInterfaces::g_pSurface = nullptr;
IMaterialSystemFixed * NSInterfaces::g_pMaterialSystemFixed = nullptr;
IVRenderView * NSInterfaces::g_pRenderView = nullptr;
IVModelRender * NSInterfaces::g_pModelRender = nullptr;
IFileSystem * NSInterfaces::g_pFileSystem = nullptr;
CGlobalVarsBase * NSInterfaces::g_pGlobals = nullptr;