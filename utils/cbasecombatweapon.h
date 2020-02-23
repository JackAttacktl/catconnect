#ifndef _CBASECOMBATWEAPON_INC_
#define _CBASECOMBATWEAPON_INC_

#include "cbaseentity.h"

namespace NSReclass
{
	class CBaseCombatWeapon : public CBaseEntity
	{
	public:

		FORCEINLINE bool IsBaseCombatWeapon() const
		{
			//Windows offset 137, linux 190
			//Yeah I know, windows offsets are crazy, I assume this is caused by aggressive msvc functions devirtualization optimization
			return CALL_VFUNC_OFFS(bool (__thiscall *)(const CBaseCombatWeapon * pThis), this, 137)(this);
		}

		FORCEINLINE int GetSlot() const
		{
			//linux offset 395
			return CALL_VFUNC_OFFS(int (__thiscall *)(const CBaseCombatWeapon * pThis), this, 327)(this);
		}

		FORCEINLINE const char * GetPrintName() const
		{
			//linux offset 398
			return CALL_VFUNC_OFFS(const char * (__thiscall *)(const CBaseCombatWeapon * pThis), this, 330)(this);
		}
	};
};

#endif