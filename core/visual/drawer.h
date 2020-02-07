#ifndef __DRAWER_INCLUDED_
#define __DRAWER_INCLUDED_

#include "Color.h"
#include "defs.h"
#include "vgui/ISurfaceV30.h"

namespace NSCore
{
	namespace NSDrawUtils
	{
		extern vgui::HFont g_iFontArial;
		//extern int g_iTexture;
		extern int g_iScreenWidth;
		extern int g_iScreenHeight;
	};

	class CDrawer
	{
	public:
		static void Init();
		static void Reload();

		static void FASTERCALL DrawString(int iX, int iY, Color cColor, const char * pText, vgui::HFont ulFont);
		static void FASTERCALL DrawLine(int iX1, int iY1, int iX2, int iY2, Color cColor);
		static void FASTERCALL DrawFilledRectangle(int iX, int iY, int iWidth, int iHeight, Color cColor);
		static void FASTERCALL DrawOutlinedRectangle(int iX, int iY, int iWidth, int iHeight, Color cColor);
	};
};

#endif