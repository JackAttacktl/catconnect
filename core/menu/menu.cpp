#include "menu.h"
#include "visual/drawer.h"
#include "public/isurfacefixed.h"
#include "xorstr.h"
#include "vmt.h"
#include "interfaces.h"
#include "tab.h"
#include "groupbox.h"
#include "checkbox.h"
#include "slider.h"
#include "listbox.h"
#include "Color.h"
#include "settings/settings.h"
#include "inputsystem/iinputsystem.h"

NSMenu::CMenu::CMenu() : m_bEnabled(false), m_bIsOnDrag(false), m_pMouse{ 0, 0 }, m_pPrevMouse{ 0, 0 }, m_pMyPosition{ 100, 100 }, m_pMyScale{ 208, 510 }, m_EMouseButton(EMouseButton::MB_None), m_iKey(BUTTON_CODE_NONE)
{
	m_pMyStyle = new CDefaultStyle();
	m_pTabs = new CTabGroup(this);
	m_iFocus = 0;
}

NSMenu::CMenu::~CMenu()
{
	delete m_pMyStyle;
	delete m_pTabs;
}

bool NSMenu::CMenu::IsMouseOver(int iX, int iY, unsigned int iWidth, unsigned int iHeight)
{
	bool bIsOver = (m_pMouse.x >= iX && m_pMouse.x <= (int)(iX + iWidth) && m_pMouse.y >= iY && m_pMouse.y <= (int)(iY + iHeight));
	return bIsOver;
}

void NSMenu::CMenu::CreateGUI()
{
	m_pTabs->AddTab(
		new CTab(this, xorstr_("Settings"),
			{
			new CGroupBox(this, xorstr_("CATs info"),
				{
					new CCheckBox(this, xorstr_("Notify party chat"), xorstr_("catconnect.partychat.notify.bots")),
					new CCheckBox(this, xorstr_("Notify in-game chat"), xorstr_("catconnect.gamechat.notify.bots")),
					new CListBox(this, xorstr_("Notify say chat"), xorstr_("catconnect.chat.notify.bots"), { xorstr_("Don't notify"), xorstr_("Notify all chat"), xorstr_("Notify team chat") }),
					new CCheckBox(this, xorstr_("Print debug (console)"), xorstr_("catconnect.showdebug"))
				}, GROUP_DEFAULT_WIDTH),
			new CGroupBox(this, xorstr_("Votekicks"),
				{
					new CCheckBox(this, xorstr_("Manage votekicks"), xorstr_("catconnect.votekicks.manage")),
					new CListBox(this, xorstr_("Show voters"), xorstr_("catconnect.votekicks.partychat.notifyvoters"), { xorstr_("Don't show"), xorstr_("All"), xorstr_("Against only") })
				}, GROUP_DEFAULT_WIDTH),
			new CGroupBox(this, xorstr_("Scoreboard"),
				{
					new CCheckBox(this, xorstr_("Show cats"), xorstr_("catconnect.scoreboard.showcats")),
					new CCheckBox(this, xorstr_("Show friends && party"), xorstr_("catconnect.scoreboard.showfriends")),
					new CCheckBox(this, xorstr_("Death notify colors"), xorstr_("catconnect.deathnotice.changecolors"))
				}, GROUP_DEFAULT_WIDTH),
			new CGroupBox(this, xorstr_("Visual"),
				{
					new CSlider(this, xorstr_("Glow on cats"), xorstr_("catconnect.glow.show.cats"), 0, 10, 1),
					new CListBox(this, xorstr_("Remove unprintable"), xorstr_("catconnect.chat.removeunprintable"), { xorstr_("Don't remove"), xorstr_("Remove & process"), xorstr_("Full remove") })
				}, GROUP_DEFAULT_WIDTH)
			}));
	BindSettings();
}

void NSMenu::CMenu::BindSettings()
{
	for (uint32_t iTabs = 0; iTabs < m_pTabs->GetTabsCount(); iTabs++)
	{
		CTab * pTab = (*m_pTabs)[iTabs];
		auto pChild = pTab->GetChildren();
		for (uint32_t iTabElements = 0; iTabElements < pChild->size(); iTabElements++)
		{
			std::vector<CBaseControl *> vContainsSettings; vContainsSettings.clear();
			FindNeededElementsRecursively((*pChild)[iTabElements], vContainsSettings);
			if (!vContainsSettings.size())
				continue;
			
			for (uint32_t iSetElem = 0; iSetElem < vContainsSettings.size(); iSetElem++)
			{
				auto pElem = vContainsSettings[iSetElem];
				const char * pSetting = pElem->GetSetting();
				if (!pSetting || !*pSetting)
					continue;
				auto pSettingInst = NSCore::CSettingsCollector::FindSettingByName(pSetting);
				if (!pSettingInst)
					continue;
				pElem->BindToSetting(pSettingInst);
			}
		}
	}
}

void NSMenu::CMenu::FindNeededElementsRecursively(CBaseControl * pElement, std::vector<CBaseControl *> & vOut)
{
	switch (pElement->GetType())
	{
		case EControlType::Control_Base:
		case EControlType::Control_Space:
		case EControlType::Control_CustomPanel:
		case EControlType::Control_Tab: //wtf???
		case EControlType::Control_TabGroup: //wtf???
			break;
		case EControlType::Control_CheckBox:
		case EControlType::Control_Slider:
		case EControlType::Control_ListBox:
			vOut.push_back(pElement);
			break;
		case EControlType::Control_GroupBox:
		{
			auto pChild = pElement->GetChildren();
			for (uint32_t iElem = 0; iElem < pChild->size(); iElem++)
			{
				auto * pElem = (*pChild)[iElem];
				FindNeededElementsRecursively(pElem, vOut);
			}
			break;
		}
		default:
			break;
	}
}

void NSMenu::CMenu::GetInput(ButtonCode_t iKey)
{
	int iMouseX = 0, iMouseY = 0;
	NSInterfaces::g_pSurface->SurfaceGetCursorPos(iMouseX, iMouseY);
	m_pPrevMouse = m_pMouse;
	m_pMouse = { iMouseX, iMouseY };

	EMouseButton eBut = EMouseButton::MB_None;

	if (NSInterfaces::g_pInputSystem->IsButtonDown(MOUSE_LEFT))
		eBut = EMouseButton::MB_LeftClick;
	else if (NSInterfaces::g_pInputSystem->IsButtonDown(MOUSE_RIGHT))
		eBut = EMouseButton::MB_RightClick;

	//if (m_EMouseButton == eBut)
	{
		if (eBut == EMouseButton::MB_LeftClick && (m_EMouseButton == EMouseButton::MB_LeftClick || m_EMouseButton == EMouseButton::MB_LeftDown))
			m_EMouseButton = EMouseButton::MB_LeftDown;
		else if (eBut == EMouseButton::MB_RightClick && (m_EMouseButton == EMouseButton::MB_RightClick || m_EMouseButton == EMouseButton::MB_RightDown))
			m_EMouseButton = EMouseButton::MB_RightDown;
		else
			m_EMouseButton = eBut;
	}
	//else
	//	m_EMouseButton = eBut;
	m_iKey = iKey;
}

void NSMenu::CMenu::EndInput()
{
	m_iKey = BUTTON_CODE_NONE;
}

void NSMenu::CMenu::Draw()
{
	if (!m_bEnabled)
		return;

	if (m_EMouseButton == EMouseButton::MB_LeftClick && IsMouseOver(m_pMyPosition.x, m_pMyPosition.y, m_pMyScale.x, MENU_TOPBAR_HEIGHT) || (m_bIsOnDrag && m_EMouseButton == EMouseButton::MB_LeftDown))
		m_bIsOnDrag = true;
	else
		m_bIsOnDrag = false;

	if (m_bIsOnDrag)
	{
		m_pMyPosition.x += m_pMouse.x - m_pPrevMouse.x;
		m_pMyPosition.y += m_pMouse.y - m_pPrevMouse.y;
	}

	//main window

	if (m_pMyPosition.x < 0)
		m_pMyPosition.x = 0;
	else if (m_pMyPosition.x + m_pMyScale.x > NSCore::NSDrawUtils::g_iScreenWidth)
		m_pMyPosition.x = NSCore::NSDrawUtils::g_iScreenWidth - m_pMyScale.x;
	if (m_pMyPosition.y < 0)
		m_pMyPosition.y = 0;
	else if (m_pMyPosition.y + m_pMyScale.y + TOPBAR_HEIGHT > NSCore::NSDrawUtils::g_iScreenHeight)
		m_pMyPosition.y = NSCore::NSDrawUtils::g_iScreenHeight - m_pMyScale.y - TOPBAR_HEIGHT;

	unsigned int iTopBar = GetStyle()->TopBar(m_pMyPosition.x, m_pMyPosition.y, m_pMyScale.x, xorstr_("Cat connect"));
	NSCore::CDrawer::DrawOutlinedRectangle(m_pMyPosition.x - 1, m_pMyPosition.y - 1, m_pMyScale.x + 2, m_pMyScale.y + iTopBar + 2, Color(7, 151, 151, 255));

	// Re-adjust pos to draw below the topbar
	POINT pAdjustedPosition = { m_pMyPosition.x, LONG(m_pMyPosition.y + iTopBar) };

	// Tab region
	NSCore::CDrawer::DrawFilledRectangle(pAdjustedPosition.x, pAdjustedPosition.y, m_pMyScale.x, DEFAULT_TAB_HEIGHT, Color(29, 47, 64, 255));
	NSCore::CDrawer::DrawLine(pAdjustedPosition.x, pAdjustedPosition.y + DEFAULT_TAB_HEIGHT - 1, pAdjustedPosition.x + m_pMyScale.x, pAdjustedPosition.y + DEFAULT_TAB_HEIGHT - 1, Color(7, 151, 151, 255));

	m_pTabs->SetPos(pAdjustedPosition.x, pAdjustedPosition.y);
	m_pTabs->HandleInput();
	m_pTabs->Draw(false);

	// Control region
	NSCore::CDrawer::DrawFilledRectangle(pAdjustedPosition.x, pAdjustedPosition.y + DEFAULT_TAB_HEIGHT, m_pMyScale.x, m_pMyScale.y - DEFAULT_TAB_HEIGHT, Color(29, 47, 64, 255));
	//NSCore::CDrawer::DrawLine(pAdjustedPosition.x, pAdjustedPosition.y, pAdjustedPosition.x + m_pMyScale.x, pAdjustedPosition.y, Color(0, 0, 0, 255));

	// Re-adjusting pos and scale again
	pAdjustedPosition.y += DEFAULT_TAB_HEIGHT + 3;
	POINT pAdjustedScale = { m_pMyScale.x, m_pMyScale.y - (pAdjustedPosition.y - pAdjustedPosition.y) };

	//now Tabs

	if (m_pTabs->GetActive())
	{
		int iCurrentX = pAdjustedPosition.x + 13;
		int iCurrentY = pAdjustedPosition.y + 12;
		unsigned int iMaxWidth = 0;
		std::vector<CBaseControl *> * pControls = const_cast<std::vector<CBaseControl *> *>(m_pTabs->GetActive()->GetChildren());
		for (uint32_t i = 0; i < pControls->size(); i++)
		{
			if ((*pControls)[i]->GetFlags() & ControlFlag_NoDraw)
				continue;

			if (iCurrentY + (int)(*pControls)[i]->GetHeight() > m_pMyScale.y + pAdjustedPosition.y - 12)
			{
				iCurrentY = pAdjustedPosition.y + 12;
				iCurrentX += 13 + iMaxWidth + 10;
				iMaxWidth = 0;
			}

			if ((*pControls)[i]->GetWidth() > iMaxWidth)
				iMaxWidth = (*pControls)[i]->GetWidth();
			(*pControls)[i]->SetPos(iCurrentX, iCurrentY);

			bool bOver = IsMouseOver(iCurrentX, iCurrentY, (*pControls)[i]->GetWidth(), (*pControls)[i]->GetHeight());
			bool bGetInput = !((*pControls)[i]->GetFlags() & ControlFlag_NoInput) && bOver && !IsDialogOpen();
			if (bGetInput)
				(*pControls)[i]->HandleInput();

			(*pControls)[i]->Draw(bGetInput);

			iCurrentY += (*pControls)[i]->GetHeight() + SPACING;
		}
	}

	//and dialogs

	unsigned int iLast = m_vDialogs.size() - 1;
	if (m_vDialogs.size() > 1)
	{
		EMouseButton eMButton = m_EMouseButton;
		POINT pMouse = m_pMouse;
		POINT pPrevMouse = m_pPrevMouse;

		// Enforce focus so that only the last dialog gets to use these variables
		m_EMouseButton = EMouseButton::MB_None;
		m_pMouse = m_pPrevMouse = { 0, 0 };

		for (size_t i = 0; i < iLast; i++)
		{
			if (!m_vDialogs[i])
				continue;

			m_vDialogs[i]->GetDrawFunction()(m_vDialogs[i]->GetData(), i + 1);
		}
		
		m_EMouseButton = eMButton;
		m_pMouse = pMouse;
		m_pPrevMouse = pPrevMouse;

		m_vDialogs[iLast]->GetDrawFunction()(m_vDialogs[iLast]->GetData(), iLast + 1);
	}
	else if (!iLast)
		m_vDialogs[iLast]->GetDrawFunction()(m_vDialogs[iLast]->GetData(), iLast + 1);

	if (m_iKey == KEY_ESCAPE && m_vDialogs.size())
		m_vDialogs.pop_back();
}

void NSMenu::CMenu::OpenDialog(NSMenu::CDialog * pDialog)
{
	m_vDialogs.push_back(pDialog);
	m_iFocus = m_vDialogs.size();
}

void NSMenu::CMenu::CloseDialog(unsigned int iIndex)
{
	if (!iIndex)
		return;

	iIndex--;
	if (iIndex >= m_vDialogs.size())
		return;

	m_vDialogs.erase(m_vDialogs.begin() + iIndex);
	m_iFocus = m_vDialogs.size();
}

NSMenu::CMenu NSMenu::g_CMenu;