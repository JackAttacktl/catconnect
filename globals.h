#ifndef _GLOBALS_INC_
#define _GLOBALS_INC_

#include "defs.h"
#include <map>
#include <string>

namespace NSGlobals
{
	extern CBasePlayer * g_pLocalPlayer;
	extern bool g_bCatConnectInited;
	extern std::map<std::string, void *> g_mMyInterfaces;
};

#endif