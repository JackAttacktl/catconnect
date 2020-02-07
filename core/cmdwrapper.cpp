#include "cmdwrapper.h"
#include "xorstr.h"
#include <boost/algorithm/string.hpp>

void NSCore::CCmdWrapper::RegisterCmd(const char * pCmd, CmdCallbackFn pCallback)
{
	//This is important!
	//We must NOT use ICvar interface to register commands because this is one of potential detection vectors for stupid server-sided anticheats like smac
	if (IsCommandAlreadyExists(pCmd))
		return; //Don't register cmd if it already exists
	GetCmdsMap()[pCmd] = pCallback;
}

void NSCore::CCmdWrapper::UnregisterCmd(const char * pCmd)
{
	if (!IsCommandAlreadyExists(pCmd))
		return;
	GetCmdsMap().erase(pCmd);
}

bool NSCore::CCmdWrapper::OnStringCommand(const char * pCmdLine)
{
	std::string sLine = pCmdLine;
	if (!boost::ends_with(sLine, xorstr_("\n")) && !boost::ends_with(sLine, xorstr_(";")))
		sLine += xorstr_("\n");
	CCommand cCmd;
	cCmd.Tokenize(sLine.c_str());
	if (!IsCommandAlreadyExists(cCmd.Arg(0)))
		return false;
	return (GetCmdsMap()[cCmd.Arg(0)](cCmd));
}