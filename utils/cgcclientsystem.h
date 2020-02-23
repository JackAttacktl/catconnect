#ifndef __CGC_CLIENTSYSTEM_INC_
#define __CGC_CLIENTSYSTEM_INC_

#include "defs.h"
#include "sigscan.h"
#include "xorstr.h"
#include "e8call.h"

namespace NSReclass
{
	class CGCClientSystem
	{
	public:
		FORCEINLINE static CGCClientSystem * GGCClientSystem()
		{
			static void * s_pFuncAddress = nullptr;
			if (!s_pFuncAddress)
				s_pFuncAddress = (void *)e8call_direct(NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x55\x8B\xEC\x51\x56\x8B\xF1\x57\x8D\x7E\x04\x8B\xCF\xE8"), 14) + 22);
			return ((CGCClientSystem * (__cdecl *)())s_pFuncAddress)();
		}

		FORCEINLINE bool BSendMessage(void * pGCProtobufMessage)
		{
			static void * s_pFuncAddress = nullptr;
			if (!s_pFuncAddress)
				s_pFuncAddress = (void *)e8call_direct(NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x55\x8B\xEC\x83\xEC\x14\x56\x8B\xF1\x8D\x4D\xEC\x68\x57\x14\x00\x00\xE8"), 18) + 44);
			return ((bool (__thiscall *)(CGCClientSystem * pThis, void * pGCMessage))s_pFuncAddress)(this, pGCProtobufMessage);
		}
	};
};

#endif