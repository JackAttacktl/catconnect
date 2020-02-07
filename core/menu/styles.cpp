#include <cstring>

#include "styles.h"
#include "visual/drawer.h"
#include "public/isurfacefixed.h"
#include "vmt.h"
#include "interfaces.h"
#include "basecontrol.h"

void NSMenu::CDefaultStyle::Dialog(int iX, int iY, unsigned int iWidth, unsigned int iHeight)
{
	NSCore::CDrawer::DrawOutlinedRectangle(iX, iY, iWidth, iHeight, Color(7, 151, 151, 255));
	NSCore::CDrawer::DrawFilledRectangle(iX + 1, iY + 1, iWidth - 2, iHeight - 2, Color(29, 47, 64, 255));
}

unsigned int NSMenu::CDefaultStyle::ControlPanel(int iX, int iY, unsigned int iWidth, unsigned int iHeight, const char * pName)
{
	NSCore::CDrawer::DrawOutlinedRectangle(iX, iY, iWidth, iHeight, Color(50, 50, 55, 255));

	unsigned int iTextMargin = 0;

	int iTextW = 0, iTextH = 15;
	if (pName)
	{
		wchar_t * pTemp = new wchar_t[strlen(pName) + 1];
		mbstowcs(pTemp, pName, strlen(pName) + 1);
		NSInterfaces::g_pSurface->GetTextSize(NSCore::NSDrawUtils::g_iFontArial, pTemp, iTextW, iTextH);
		delete[] pTemp;
	}

	NSCore::CDrawer::DrawFilledRectangle(iX + 1, iY + 1, iWidth - 2, iTextH + iTextMargin, Color(25, 25, 25, 255));
	if (pName)
		NSCore::CDrawer::DrawString(iX + ((iWidth / 2) - (iTextW / 2)), iY, Color(150, 150, 150, 255), pName, NSCore::NSDrawUtils::g_iFontArial);

	NSCore::CDrawer::DrawLine(iX, iY + iTextH + iTextMargin, iX + iWidth, iY + iTextH + iTextMargin, Color(50, 50, 55, 255));
	iX += 1;
	iY += iTextH + iTextMargin + 1;
	iWidth -= 2;
	iHeight -= iTextH + iTextMargin + 2;

	NSCore::CDrawer::DrawFilledRectangle(iX, iY, iWidth, iHeight, Color(30, 30, 33, 255));

	return iTextH + iTextMargin + 1;
}

unsigned int NSMenu::CDefaultStyle::DialogButton(int iX, int iY, unsigned int iWidth, const char * pText, bool bMouseOver)
{
	NSCore::CDrawer::DrawFilledRectangle(iX, iY, iWidth, 15, Color(29, 47, 64, 255));
	NSCore::CDrawer::DrawOutlinedRectangle(iX, iY, iWidth, 15, Color(7, 151, 151, 255));

	NSCore::CDrawer::DrawString(iX + 3, iY, bMouseOver ? HTEXT : CTEXT, pText, NSCore::NSDrawUtils::g_iFontArial);

	return 15;
}

unsigned int NSMenu::CDefaultStyle::TopBar(int iX, int iY, unsigned int iWidth, const char * pTitle)
{
	// Dark topbar
	NSCore::CDrawer::DrawFilledRectangle(iX, iY, iWidth, TOPBAR_HEIGHT, Color(7, 151, 151, 255));

	if (pTitle)
		NSCore::CDrawer::DrawString(iX + 10, iY, CTEXT, pTitle, NSCore::NSDrawUtils::g_iFontArial);

	return TOPBAR_HEIGHT;
}