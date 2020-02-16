#include "globals.h"
#include "ctfplayerresource.h"
#include "netvars.h"
#include "vmt.h"

CBasePlayer * NSGlobals::g_pLocalPlayer = nullptr;
NSUtils::CNetVarTree NSUtils::g_CNetvars;
NSReclass::CTFPlayerResource * NSReclass::g_pCTFPlayerResource = nullptr;
bool NSGlobals::g_bCatConnectInited = false;
std::map<std::string, void *> NSGlobals::g_mMyInterfaces;
#ifdef __linux__
bool NSUtils::CVirtualMemberTableMan::ms_bSegvCatcherInstalled = false;
int NSUtils::CVirtualMemberTableMan::ms_iInstallsCounter = 0;
#endif