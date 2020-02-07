#ifndef _DRAWPANEL_CONTROL_INC_
#define _DRAWPANEL_CONTROL_INC_

#include "basecontrol.h"
#include "menu.h"

namespace NSMenu
{
	class CMenu;

	class CDrawPanel : public CBaseControl
	{
	public:
		CDrawPanel(CMenu * pDrawer, const char * pName, DrawFunctionFn pDrawFunction, unsigned int iW = 200, unsigned int iH = 200, int iX = 0, int iY = 0) : CBaseControl(pDrawer)
		{
			m_iType = EControlType::Control_CustomPanel;
			m_iFlags = EControlFlags(ControlFlag_Scale_Width | ControlFlag_Scale_Height | ControlFlag_NoInput);
			strcpy(m_cName, pName);
			m_pDrawFunc = pDrawFunction;
			m_iX = iX;
			m_iY = iY;
			m_iWidth = iW;
			m_iHeight = iH;
		}

		virtual ~CDrawPanel() {}

	public: //iface

		virtual unsigned int Draw(bool bMouseOver = false);
		virtual void HandleInput() {}

	private:
		DrawFunctionFn m_pDrawFunc;
	};
};

#endif