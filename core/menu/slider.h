#ifndef _SLIDER_CONTROL_INC_
#define _SLIDER_CONTROL_INC_

#include "basecontrol.h"

namespace NSMenu
{
	class CMenu;

	class CSlider : public CBaseControl
	{
	public:
		CSlider(CMenu * pDrawer, const char * pName, const char * pSetting, int iMin, int iMax, int iStep, int iWidth = 100, int iX = 0, int iY = 0) : CBaseControl(pDrawer)
		{
			m_iType = EControlType::Control_Slider;
			m_iFlags = ControlFlag_Scale_Width;
			strcpy(m_cName, pName);
			strcpy(m_cSetting, pSetting);
			m_iValue = 0;
			m_iX = iX;
			m_iY = iY;
			m_iMin = iMin;
			m_iMax = iMax;
			m_iStep = iStep;
			m_iHeight = 24;
			m_iWidth = iWidth;
		}

	public: //iface
		virtual unsigned int Draw(bool bMouseOver = false);
		virtual void HandleInput();

	private:
		inline void RecheckValue()
		{
			if (m_iValue > m_iMax)
				m_iValue = m_iMin;
			else if (m_iValue < m_iMin)
				m_iValue = m_iMax;
		}

	private:
		int m_iValue;
		int m_iMin = 0;
		int m_iMax = 0;
		int m_iStep = 0;
	};
};

#endif