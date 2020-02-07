#ifndef __SIGSCAN_INC
#define __SIGSCAN_INC

#include <Windows.h>
#include <Psapi.h>
#include <stdint.h>
#include "lazy.h"

namespace NSUtils
{
#define InRange_NSUtils(x, a, b) (x >= a && x <= b) 
#define GetBits_NSUtils(x) (InRange_NSUtils((x & (~0x20)),'A','F') ? ((x & (~0x20)) - 'A' + 0xa) : (InRange_NSUtils(x,'0','9') ? x - '0' : 0))
#define GetByte_NSUtils(x) (GetBits_NSUtils(x[0]) << 4 | GetBits_NSUtils(x[1]))

	//Old style: "13 37 AA BB 14 48 ? ? ? ? 22 80"
	inline uint32_t Sigscan(const char * pModule, const char * pSignature)
	{
		MODULEINFO sInfo;
		LI_FN(GetModuleInformation)(LI_FN(GetCurrentProcess)(), LI_FN(GetModuleHandleA)(pModule), &sInfo, sizeof(MODULEINFO));

		DWORD dwStartAddr = (DWORD)sInfo.lpBaseOfDll;
		DWORD dwEndAddr = dwStartAddr + sInfo.SizeOfImage;
		const char * pPattern = pSignature;
		DWORD dwFirstMatch = 0;

		for (DWORD dwCurrent = dwStartAddr; dwCurrent < dwEndAddr; dwCurrent++)
		{
			if (!*pPattern)
				return dwFirstMatch;

			//Yeah, it must be unsigned char, otherwise it will not work cuz thats how simple math work!
			if (*(unsigned char *)pPattern == '?' || *(unsigned char *)dwCurrent == GetByte_NSUtils(pPattern))
			{
				if (!dwFirstMatch)
					dwFirstMatch = dwCurrent;

				if (!pPattern[1] || !pPattern[2])
					return dwFirstMatch;

				pPattern += (*pPattern == '?' ? 2 : 3);
			}
			else
			{
				pPattern = pSignature;
				dwFirstMatch = 0;
			}
		}

		return 0;
	}

	//New style: "\x13\x37\xAA\xBB\x14\x48\x2A\x2A\x2A\x2A\x22\x80"
	inline uint32_t Sigscan(const char * pModule, const char * pSignature, uint32_t iSigLen)
	{
		MODULEINFO sInfo;
		LI_FN(GetModuleInformation)(LI_FN(GetCurrentProcess)(), LI_FN(GetModuleHandleA)(pModule), &sInfo, sizeof(MODULEINFO));

		DWORD dwStartAddr = (DWORD)sInfo.lpBaseOfDll;
		DWORD dwEndAddr = dwStartAddr + sInfo.SizeOfImage;
		const char * pPattern = pSignature;
		bool bFound = false;

		for (DWORD dwCurrent = dwStartAddr; dwCurrent < dwEndAddr; dwCurrent++)
		{
			bFound = true;
			for (DWORD dwSig = 0; dwSig < iSigLen; dwSig++)
			{
				if (pSignature[dwSig] != '\x2A' && ((unsigned char *)pSignature)[dwSig] != *((unsigned char *)dwCurrent + dwSig))
				{
					bFound = false;
					break;
				}
			}
			if (bFound) return dwCurrent;
		}
		return 0;
	}
};

#endif