#ifndef _TAB_GROUP_CONTROL_INC_
#define _TAB_GROUP_CONTROL_INC_

#include "basecontrol.h"
#include "tab.h"

namespace NSMenu
{
	class CTabGroup : public CBaseControl
	{
	public:
		CTabGroup(CMenu * pDrawer, const std::vector<CTab *> & vTabs, unsigned int iWidth = 20);
		CTabGroup(CMenu * pDrawer, unsigned int iWidth = 20);

		virtual ~CTabGroup() {}

	public: //iface
		virtual unsigned int Draw(bool bMouseOver = false);
		virtual void HandleInput();

	public: //help
		void FASTERCALL AddTab(CTab * pTab);
		inline unsigned int GetTabsCount() const { return m_vChilds.size(); }
		inline CTab * operator[](int iTab) const { return (CTab *)m_vChilds[iTab]; }
		inline CTab * GetActive() const { return m_pActiveTab; }

	protected: //or make it private?
		CTab * m_pActiveTab;
	};
};

#endif