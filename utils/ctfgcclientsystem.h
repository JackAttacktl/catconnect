#ifndef __CTF_GC_CLIENTSYSTEM_INC_
#define __CTF_GC_CLIENTSYSTEM_INC_

#include "cgcclientsystem.h"
#include "defs.h"
#include "sigscan.h"
#include "xorstr.h"
#include "e8call.h"
#include "vmt.h"

namespace NSReclass
{
	class CTFGCClientSystem : private CGCClientSystem
	{
	public:
		FORCEINLINE static CTFGCClientSystem * GTFGCClientSystem()
		{
			static void * s_pFuncAddress = nullptr;
			if (!s_pFuncAddress)
				s_pFuncAddress = (void *)e8call(NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x55\x8B\xEC\x51\x53\x56\x57\x8B\xF9\xE8\x2A\x2A\x2A\x2A\x8B\x8F\x80\x02\x00\x00"), 20) + 10);
			return ((CTFGCClientSystem * (__cdecl *)())s_pFuncAddress)();
		}
		
		FORCEINLINE CGCClientSystem * ToBaseClientSystem() { return (CGCClientSystem *)NSUtils::CVirtualMemberTableMan::DoDynamicCast((void ***)this, xorstr_("CGCClientSystem")); }

		FORCEINLINE void AbandonCurrentMatch()
		{
			static void * s_pFuncAddress = nullptr;
			if (!s_pFuncAddress)
				s_pFuncAddress = (void *)NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x55\x8B\xEC\x83\xEC\x14\x56\x57\x8B\x3D\x2A\x2A\x2A\x2A\x8B"), 15);
			((void (__thiscall *)(CTFGCClientSystem * pThis))s_pFuncAddress)(this);
		}

		FORCEINLINE bool BHaveLiveMatch()
		{
			static void * s_pFuncAddress = nullptr;
			if (!s_pFuncAddress)
				s_pFuncAddress = (void *)NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x56\x8B\xF1\x8D\x8E\x98\x05\x00\x00\xE8"), 10);
			return ((bool (__thiscall *)(CTFGCClientSystem * pThis))s_pFuncAddress)(this);
		}

		FORCEINLINE void JoinMMMatch()
		{
			static void * s_pFuncAddress = nullptr;
			if (!s_pFuncAddress)
				s_pFuncAddress = (void *)NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x56\x8B\xF1\x57\x8D\x8E\x98\x05\x00\x00\xE8"), 11);
			((void (__thiscall *)(CTFGCClientSystem * pThis))s_pFuncAddress)(this);
		}

		FORCEINLINE bool BConnectedToMatchServer(bool bUnknown = false)
		{
			static void * s_pFuncAddress = nullptr;
			if (!s_pFuncAddress)
				s_pFuncAddress = (void *)NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x55\x8B\xEC\x53\x8A\x5D\x08\x56\x8B\xF1\x84\xDB\x74\x2A\x8D\x8E\x98\x05\x00\x00"), 20);
			return ((bool (__thiscall *)(CTFGCClientSystem * pThis, bool bUnk))s_pFuncAddress)(this, bUnknown);
		}

	private:
		FORCEINLINE void ConnectToServer_Useless(const char * pServerAddress)
		{
			static void * s_pFuncAddress = nullptr;
			if (!s_pFuncAddress)
				s_pFuncAddress = (void *)NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x55\x8B\xEC\x51\x57\xFF\x75\x08\x8B\xF9\x68\x2A\x2A\x2A\x2A\xFF"), 16);
			((void (__thiscall *)(CTFGCClientSystem * pThis, const char * pServerAddress))s_pFuncAddress)(this, pServerAddress);
		}
	};
};

#endif