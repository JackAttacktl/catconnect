#ifndef _PRINTERS_INCLUDED_
#define _PRINTERS_INCLUDED

#include <stdarg.h>
#include <stdio.h>
#include "Color.h"

namespace NSUtils
{
	void PrintToClientChat(const char * pFormat, ...);
	void PrintToPartyChat(const char * pFormat, ...);
	void PrintToClientConsole(Color cColor, const char * pFormat, ...);
	void PrintToChatAll(bool bTeamChat, const char * pFormat, ...);
};

#endif