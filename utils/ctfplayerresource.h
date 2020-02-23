#ifndef __C_TF_PLAYERRESOURCE_INC
#define __C_TF_PLAYERRESOURCE_INC

#include <cstring>
#include <string>
#include "interfaces.h"
#include "defs.h"
#include "netvars.h"
#include "cdll_int.h"
#include "client_class.h"
#include "xorstr.h"
#include "const.h"
#include "icliententitylist.h"
#include "icliententity.h"
#include "iclientnetworkable.h"
#include "iclientunknown.h"

namespace NSReclass
{
	class CBaseEntity;

	class CTFPlayerResource
	{
	public:
		FORCEINLINE CTFPlayerResource() : m_iEntityNumber(0), m_iMyClassID(-1) { RecheckAndGetEntity(); }
		FORCEINLINE ~CTFPlayerResource() {}

		FORCEINLINE bool IsPlayerAlive(int iClient)
		{
			CBaseEntity * pMyself = EntPointerOfEntNumber(RecheckAndGetEntity());
			if (!pMyself) return false;
			static unsigned int s_iOffset = 0;
			if (!s_iOffset)
				s_iOffset = NETVARS_GET_OFFSET(xorstr_("DT_TFPlayerResource"), xorstr_("baseclass"), xorstr_("m_bAlive"));
			return *((bool *)((char *)pMyself + s_iOffset + iClient));
		}

		FORCEINLINE int GetPlayerClass(int iClient)
		{
			CBaseEntity * pMyself = EntPointerOfEntNumber(RecheckAndGetEntity());
			if (!pMyself) return 0;
			static unsigned int s_iOffset = 0;
			if (!s_iOffset)
				s_iOffset = NETVARS_GET_OFFSET(xorstr_("DT_TFPlayerResource"), xorstr_("m_iPlayerClass"));
			return *((int *)((char *)pMyself + s_iOffset + iClient * 4));
		}

		FORCEINLINE int GetPlayerTeam(int iClient)
		{
			CBaseEntity * pMyself = EntPointerOfEntNumber(RecheckAndGetEntity());
			if (!pMyself) return 0;
			static unsigned int s_iOffset = 0;
			if (!s_iOffset)
				s_iOffset = NETVARS_GET_OFFSET(xorstr_("DT_TFPlayerResource"), xorstr_("baseclass"), xorstr_("m_iTeam"));
			return *((int *)((char *)pMyself + s_iOffset + iClient * 4));
		}

	private:
		FORCEINLINE CBaseEntity * EntPointerOfEntNumber(int iEntity)
		{
			if (iEntity <= 0)
				return nullptr;
			IClientEntity * pClEnt = NSInterfaces::g_pClientEntityList->GetClientEntity(iEntity);
			if (!pClEnt)
				return nullptr;
			IClientNetworkable * pNt = pClEnt->GetClientNetworkable();
			if (!pNt)
				return nullptr;
			IClientUnknown * pUnk = pNt->GetIClientUnknown();
			if (!pUnk)
				return nullptr;
			CBaseEntity * pEnt = (CBaseEntity *)pUnk->GetBaseEntity();
			return pEnt;
		}

		FORCEINLINE int RecheckAndGetEntity()
		{
			if (m_iMyClassID <= 0) //m_ClassID will be 0 until all inited
			{
				ClientClass * pClasses = NSInterfaces::g_pClient->GetAllClasses();
				std::string sClName = xorstr_("CTFPlayerResource");
				while (pClasses)
				{
					if (!strcmp(pClasses->GetName(), sClName.data()))
					{
						m_iMyClassID = pClasses->m_ClassID;
						break;
					}
					pClasses = pClasses->m_pNext;
				}
				memset(sClName.data(), 0, sClName.length());
			}

			//this entity is only valid when we in-game
			if (!NSInterfaces::g_pEngineClient->IsInGame())
			{
				m_iEntityNumber = 0;
				return 0;
			}

			//first check current entity
			if (m_iEntityNumber)
			{
				IClientEntity * pClEnt = NSInterfaces::g_pClientEntityList->GetClientEntity(m_iEntityNumber);
				if (pClEnt)
				{
					ClientClass * pEntClass = pClEnt->GetClientClass();
					if (pEntClass && pEntClass->m_ClassID == m_iMyClassID)
						return m_iEntityNumber;
				}
			}

			//if current entity is invalid, then find new

			for (int i = NSInterfaces::g_pEngineClient->GetMaxClients() + 1; i < MAX_EDICTS; i++)
			{
				IClientEntity * pClEnt = NSInterfaces::g_pClientEntityList->GetClientEntity(i);
				if (!pClEnt)
					continue;
				ClientClass * pEntClass = pClEnt->GetClientClass();
				if (pEntClass && pEntClass->m_ClassID == m_iMyClassID)
				{
					m_iEntityNumber = i;
					break;
				}
			}

			return m_iEntityNumber;
		}

	private:
		int m_iEntityNumber;
		int m_iMyClassID;
	};

	extern CTFPlayerResource * g_pCTFPlayerResource; //defined in globals.cpp, inited in interfaces.cpp, don't use it until initialized!
};

#endif