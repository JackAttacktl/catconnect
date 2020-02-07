#ifndef _BASE_CONTROL_INC_
#define _BASE_CONTROL_INC_

#include <Windows.h>
#include <vector>
#include "defs.h"
#include "settings/settings.h"
#include "Color.h"

#define MAX_ELEM_NAME_LEN 256

namespace NSMenu
{
	enum EControlFlags : unsigned int
	{
		ControlFlag_None,
		ControlFlag_NoDraw = (1 << 0),
		ControlFlag_NoInput = (1 << 1),
		ControlFlag_Scale_Width = (1 << 2),
		ControlFlag_Scale_Height = (1 << 3)
	};

	enum class EControlType
	{
		Control_Base,
		Control_TabGroup,
		Control_Tab,
		Control_GroupBox,
		Control_CheckBox,
		Control_Slider,
		Control_ListBox,
		//Control_ColorPicker,
		//Control_KeyBind,
		Control_CustomPanel,
		Control_Space
	};

#define LTEXT Color(220, 220, 220, 255)
#define BTEXT Color(150, 150, 150, 255)
#define HTEXT Color(119, 196, 196, 255)
#define CTEXT Color(206, 206, 206, 255)
#define BACKGR Color(65, 65, 72, 255)

#define CONTROL_WIDTH 100
#define SPACING 10

	class CMenu;

	class CBaseControl
	{
	public:
		inline CBaseControl(CMenu * pMenu) : m_pDrawingMenu(pMenu), m_cName{ 0 }, m_cSetting{ 0 }, m_pBindedSetting(nullptr), m_iType(EControlType::Control_Base), m_iFlags(ControlFlag_None), m_bVisible(nullptr), m_iWidth(0), m_iHeight(0) {}
		virtual ~CBaseControl() { for (auto pChild : m_vChilds) delete pChild; if (m_bVisible) delete m_bVisible; }
		virtual unsigned int Draw(bool bMouseOver = false) { return 0; }
		virtual void HandleInput() {}
		virtual unsigned int GetWidth() const { return m_iWidth; }
		virtual unsigned int GetHeight() const { return m_iHeight; }
		void FASTERCALL RunControl(int iIndex = 0);

	public: //help
		inline void SetX(int iX) { m_iX = iX; }
		inline void SetY(int iY) { m_iY = iY; }
		inline void SetPos(int iX, int iY) { m_iX = iX, m_iY = iY; }
		inline void SetPos(POINT pPos) { m_iX = pPos.x, m_iY = pPos.y; }
		inline void SetFlags(EControlFlags iFlags) { m_iFlags = iFlags; }
		inline void SetVisible(bool bVisible) { if (!m_bVisible) m_bVisible = new bool; *m_bVisible = bVisible; }
		inline POINT GetPos() const { return POINT{ m_iX, m_iY }; }
		inline void SetWidth(unsigned int iWidth) { if (m_iFlags & ControlFlag_Scale_Width) m_iWidth = iWidth; }
		inline void SetHeight(unsigned int iHeight) { if (m_iFlags & ControlFlag_Scale_Height) m_iHeight = iHeight; }
		inline void SetScale(unsigned int iWidth, unsigned int iHeight)
		{
			if (m_iFlags & ControlFlag_Scale_Width) m_iWidth = iWidth;
			if (m_iFlags & ControlFlag_Scale_Height) m_iHeight = iHeight;
		}
		inline void SetChildren(const std::vector<CBaseControl *> & rChildren) { m_vChilds = rChildren; }
		inline const std::vector<CBaseControl *> * GetChildren() const { return &m_vChilds; }
		inline EControlFlags GetFlags() const { return m_iFlags; }
		inline EControlType GetType() const { return m_iType; }
		inline bool IsVisible() const { if (!m_bVisible) return true; return *m_bVisible; }
		inline int GetX() const { return m_iX; }
		inline int GetY() const { return m_iY; }
		inline const char * GetSetting() const { return m_cSetting; }
		inline CMenu * GetDrawer() const { return m_pDrawingMenu; }
		inline void BindToSetting(NSCore::CSetting * pSetting) { m_pBindedSetting = pSetting; }

	protected:
		char m_cName[MAX_ELEM_NAME_LEN];
		char m_cSetting[SETTING_MAX_NAME_LEN];
		NSCore::CSetting * m_pBindedSetting;
		EControlType m_iType;
		EControlFlags m_iFlags;
		int m_iX = 0;
		int m_iY = 0;
		bool * m_bVisible;

	protected:
		mutable unsigned int m_iWidth = 0;
		mutable unsigned int m_iHeight = 0;
		std::vector<CBaseControl *> m_vChilds;
		CMenu * m_pDrawingMenu;
	};
};

#endif