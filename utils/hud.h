#ifndef _HUD_RC_INC_
#define _HUD_RC_INC_

#include "defs.h"
#include "sigscan.h"
#include "e8call.h"
#include "xorstr.h"
#include "vmt.h"
#include "utlvector.h"
#include "Color.h"

namespace NSReclass
{
    class CHudElement
    {
        //private
        void * m_pVTable;
    };

    class CHudBaseChat : public CHudElement
    {
    public:
        FORCEINLINE void Printf(const char * pMessage)
        {
            typedef void (__cdecl * Printf_t)(CHudBaseChat *, int, const char *, ...);
            CALL_VFUNC_OFFS(Printf_t, this, 18)(this, 0, xorstr_("%s"), pMessage); //21 in linux
        }
    };

    class CHud
    {
    public:
        FORCEINLINE CHudElement * FindElement(const char * pName)
        {
            typedef CHudElement * (__thiscall * FindElement_t)(CHud *, const char *);
            static FindElement_t FindElementFn = nullptr;
            if (!FindElementFn)
            {
                void * pAddr = (void *)e8call_direct(NSUtils::Sigscan(xorstr_("client.dll"), xorstr_("\x8B\x01\x8B\x40\x24\xFF\xD0\x8B\xC8\xE8\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xB9\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x85\xC0"), 31) + 24);  //also another sig: \x55\x8B\xEC\x53\x8B\x5D\x08\x56\x57\x8B\xF9\x33\xF6\x39\x77\x28
                FindElementFn = FindElement_t(pAddr);
            }

            if (FindElementFn) //this may happen if we load too fast
                return FindElementFn(this, pName);
            return nullptr;
        }

    public:
        int m_iKeyBits;
#ifndef _XBOX
        float m_flMouseSensitivity;
        float m_flMouseSensitivityFactor;
#endif
        float m_flFOVSensitivityAdjust;
        Color m_clrNormal;
        Color m_clrCaution;
        Color m_clrYellowish;
        CUtlVector<CHudElement *> m_HudList;
        bool m_bHudTexturesLoaded;
    };
};

#endif