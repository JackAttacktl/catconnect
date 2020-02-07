#ifndef _GROUPBOX_CONTROL_INC_
#define _GROUPBOX_CONTROL_INC_

#include "basecontrol.h"

#define GROUP_DEFAULT_WIDTH 180

namespace NSMenu
{
	class CMenu;

	class CGroupBox : public CBaseControl
	{
	public:
		CGroupBox(CMenu * pDrawer, const char * pName, const std::vector<CBaseControl *> & vChild, unsigned int iWidth, unsigned int iHeight = 0) : CBaseControl(pDrawer)
		{
			m_iType = EControlType::Control_GroupBox;
			strcpy(m_cName, pName);
			m_vChilds = vChild;
			m_iWidth = iWidth;
			m_iHeight = iHeight;
			m_iFlags = ControlFlag_Scale_Width;
		}

		virtual ~CGroupBox() {}

	public: //iface
		virtual unsigned int Draw(bool bMouseOver = false);
		virtual void HandleInput();
		virtual unsigned int GetHeight() const;
	};
};

#endif