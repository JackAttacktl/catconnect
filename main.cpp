#include <Windows.h>
#include "lazy.h"
#include "catconnect.h"
#include "logger.h"
#include "hooksman.h"
#include "globals.h"
#include "interface.h"
#include "interfaces.h"

DWORD WINAPI Start(LPVOID lpNothing)
{
	NSUtils::CLogger::Init();
	NSInterfaces::InitInterfaces();
	CHooksMan::InitHooks();
	return 0;
}

void Stop()
{
	CCatConnect::Destroy();
}

extern "C" __declspec(dllexport) void * __cdecl CreateInterface(const char * pIFaceName, int * pReturnCode)
{
	if (NSGlobals::g_bCatConnectInited)
	{
		//we are not inited yet
		if (pReturnCode)
			*pReturnCode = IFACE_FAILED;
		return nullptr;
	}

	void * pIFace = nullptr;
	for (auto sIFace : NSGlobals::g_mMyInterfaces)
	{
		if (sIFace.first == pIFaceName)
		{
			pIFace = sIFace.second;
			break;
		}
	}

	if (pReturnCode)
	{
		if (pIFace)
			*pReturnCode = IFACE_OK;
		else
			*pReturnCode = IFACE_FAILED;
	}

	return pIFace;
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwAction, LPVOID lpReserved)
{
	switch (dwAction)
	{
	case DLL_PROCESS_ATTACH:
		LI_FN(DisableThreadLibraryCalls)(hInstance);
		LI_FN(CreateThread)(nullptr, 0, Start, hInstance, 0, nullptr);
		return TRUE;
	case DLL_PROCESS_DETACH:
		Stop();
		LI_FN(FreeLibraryAndExitThread)(hInstance, 0);
		return TRUE;
	default:
		return TRUE;
	}
}