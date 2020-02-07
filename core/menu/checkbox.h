#ifndef _CHECKBOX_CONTROL_INC_
#define _CHECKBOX_CONTROL_INC_

#include "basecontrol.h"

namespace NSMenu
{
	class CMenu;

	class CCheckBox : public CBaseControl
	{
	public:
		CCheckBox(CMenu * pDrawer, const char * pName, const char * pSetting, unsigned int iWidth = 100, int iX = 0, int iY = 0) : CBaseControl(pDrawer)
		{
			m_iType = EControlType::Control_CheckBox;
			strcpy(m_cName, pName);
			strcpy(m_cSetting, pSetting);
			m_iX = iX, m_iY = iY;
			m_iFlags = ControlFlag_Scale_Width;
			m_iWidth = iWidth;
			m_iHeight = 16;
			m_bChecked = false;
		}

		virtual ~CCheckBox() {}

		bool QuickReturn(bool bValue, int iX, int iY, unsigned int iWidth = 0);

	public: //iface
		virtual unsigned int Draw(bool bMouseOver = false);
		virtual void HandleInput();

	public: //help
		inline bool IsChecked() { return m_bChecked; }
		inline void SetChecked(bool bChecked) { m_bChecked = bChecked; }

	private:
		bool m_bChecked;
	};
};

#endif