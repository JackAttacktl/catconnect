#include "globals.h"
#include "netvars.h"

CBasePlayer * NSGlobals::g_pLocalPlayer = nullptr;
NSUtils::CNetVarTree NSUtils::g_CNetvars;
bool NSGlobals::g_bCatConnectInited = false;
std::map<std::string, void *> NSGlobals::g_mMyInterfaces;