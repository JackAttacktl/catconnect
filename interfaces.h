#ifndef __IFACES_INC_
#define __IFACES_INC_

#include "public/globalvars_base.h"
#include "cdll_int.h"
#include "defs.h"

class IBaseClientDLL;
class IClientEntityList;
class IClientMode;
class IGameEventManager2;
namespace vgui { class IPanel; }
class IEngineVGui;
class ICvar;
namespace NSReclass { class CHud; }
class CUserMessages;
class IViewPort;
class ISteamClient;
class ISteamFriends;
class IInputSystem;
namespace SurfaceV30 { class ISurfaceFixed; }
class IMaterialSystemFixed;
class IVRenderView;
class IVModelRender;

namespace NSInterfaces
{
	extern IBaseClientDLL * g_pClient;
	extern IClientEntityList * g_pClientEntityList;
	extern IClientMode * g_pClientModeShared;

	extern IVEngineClient * g_pEngineClient;
	extern IGameEventManager2 * g_pGameEventsMan2;
	extern vgui::IPanel * g_pVGUIPanel;
	extern IEngineVGui * g_pEngineVGUI;
	extern ICvar * g_pCVar;
	extern IInputSystem * g_pInputSystem;
	extern NSReclass::CHud * g_pHud;
	extern CUserMessages * g_pUserMessages; //we need to wait for it and thats why it defined here!
	extern IViewPort * g_pViewPortInterface;
	extern ISteamClient * g_pSteamClient;
	extern ISteamFriends * g_pSteamFriends;
	extern SurfaceV30::ISurfaceFixed * g_pSurface;
	extern IMaterialSystemFixed * g_pMaterialSystemFixed;
	extern IVRenderView * g_pRenderView;
	extern IVModelRender * g_pModelRender;

	extern CGlobalVarsBase * g_pGlobals;
	
	void InitInterfaces();
	void Destroy();
	void FASTERCALL WaitForModule(const char * pModuleName);
	void * FASTERCALL WaitForExternal(const char * pModuleName, const char * pExternalName);
	void * FASTERCALL WaitForInterface(CreateInterfaceFn pFactory, const char * pInterfaceName);
	void * FASTERCALL WaitForObjectAllocationAndInitialization(void ** pObjectPtr, bool bRTTIObject);

	extern bool g_bHaveBeenWaitedForSomething;
};

#endif