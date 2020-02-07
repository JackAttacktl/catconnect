#ifndef _TAB_CONTROL_INC_
#define _TAB_CONTROL_INC_

#include "basecontrol.h"

namespace NSMenu
{
#define DEFAULT_TAB_HEIGHT 20
#define DEFAULT_TAB_PADDING 5

	class CMenu;

	class CTab : public CBaseControl
	{
	public:
		CTab(CMenu * pDrawer, const char * pName, const std::vector<CBaseControl *> & vChild) : CBaseControl(pDrawer), m_bEnabled(false)
		{
			m_iType = EControlType::Control_Tab;
			strcpy(m_cName, pName);
			m_vChilds = vChild;
			m_iHeight = DEFAULT_TAB_HEIGHT;
			m_iFlags = ControlFlag_Scale_Width;
		}

		virtual ~CTab() {}

	public: //iface
		virtual unsigned int Draw(bool bMouseOver = false);
		virtual void HandleInput() {}

	public: //help
		inline void SetEnabled(bool bEnabled) { m_bEnabled = bEnabled; }
		inline bool IsEnabled() { return m_bEnabled; }

	private:
		bool m_bEnabled;
	};
};

#endif