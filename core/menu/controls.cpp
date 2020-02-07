#include <stdint.h>
#include <cstring>

#include "visual/drawer.h"
#include "interfaces.h"
#include "public/isurfacefixed.h"
#include "xorstr.h"
#include "vmt.h"

#include "basecontrol.h"
#include "tab.h"
#include "tabgroup.h"
#include "groupbox.h"
#include "checkbox.h"
#include "drawpanel.h"
#include "slider.h"
#include "listbox.h"
#include "space.h"

#include "menu.h"

void NSMenu::CBaseControl::RunControl(int iIndex)
{
	if (m_iType == EControlType::Control_Base)
		return;

	bool bMouseOver = m_pDrawingMenu->IsMouseOver(m_iX, m_iY, GetWidth(), GetHeight());
	if (m_pDrawingMenu->GetFocus() == iIndex && !(m_iFlags & ControlFlag_NoInput) && bMouseOver)
		HandleInput();
	if (!(m_iFlags & ControlFlag_NoDraw))
		Draw(bMouseOver);
}

unsigned int NSMenu::CTab::Draw(bool bMouseOver)
{
	int iStrW, iStrH;
	wchar_t * pTemp = new wchar_t[strlen(m_cName) + 1];
	mbstowcs(pTemp, m_cName, strlen(m_cName) + 1);
	NSInterfaces::g_pSurface->GetTextSize(NSCore::NSDrawUtils::g_iFontArial, pTemp, iStrW, iStrH);
	delete[] pTemp;
	m_iWidth = DEFAULT_TAB_PADDING * 2 + iStrW;
	if (m_bEnabled)
	{
		NSCore::CDrawer::DrawFilledRectangle(m_iX, m_iY, m_iWidth, m_iHeight, Color(7, 151, 151, 255));
	}
	else
	{
		if (bMouseOver)
			NSCore::CDrawer::DrawFilledRectangle(m_iX, m_iY, m_iWidth, m_iHeight, Color(64, 103, 137, 125));
		else
			NSCore::CDrawer::DrawFilledRectangle(m_iX, m_iY, m_iWidth, m_iHeight, Color(29, 47, 64, 255));
	}

	NSCore::CDrawer::DrawString(m_iX + DEFAULT_TAB_PADDING, m_iY + 3, m_bEnabled ? LTEXT : BTEXT, m_cName, NSCore::NSDrawUtils::g_iFontArial);

	return m_iWidth;
}

NSMenu::CTabGroup::CTabGroup(CMenu * pDrawer, const std::vector<CTab *> & vTabs, unsigned int iWidth) : CBaseControl(pDrawer)
{
	m_iWidth = iWidth;
	m_vChilds = (const std::vector<CBaseControl *> &)vTabs;
	m_pActiveTab = vTabs[0];
	//m_iFlags = ControlFlag_Scale_Width;
}

NSMenu::CTabGroup::CTabGroup(CMenu * pDrawer, unsigned int iWidth) : CBaseControl(pDrawer)
{
	m_iWidth = iWidth;
	m_pActiveTab = nullptr;
	//m_iFlags = ControlFlag_Scale_Width;
}

void NSMenu::CTabGroup::AddTab(CTab * pTab)
{
	if (!m_vChilds.size())
		m_pActiveTab = pTab;
	m_vChilds.push_back(pTab);
}

unsigned int NSMenu::CTabGroup::Draw(bool bMouseOver)
{
	m_iHeight = GetHeight();

	int iCurX = m_iX;
	for (uint32_t i = 0; i < m_vChilds.size(); i++)
	{
		CTab * pTab = (CTab *)m_vChilds[i];
		pTab->SetWidth(m_iWidth);
		pTab->SetPos(iCurX, m_iY);
		pTab->SetEnabled(pTab == m_pActiveTab);

		pTab->HandleInput();
		unsigned int iAddX = pTab->Draw(m_pDrawingMenu->IsMouseOver(iCurX, m_iY, pTab->GetWidth(), DEFAULT_TAB_HEIGHT));

		iCurX += (int)iAddX;
	}

	return iCurX - m_iX;
}

void NSMenu::CTabGroup::HandleInput()
{
	m_iHeight = GetHeight();
	if (!m_pDrawingMenu->IsMouseOver(m_iX, m_iY, m_iWidth, m_iHeight))
		return;

	int iCurX = m_iX;
	for (uint32_t i = 0; i < m_vChilds.size(); i++)
	{
		CTab * pTab = (CTab *)m_vChilds[i];
		if (m_pDrawingMenu->IsMouseOver(iCurX, m_iY, pTab->GetWidth(), DEFAULT_TAB_HEIGHT) && m_pDrawingMenu->GetMouseButton() == EMouseButton::MB_LeftClick)
		{
			m_pActiveTab = pTab;
			break;
		}

		iCurX += (int)pTab->GetWidth();
	}
}

unsigned int NSMenu::CGroupBox::Draw(bool bMouseOver)
{
	// Initializing our height variable
	GetHeight();

	int iStrW, iStrH;
	wchar_t * pTemp = new wchar_t[strlen(m_cName) + 1];
	mbstowcs(pTemp, m_cName, strlen(m_cName) + 1);
	NSInterfaces::g_pSurface->GetTextSize(NSCore::NSDrawUtils::g_iFontArial, pTemp, iStrW, iStrH);
	delete[] pTemp;

	m_iY += (iStrH / 2);

	NSCore::CDrawer::DrawLine(m_iX, m_iY, m_iX, m_iY + m_iHeight, Color(7, 151, 151, 255));
	NSCore::CDrawer::DrawLine(m_iX + m_iWidth, m_iY, m_iX + m_iWidth, m_iY + m_iHeight, Color(7, 151, 151, 255));
	NSCore::CDrawer::DrawLine(m_iX, m_iY + m_iHeight, m_iX + m_iWidth, m_iY + m_iHeight, Color(7, 151, 151, 255));

	NSCore::CDrawer::DrawString(m_iX + (m_iWidth / 2) - (iStrW / 2), m_iY - (iStrH / 2), CTEXT, m_cName, NSCore::NSDrawUtils::g_iFontArial);
	NSCore::CDrawer::DrawLine(m_iX, m_iY, m_iX + (m_iWidth / 2) - (iStrW / 2) - 5, m_iY, Color(7, 151, 151, 255));
	NSCore::CDrawer::DrawLine(m_iX + (m_iWidth / 2) + (iStrW / 2) + 5, m_iY, m_iX + m_iWidth, m_iY, Color(7, 151, 151, 255));

	int iCurX = m_iX + SPACING, iCurY = m_iY + SPACING;

	for (uint32_t i = 0; i < m_vChilds.size(); i++)
	{
		m_vChilds[i]->SetPos(iCurX, iCurY);
		m_vChilds[i]->SetWidth(m_iWidth - (SPACING * 2));

		bool bMouseOver = m_pDrawingMenu->IsMouseOver(iCurX, iCurY, m_vChilds[i]->GetWidth(), m_vChilds[i]->GetHeight());
		bool bGetInput = !(m_vChilds[i]->GetFlags() & ControlFlag_NoInput) && bMouseOver && !m_pDrawingMenu->IsDialogOpen();

		m_vChilds[i]->Draw(bGetInput);
		iCurY += m_vChilds[i]->GetHeight() + SPACING;
	}

	m_iY -= (iStrH / 2);

	return m_iHeight;
}

void NSMenu::CGroupBox::HandleInput()
{
	int iStrW, iStrH;
	wchar_t * pTemp = new wchar_t[strlen(m_cName) + 1];
	mbstowcs(pTemp, m_cName, strlen(m_cName) + 1);
	NSInterfaces::g_pSurface->GetTextSize(NSCore::NSDrawUtils::g_iFontArial, pTemp, iStrW, iStrH);
	delete[] pTemp;

	int iCurX = m_iX + SPACING;
	int iCurY = m_iY + SPACING + (iStrH / 2);

	for (uint32_t i = 0; i < m_vChilds.size(); i++)
	{
		m_vChilds[i]->SetPos(iCurX, iCurY);
		m_vChilds[i]->SetWidth(m_iWidth - (SPACING * 2));

		bool bMouserOver = m_pDrawingMenu->IsMouseOver(iCurX, iCurY, m_vChilds[i]->GetWidth(), m_vChilds[i]->GetHeight());
		bool bGetInput = !(m_vChilds[i]->GetFlags() & ControlFlag_NoInput) && bMouserOver && !m_pDrawingMenu->IsDialogOpen();
		if (bGetInput)
			m_vChilds[i]->HandleInput();

		iCurY += m_vChilds[i]->GetHeight() + SPACING;
	}
}

unsigned int NSMenu::CGroupBox::GetHeight() const
{
	m_iHeight = SPACING;

	for (uint32_t i = 0; i < m_vChilds.size(); i++)
	{
		if (!m_vChilds[i]->IsVisible())
			continue;
		m_iHeight += m_vChilds[i]->GetHeight() + SPACING;
	}

	int iStrW, iStrH;
	wchar_t* pTemp = new wchar_t[strlen(m_cName) + 1];
	mbstowcs(pTemp, m_cName, strlen(m_cName) + 1);
	NSInterfaces::g_pSurface->GetTextSize(NSCore::NSDrawUtils::g_iFontArial, pTemp, iStrW, iStrH);
	delete[] pTemp;

	return m_iHeight + (iStrH / 2);
}

unsigned int NSMenu::CCheckBox::Draw(bool bMouseOver)
{
	if (m_pBindedSetting)
		m_bChecked = m_pBindedSetting->GetBool();
	if (m_bChecked)
	{
		if (bMouseOver)
			NSCore::CDrawer::DrawFilledRectangle(m_iX + 1, m_iY + 1, m_iHeight - 2, m_iHeight - 2, Color(64, 103, 137, 125));
		NSCore::CDrawer::DrawFilledRectangle(m_iX + 4, m_iY + 4, m_iHeight - 8, m_iHeight - 8, Color(7, 151, 151, 255));
		NSCore::CDrawer::DrawOutlinedRectangle(m_iX, m_iY, m_iHeight, m_iHeight, Color(7, 151, 151, 255));
	}
	else
	{
		if (bMouseOver)
			NSCore::CDrawer::DrawFilledRectangle(m_iX + 1, m_iY + 1, m_iHeight - 2, m_iHeight - 2, Color(119, 196, 196, 125));
		NSCore::CDrawer::DrawOutlinedRectangle(m_iX, m_iY, m_iHeight, m_iHeight, Color(255, 0, 0, 255));
	}

	NSCore::CDrawer::DrawString(m_iX + 22, m_iY, bMouseOver ? HTEXT : CTEXT, m_cName, NSCore::NSDrawUtils::g_iFontArial);

	return m_iHeight;
}

void NSMenu::CCheckBox::HandleInput()
{
	if (m_pDrawingMenu->GetMouseButton() == EMouseButton::MB_LeftClick || m_pDrawingMenu->GetMouseButton() == EMouseButton::MB_RightClick)
	{
		m_bChecked = !m_bChecked;
		m_pBindedSetting->SetValue(m_bChecked ? xorstr_("1") : xorstr_("0"));
	}
}

bool NSMenu::CCheckBox::QuickReturn(bool bValue, int iX, int iY, unsigned int iW)
{
	int iWidth = !iW ? m_iWidth : iW;
	bool bHovered = m_pDrawingMenu->IsMouseOver(iX, iY, iWidth, m_iHeight);
	m_bChecked = bValue;
	m_iX = iX;
	m_iY = iY;
	m_iWidth = iWidth;
	Draw(bHovered);
	if (bHovered)
		HandleInput();

	return m_bChecked;
}

unsigned int NSMenu::CDrawPanel::Draw(bool bMouseOver)
{
	// Draw a control panel, then draw our stuff within it
	int iTopBar = m_pDrawingMenu->GetStyle()->ControlPanel(m_iX, m_iY, m_iWidth, m_iHeight, m_cName);
	m_pDrawFunc(m_iX, m_iY + iTopBar, m_iWidth, m_iHeight - iTopBar);

	return m_iHeight;
}

unsigned int NSMenu::CSlider::Draw(bool bMouseOver)
{
	if (m_pBindedSetting)
	{
		m_iValue = m_pBindedSetting->GetInt();
		RecheckValue();
	}
	int iNewWidth = m_iWidth - 30;
	Color cColor = bMouseOver ? HTEXT : CTEXT;
	NSCore::CDrawer::DrawString(m_iX, m_iY, cColor, m_cName, NSCore::NSDrawUtils::g_iFontArial);

	int iAdjustedY = m_iY + 5;

	int iPercent = (iNewWidth - 10) * (m_iValue - m_iMin) / (m_iMax - m_iMin);
	NSCore::CDrawer::DrawFilledRectangle(m_iX, iAdjustedY + 17, iNewWidth, 2, Color(7, 151, 151, 255));
	NSCore::CDrawer::DrawOutlinedRectangle(m_iX + 2 + iPercent, iAdjustedY + 11, 6, 14, Color(7, 151, 151, 255));
	if (bMouseOver)
		NSCore::CDrawer::DrawFilledRectangle(m_iX + 3 + iPercent, iAdjustedY + 12, 4, 12, Color(64, 103, 137, 255));
	else
		NSCore::CDrawer::DrawFilledRectangle(m_iX + 3 + iPercent, iAdjustedY + 12, 4, 12, Color(29, 47, 64, 255));
	
	NSCore::CDrawer::DrawString(m_iX + iNewWidth + 6, iAdjustedY + 9, cColor, std::to_string(m_iValue).c_str(), NSCore::NSDrawUtils::g_iFontArial);

	return m_iHeight;
}

void NSMenu::CSlider::HandleInput()
{
	if (m_pDrawingMenu->GetMouseButton() == EMouseButton::MB_RightClick)
		m_iValue -= m_iStep;
	else if (m_pDrawingMenu->GetMouseButton() == EMouseButton::MB_LeftClick)
		m_iValue += m_iStep;

	RecheckValue();
	m_pBindedSetting->SetValue(std::to_string(m_iValue).c_str());
}

unsigned int NSMenu::CListBox::Draw(bool bMouseOver)
{
	if (m_pBindedSetting)
		m_iValue = m_pBindedSetting->GetInt();
	RecheckValue();

	Color cColor = bMouseOver ? HTEXT : CTEXT;
	NSCore::CDrawer::DrawString(m_iX, m_iY, cColor, m_cName, NSCore::NSDrawUtils::g_iFontArial);
	unsigned int iAddH = m_pDrawingMenu->GetStyle()->DialogButton(m_iX, m_iY + LISTBOX_TEXT_HIGHT, m_iWidth, m_vList[m_iValue].c_str(), bMouseOver);

	return m_iHeight + iAddH;
}

void NSMenu::CListBox::HandleInput()
{
	if (m_pBindedSetting)
		m_iValue = m_pBindedSetting->GetInt();
	RecheckValue();

	if (m_pDrawingMenu->GetMouseButton() != EMouseButton::MB_LeftClick)
		return;

	ms_CListDialog.SetData(this);
	m_pDrawingMenu->OpenDialog(&ms_CListDialog);
}

void NSMenu::CListBox::RecheckValue()
{
	if (!m_vList.size())
		m_iValue = 0;
	else if (m_iValue >= m_vList.size())
		m_iValue = m_vList.size() - 1;
}

void NSMenu::CListBox::SetCurrentValue(unsigned int iValue)
{
	m_iValue = iValue;
	RecheckValue();
	m_pBindedSetting->SetValue(std::to_string(m_iValue).c_str());
}

void NSMenu::CListBox::DrawListBox(void * pData, unsigned int iIndex)
{
	CBaseControl * pControl = (CBaseControl *)pData;
	if (!pControl || pControl->GetType() != EControlType::Control_ListBox)
	{
		if (pControl)
		{
			pControl->GetDrawer()->CloseDialog(iIndex);
			return;
		}
		return;
	}

	CMenu * pDrawer = pControl->GetDrawer();

	CListBox * pListBox = (CListBox *)pControl;
	int iX = pListBox->GetX();
	int iY = pListBox->GetY() + pListBox->GetHeight();
	unsigned int iWidth = pListBox->GetWidth();
	unsigned int iHeight = pListBox->GetList()->size() * 16 + 20;

	if (pDrawer->GetMouseButton() == EMouseButton::MB_LeftClick && !pDrawer->IsMouseOver(iX, iY - pListBox->GetHeight(), iWidth, iHeight + pListBox->GetHeight()))
	{
		pDrawer->CloseDialog(iIndex);
		return;
	}

	pDrawer->GetStyle()->Dialog(iX, iY, iWidth, iHeight);

	iX += 10;
	iY += 12;
	iWidth -= 20;
	iHeight -= 20;
	for (uint32_t i = 0; i < pListBox->GetList()->size(); i++)
	{
		if (pDrawer->IsMouseOver(iX, iY, iWidth, 15))
		{
			if (pDrawer->GetMouseButton() == EMouseButton::MB_LeftClick)
			{
				pListBox->SetCurrentValue(i);
				pDrawer->CloseDialog(iIndex);
				return;
			}

			NSCore::CDrawer::DrawFilledRectangle(iX, iY, iWidth, 16, Color(64, 103, 137, 255));
		}
		NSCore::CDrawer::DrawString(iX, iY + 1, CTEXT, (*pListBox->GetList())[i].c_str(), NSCore::NSDrawUtils::g_iFontArial);
		iY += 16;
	}
}

NSMenu::CDialog NSMenu::CListBox::ms_CListDialog(&NSMenu::CListBox::DrawListBox);