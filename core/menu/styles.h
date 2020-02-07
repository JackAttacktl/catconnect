#ifndef _MENU_STYLES_INC_
#define _MENU_STYLES_INC_

#define TAB_WIDTH 150
#define TOPBAR_HEIGHT 15
#define MENU_TOPBAR_HEIGHT (TOPBAR_HEIGHT + 5)

namespace NSMenu
{
	class IWindowStyle
	{
	public:
		virtual ~IWindowStyle() {}
		// Draws a framed control area for dialogs
		virtual void Dialog(int iX, int iY, unsigned int iWidth, unsigned int iHeight) = 0;
		// Draws a framed control area for small panels within another control area
		// - Returns the height of the namebar
		virtual unsigned int ControlPanel(int iX, int iY, unsigned int iWidth, unsigned int iHeight, const char * pName = nullptr) = 0;
		// Draws a button made to open dialogs
		// - Returns the height of the box
		virtual unsigned int DialogButton(int iX, int iY, unsigned int iWidth, const char * pText = nullptr, bool bMouseOver = false) = 0;
		// Draws a window top bar
		// - Returns the height of the bar
		virtual unsigned int TopBar(int iX, int iY, unsigned int iWidth, const char * pTitle = nullptr) = 0;
	};

	class CDefaultStyle : public IWindowStyle
	{
	public:
		CDefaultStyle() {}
		virtual ~CDefaultStyle() {}
		virtual void Dialog(int iX, int iY, unsigned int iWidth, unsigned int iHeight);
		virtual unsigned int ControlPanel(int iX, int iY, unsigned int iWidth, unsigned int iHeight, const char * pName = nullptr);
		virtual unsigned int DialogButton(int iX, int iY, unsigned int iWidth, const char * pText = nullptr, bool bMouseOver = false);
		virtual unsigned int TopBar(int iX, int iY, unsigned int iWidth, const char * pTitle = nullptr);
	};
};

#endif