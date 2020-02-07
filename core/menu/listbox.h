#ifndef _LISTBOX_CONTROL_INC_
#define _LISTBOX_CONTROL_INC_

#include "basecontrol.h"
#include "menu.h"
#include <vector>

namespace NSMenu
{
#define LISTBOX_TEXT_HIGHT 19

	class CListBox : public CBaseControl
	{
	public:
		CListBox(CMenu * pDrawer, const char * pName, const char * pSetting, std::vector<std::string> vList, int iWidth = 100, int iX = 0, int iY = 0) : CBaseControl(pDrawer)
		{
			m_iType = EControlType::Control_ListBox;
			m_iFlags = ControlFlag_Scale_Width;
			strcpy(m_cName, pName);
			strcpy(m_cSetting, pSetting);
			m_vList = vList;
			m_iValue = 0;
			m_iX = iX;
			m_iY = iY;
			m_iHeight = 34;
			m_iWidth = iWidth;
		}

	public: //iface
		virtual unsigned int Draw(bool bMouseOver = false);
		virtual void HandleInput();

		inline auto * GetList() const { return &m_vList; }
		inline unsigned int GetCurrentValue() const { return m_iValue; }
		void SetCurrentValue(unsigned int iValue);

	private:
		static void DrawListBox(void * pData, unsigned int iIndex);
		void RecheckValue();

	private:
		unsigned int m_iValue;
		std::vector<std::string> m_vList;
		static CDialog ms_CListDialog;
	};
};

#endif