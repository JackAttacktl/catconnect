#ifndef _MENU_MAIN_INC_
#define _MENU_MAIN_INC_

#include <Windows.h>
#include "defs.h"
#include "styles.h"
#include "tabgroup.h"
#include "inputsystem/buttoncode.h"

enum class EMouseButton
{
	MB_None,
	MB_LeftClick,
	MB_RightClick,
	MB_LeftDown,
	MB_RightDown
};

namespace NSMenu
{
	typedef void (__cdecl * DrawFunctionFn)(int iX, int iY, unsigned int iW, unsigned int iH);
	typedef void (__cdecl * DrawDialogFn)(void * pData, unsigned int iIndex);

	class CDialog
	{
	public:
		CDialog(DrawDialogFn pDrawFn, unsigned int iWidth = 200, unsigned int iHeight = 200)
		{
			m_pDrawFunc = pDrawFn;
			m_iWidth = iWidth;
			m_iHeight = iHeight;
			m_pData = nullptr;
		}

		inline void SetData(void * pData) { m_pData = pData; }
		inline void * GetData() const { return m_pData; }
		inline unsigned int GetWidth() const { return m_iWidth; }
		inline unsigned int GetHeight() const { return m_iHeight; }
		inline DrawDialogFn GetDrawFunction() const { return m_pDrawFunc; }

	private:
		unsigned int m_iWidth;
		unsigned int m_iHeight;
		DrawDialogFn m_pDrawFunc;
		void * m_pData;
	};

	class CMenu
	{
	public:
		CMenu();
		~CMenu();

		bool FASTERCALL IsMouseOver(int iX, int iY, unsigned int iWidth, unsigned int iHeight);

		void Draw();
		void CreateGUI();
		void FASTERCALL GetInput(ButtonCode_t iKey);
		void EndInput();

		void FASTERCALL OpenDialog(CDialog * pDialog);
		void FASTERCALL CloseDialog(unsigned int iIndex);

		inline void ToggleEnabled() { m_bEnabled = !m_bEnabled; }
		inline EMouseButton GetMouseButton() const { return m_EMouseButton; }
		inline bool IsDialogOpen() const { return !!m_vDialogs.size(); } //TODO
		inline unsigned int GetFocus() const { return m_iFocus; }
		inline IWindowStyle * GetStyle() const { return m_pMyStyle; }
		inline bool IsEnabled() const { return m_bEnabled; }

	private:
		void FASTERCALL FindNeededElementsRecursively(CBaseControl * pElement, std::vector<CBaseControl *> & vOut);
		void BindSettings();

	private:
		bool m_bEnabled;
		bool m_bIsOnDrag;
		POINT m_pMouse;
		POINT m_pPrevMouse;
		POINT m_pMyPosition;
		POINT m_pMyScale;
		EMouseButton m_EMouseButton;
		ButtonCode_t m_iKey;
		CTabGroup * m_pTabs;
		IWindowStyle * m_pMyStyle;

		unsigned int m_iFocus;
		std::vector<CDialog *> m_vDialogs;
	};

	extern CMenu g_CMenu;
};

#endif