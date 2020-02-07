#ifndef _CBASEENTITY_INC_
#define _CBASEENTITY_INC_

#include "iclientnetworkable.h"
#include "iclientunknown.h"
#include "netvars.h"
#include "vmt.h"
#include "xorstr.h"
#include "icliententitylist.h"
#include "interfaces.h"
#include "basehandle.h"

namespace NSReclass
{
	class CBaseEntity
	{
	public:

		inline IClientNetworkable * GetClientNetworkable() const { return CALL_VFUNC_OFFS(IClientNetworkable * (__thiscall *)(const CBaseEntity *), this, 4)(this); }

		inline bool IsPlayer() const
		{
			//linux offset 184
			return CALL_VFUNC_OFFS(bool (__thiscall *)(const CBaseEntity * pThis), this, 131)(this);
		}

		inline CBaseEntity * GetOwner(int * pIndex = nullptr) const
		{
			static unsigned int s_iOffset = 0;
			if (!s_iOffset)
				s_iOffset = NETVARS_GET_OFFSET(xorstr_("DT_BaseEntity"), xorstr_("m_hOwnerEntity"));
			auto * pOwnerHandle = (CBaseHandle *)((char *)this + s_iOffset);
			if (!pOwnerHandle || !pOwnerHandle->IsValid())
				return nullptr;
			IClientUnknown * pUnk = NSInterfaces::g_pClientEntityList->GetClientUnknownFromHandle(*pOwnerHandle);
			if (!pUnk)
				return nullptr;
			if (pIndex)
				*pIndex = pOwnerHandle->GetEntryIndex();
			return (CBaseEntity *)pUnk->GetBaseEntity();
		}

		inline int GetTeam() const
		{
			static unsigned int s_iOffset = 0;
			if (!s_iOffset)
				s_iOffset = NETVARS_GET_OFFSET(xorstr_("DT_BaseEntity"), xorstr_("m_iTeamNum"));
			return *(int *)((char *)this + s_iOffset);
		}

		inline int GetClass() const
		{
			static unsigned int s_iOffset = 0;
			if (!s_iOffset)
				s_iOffset = NETVARS_GET_OFFSET(xorstr_("DT_TFPlayer"), xorstr_("m_PlayerClass"), xorstr_("m_iClass"));
			return *(int *)((char *)this + s_iOffset);
		}

		inline bool IsPlayerAlive() const
		{
			static unsigned int s_iOffset = 0;
			if (!s_iOffset)
				s_iOffset = NETVARS_GET_OFFSET(xorstr_("DT_BasePlayer"), xorstr_("m_lifeState"));
			return *(char *)((char *)this + s_iOffset) == 0; //0 == LIFE_ALIVE
		}
	};
};

#endif