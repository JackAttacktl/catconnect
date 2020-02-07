#ifndef _SPACE_CONTROL_INC_
#define _SPACE_CONTROL_INC_

#include "basecontrol.h"

namespace NSMenu
{
	class CMenu;

	class CSpace : public CBaseControl
	{
	public:
		CSpace(CMenu * pDrawer, unsigned int iAmount = 0) : CBaseControl(pDrawer)
		{
			m_iType = EControlType::Control_Space;
			m_iHeight = iAmount;
			m_iFlags = EControlFlags(ControlFlag_Scale_Width | ControlFlag_Scale_Height | ControlFlag_NoInput);
		}

		virtual ~CSpace() {}
	};
};

#endif