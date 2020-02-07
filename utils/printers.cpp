#include "defs.h"
#include "printers.h"
#include "ctfpartyclient.h"
#include "icvar.h"
#include "interfaces.h"
#include "utils/hud.h" //utils/ important here
#include "xorstr.h"
#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>

void NSUtils::PrintToClientChat(const char * pFormat, ...)
{
	char cBuffer[1024];

	va_list vaP;
	va_start(vaP, pFormat);
	size_t len = vsnprintf(cBuffer, sizeof(cBuffer), pFormat, vaP);
	va_end(vaP);

	NSReclass::CHudBaseChat * pChat = (NSReclass::CHudBaseChat *)NSInterfaces::g_pHud->FindElement(xorstr_("CHudChat"));
	if (pChat)
		pChat->Printf(cBuffer);
	memset(cBuffer, 0, sizeof(cBuffer));
}

void NSUtils::PrintToPartyChat(const char * pFormat, ...)
{
	char cBuffer[256];

	va_list vaP;
	va_start(vaP, pFormat);
	size_t len = vsnprintf(cBuffer, sizeof(cBuffer), pFormat, vaP);
	va_end(vaP);

	NSReclass::CTFPartyClient::GTFPartyClient()->SendPartyChat(cBuffer);
	memset(cBuffer, 0, sizeof(cBuffer));
}

void NSUtils::PrintToClientConsole(Color cColor, const char * pFormat, ...)
{
	char cBuffer[1024];

	va_list vaP;
	va_start(vaP, pFormat);
	size_t len = vsnprintf(cBuffer, sizeof(cBuffer), pFormat, vaP);
	va_end(vaP);

	std::string sLine = cBuffer;
	if (!boost::ends_with(sLine, xorstr_("\n")))
		sLine += xorstr_("\n");

	NSInterfaces::g_pCVar->ConsoleColorPrintf(cColor, sLine.c_str());
	memset(cBuffer, 0, sizeof(cBuffer));
	memset(sLine.data(), 0, sLine.length());
}

void NSUtils::PrintToChatAll(bool bTeamChat, const char * pFormat, ...)
{
	char cBuffer[256];

	va_list vaP;
	va_start(vaP, pFormat);
	size_t len = vsnprintf(cBuffer, sizeof(cBuffer), pFormat, vaP);
	va_end(vaP);

	std::string sToPrintAll = "";
	if (bTeamChat)
		sToPrintAll += xorstr_("say_team ");
	else
		sToPrintAll += xorstr_("say ");
	sToPrintAll += cBuffer;

	NSInterfaces::g_pEngineClient->ClientCmd_Unrestricted(sToPrintAll.c_str());
	memset(cBuffer, 0, sizeof(cBuffer));
	memset(sToPrintAll.data(), 0, sToPrintAll.length());
}