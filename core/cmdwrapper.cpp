#include "cmdwrapper.h"
#include "xorstr.h"
#include <boost/algorithm/string.hpp>
#include <boost/range/algorithm/find_if.hpp>

void NSCore::CCmdWrapper::RegisterCmd(NSCore::CCatCommandSafe * pCmd, NSCore::CmdCallbackFn pCallback)
{
	//This is important!
	//We must NOT use ICvar interface to register commands because this is one of potential detection vectors for stupid server-sided anticheats like smac
	if (IsCommandAlreadyExists(pCmd))
		return; //Don't register cmd if it already exists
	GetCmdsMap()[pCmd] = pCallback;
}

void NSCore::CCmdWrapper::UnregisterCmd(NSCore::CCatCommandSafe * pCmd)
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
	const CCatCommandSafe * pCmd = FindCmdByName(cCmd.Arg(0));
	return (GetCmdsMap()[pCmd](cCmd));
}

const NSCore::CCatCommandSafe * NSCore::CCmdWrapper::FindCmdByName(const char * pName)
{
	auto& mMap = GetCmdsMap();
	auto iIter = std::find_if(mMap.begin(), mMap.end(), [&pName](const std::pair<const CCatCommandSafe *, CmdCallbackFn> pElem) -> bool { return !strcmp(pElem.first->GetMyName(), pName); });
	if (iIter != mMap.end())
		return iIter->first;
	return nullptr;
}

bool NSCore::CCmdWrapper::IsCommandAlreadyExists(NSCore::CCatCommandSafe * pCmd)
{
	auto& mMap = GetCmdsMap();
	auto iIter = std::find_if(mMap.begin(), mMap.end(), [&pCmd](const std::pair<const CCatCommandSafe *, CmdCallbackFn> pElem) -> bool { return pElem.first == pCmd; });
	return iIter != mMap.end();
}