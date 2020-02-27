#ifndef __CMD_WRAPPER_INC
#define __CMD_WRAPPER_INC

#include <map>
#include <string>
#include <stdint.h>
#include "tier1/convar.h"
#include "catconnect.h"
#include "xorstr.h"

namespace NSCore
{
#define CCMD_FLAG_NONE			0
#define CCMD_FLAG_HOOK			(1 << 0)
#define CCMD_FLAG_HIDDEN		(1 << 1)

	//return true to handle (don't call original function) and false to allow engine to process it
	typedef bool (* CmdCallbackFn)(const CCommand & rCmd);

	class CCatCommandSafe;

	class CCmdWrapper
	{
	public:
		static inline uint32_t GetCommandsCount() { return (uint32_t)GetCmdsMap().size(); }

		static inline const CCatCommandSafe * GetCommandByNumber(uint32_t iCmd)
		{
			auto iIter = GetIterByNumber(iCmd);
			if (iIter != GetCmdsMap().end()) return iIter->first;
			return nullptr;
		}

		static inline CmdCallbackFn GetCommandCallbackByNumber(uint32_t iCmd)
		{
			auto iIter = GetIterByNumber(iCmd);
			if (iIter != GetCmdsMap().end()) return iIter->second;
			return nullptr;
		}

		static const CCatCommandSafe * FindCmdByName(const char * pName);

	private:
		friend bool CCatConnect::OnClientCommand(const char * pCmdLine);
		friend class CCatCommandSafe;

		static void RegisterCmd(CCatCommandSafe * pCmd, CmdCallbackFn pCallback);
		static void UnregisterCmd(CCatCommandSafe * pCmd);

		static bool OnStringCommand(const char * pCmdLine); //must be called from catconnect only!

	private:
		static inline bool IsCommandAlreadyExists(const char * pCmd) { return !!FindCmdByName(pCmd); }
		static bool IsCommandAlreadyExists(CCatCommandSafe * pCmd);

		static inline std::map<const CCatCommandSafe *, CmdCallbackFn> & GetCmdsMap()
		{
			static std::map<const CCatCommandSafe *, CmdCallbackFn> s_mMap;
			return s_mMap;
		}

		static inline std::map<const CCatCommandSafe *, CmdCallbackFn>::iterator GetIterByNumber(uint32_t iCmd)
		{
			auto& mMap = GetCmdsMap();
			if (iCmd >= mMap.size()) return mMap.end();
			auto iIter = mMap.begin();
			for (uint32_t iNum = 0; iNum != iCmd; iNum++) iIter++;
			return iIter;
		}
	};

	class CCatCommandSafe
	{
	public:
		inline explicit CCatCommandSafe(const char * pCmd, CmdCallbackFn pCallback) { RegisterMyself(pCmd, pCallback); }
		inline explicit CCatCommandSafe(const char * pCmd, uint32_t iFlags, CmdCallbackFn pCallback) : m_iFlags(iFlags) { RegisterMyself(pCmd, pCallback); }
		inline explicit CCatCommandSafe(const char * pCmd, uint32_t iFlags, const char * pDescription, CmdCallbackFn pCallback) : m_iFlags(iFlags) { RegisterMyself(pCmd, pCallback); m_sDesc = pDescription; }
		inline ~CCatCommandSafe() { UnregisterMyself(); }

		inline const char * GetMyName() const { return m_sMyName.c_str(); }
		inline const char * GetMyDescription() const { return m_sDesc.c_str(); }
		inline uint32_t GetMyFlags() const { return m_iFlags; }

	private:
		inline void RegisterMyself(const char * pCmd, CmdCallbackFn pCallback) { m_sMyName = pCmd; CCmdWrapper::RegisterCmd(this, pCallback); m_sDesc = xorstr_("No description"); }
		inline void UnregisterMyself() { CCmdWrapper::UnregisterCmd(this); }

	private:
		std::string m_sMyName;
		std::string m_sDesc;
		uint32_t m_iFlags;
	};
};

#endif