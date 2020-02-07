#include "drawer.h"
#include "interfaces.h"
#include "public/isurfacefixed.h"
#include "xorstr.h"
#include "vmt.h"
#include <cstring>

#ifdef CreateFont
#undef CreateFont
#endif

void NSCore::CDrawer::Init()
{
	NSDrawUtils::g_iFontArial = NSInterfaces::g_pSurface->CreateFont();
	//NSDrawUtils::g_iTexture = NSInterfaces::g_pSurface->CreateNewTextureID();
	Reload();
}

void NSCore::CDrawer::Reload()
{
	NSInterfaces::g_pSurface->SetFontGlyphSet(NSDrawUtils::g_iFontArial, xorstr_("Arial"), 15, 250, 0, 0, SurfaceV30::ISurface::EFontFlags::FONTFLAG_OUTLINE);
	//constexpr unsigned char cTexColor[] = { 170, 100, 64, 255 };
	//NSInterfaces::g_pSurface->DrawSetTextureRGBA(NSDrawUtils::g_iTexture, cTexColor, 1, 1, 0, false);
}

void NSCore::CDrawer::DrawString(int iX, int iY, Color cColor, const char * pText, vgui::HFont ulFont)
{
	wchar_t * pTemp = new wchar_t[strlen(pText) + 1];
	mbstowcs(pTemp, pText, strlen(pText) + 1);
	NSInterfaces::g_pSurface->DrawSetTextPos(iX, iY);
	NSInterfaces::g_pSurface->DrawSetTextFont(ulFont);
	NSInterfaces::g_pSurface->DrawSetTextColor(cColor);
	NSInterfaces::g_pSurface->DrawPrintText(pTemp, wcslen(pTemp));
	delete[] pTemp;
}

void NSCore::CDrawer::DrawLine(int iX1, int iY1, int iX2, int iY2, Color cColor)
{
	NSInterfaces::g_pSurface->DrawSetColor(cColor.r(), cColor.g(), cColor.b(), cColor.a());
	NSInterfaces::g_pSurface->DrawLine(iX1, iY1, iX2, iY2);
}

void NSCore::CDrawer::DrawFilledRectangle(int iX, int iY, int iWidth, int iHeight, Color cColor)
{
	NSInterfaces::g_pSurface->DrawSetColor(cColor.r(), cColor.g(), cColor.b(), cColor.a());
	NSInterfaces::g_pSurface->DrawFilledRect(iX, iY, iX + iWidth, iY + iHeight);
}

void NSCore::CDrawer::DrawOutlinedRectangle(int iX, int iY, int iWidth, int iHeight, Color cColor)
{
	NSInterfaces::g_pSurface->DrawSetColor(cColor.r(), cColor.g(), cColor.b(), cColor.a());
	NSInterfaces::g_pSurface->DrawOutlinedRect(iX, iY, iX + iWidth, iY + iHeight);
}

vgui::HFont NSCore::NSDrawUtils::g_iFontArial = 0;
//int NSCore::NSDrawUtils::g_iTexture = 0;
int NSCore::NSDrawUtils::g_iScreenWidth = 0;
int NSCore::NSDrawUtils::g_iScreenHeight = 0;