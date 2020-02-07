#ifndef __CMD_WRAPPER_INC
#define __CMD_WRAPPER_INC

#include <map>
#include <string>
#include "tier1/convar.h"
#include "catconnect.h"

namespace NSCore
{
	//return true to handle (don't call original function) and false to allow engine to process it
	typedef bool (* CmdCallbackFn)(const CCommand & rCmd);

	class CCatCommandSafe;

	class CCmdWrapper
	{
	private:
		friend bool CCatConnect::OnClientCommand(const char * pCmdLine);
		friend class CCatCommandSafe;

		static void RegisterCmd(const char * pCmd, CmdCallbackFn pCallback);
		static void UnregisterCmd(const char * pCmd);

		static bool OnStringCommand(const char * pCmdLine); //must be called from catconnect only!

	private:
		static inline bool IsCommandAlreadyExists(const char * pCmd) { return GetCmdsMap().find(pCmd) != GetCmdsMap().end(); }

		static inline std::map<std::string, CmdCallbackFn> & GetCmdsMap()
		{
			static std::map<std::string, CmdCallbackFn> s_mMap;
			return s_mMap;
		}
	};

	class CCatCommandSafe
	{
	public:
		inline explicit CCatCommandSafe(const char * pCmd, CmdCallbackFn pCallback) { m_sMyName = pCmd; CCmdWrapper::RegisterCmd(pCmd, pCallback); }
		inline ~CCatCommandSafe() { CCmdWrapper::UnregisterCmd(m_sMyName.c_str()); }

	private:
		std::string m_sMyName;
	};
};

#endif