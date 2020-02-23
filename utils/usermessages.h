#ifndef __USERMESSAGES_RECLASS_INC_
#define __USERMESSAGES_RECLASS_INC_

#include "defs.h"
#include "sigscan.h"
#include "xorstr.h"
#include "interfaces.h"
#include "usermessages.h"

/*
	IMPORTANT!
	Don't use this reclass!!!
	It will crash you. This is probably because of usage old bitbuffers classes from Valve's side. I'll fix it later (or if you really need this reclass you can do it)
	Use DispatchUserMessage hook in hooksman.cpp instead
*/

namespace NSReclass
{
	class CUserMessages
	{
	public:
		FORCEINLINE static CUserMessages * GetUserMessages()
		{
			static void * s_pAddr = nullptr;
			if (!s_pAddr)
				s_pAddr = (void *)NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x83\x3D\x2A\x2A\x2A\x2A\x00\x75\x2A\x6A\x24\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04"), 19);
			typedef void (__cdecl * CreateUserMessages)();
			((CreateUserMessages)s_pAddr)();
			NSInterfaces::g_pUserMessages = (::CUserMessages *)*(void **)((char *)s_pAddr + 2);
			return (CUserMessages *)NSInterfaces::g_pUserMessages;
		}

		//Server only (why the fuck this in client.dll then?)
		FORCEINLINE void Register(const char * pName, int iSize)
		{
			static void * s_pFuncAddress = nullptr;
			if (!s_pFuncAddress)
				s_pFuncAddress = (void *)NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x55\x8B\xEC\x83\xEC\x08\x56\x8B\x75\x08\x57\x8B\xF9\x85\xF6\x74\x2A\x8D\x45\xF8\x89\x75\xF8\x50\xE8\x2A\x2A\x2A\x2A\x83"), 30);
			((void (__thiscall *)(CUserMessages * pThis, const char * pName, int iSize))s_pFuncAddress)(this, pName, iSize);
		}

		//Client only
		FORCEINLINE void HookUserMessage(const char * pName, pfnUserMsgHook pCallback)
		{
			static void * s_pFuncAddress = nullptr;
			if (!s_pFuncAddress)
				s_pFuncAddress = (void *)NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x55\x8B\xEC\x83\xEC\x08\x53\x56\x8B\x75\x08\x8B\xD9\x85\xF6"), 15);
			((void (__thiscall *)(CUserMessages * pThis, const char * pName, pfnUserMsgHook pCallback))s_pFuncAddress)(this, pName, pCallback);
		}

		FORCEINLINE bool DispatchUserMessage(int iMessageType, bf_read& bfData)
		{
			static void * s_pFuncAddress = nullptr;
			if (!s_pFuncAddress)
				s_pFuncAddress = (void *)NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x55\x8B\xEC\x8B\x55\x08\x83\xEC\x18\x56"), 10);
			return ((bool (__thiscall *)(CUserMessages * pThis, int iMessageType, bf_read& bfData))s_pFuncAddress)(this, iMessageType, bfData);
		}
	};
};

#endif