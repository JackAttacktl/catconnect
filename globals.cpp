#include "globals.h"
#include "ctfplayerresource.h"
#include "netvars.h"

CBasePlayer * NSGlobals::g_pLocalPlayer = nullptr;
NSUtils::CNetVarTree NSUtils::g_CNetvars;
NSReclass::CTFPlayerResource * NSReclass::g_pCTFPlayerResource = nullptr;
bool NSGlobals::g_bCatConnectInited = false;
std::map<std::string, void *> NSGlobals::g_mMyInterfaces;