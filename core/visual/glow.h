#ifndef _GLOW_INC_
#define _GLOW_INC_

#include "materialsystem/MaterialSystemUtil.h"
#include "ScreenSpaceEffects.h"
#include "defs.h"
#include "Color.h"

class KeyValues;
class IClientNetworkable;

namespace NSCore
{
	struct SRGBA
	{
		inline SRGBA() : r(0.0), g(0.0), b(0.0), a(255.0) {}
		inline SRGBA(float flR, float flG, float flB) : r(flR), g(flG), b(flB), a(255.0) {}
		inline SRGBA(float flR, float flG, float flB, float flA) : r(flR), g(flG), b(flB), a(flA) {}
		inline SRGBA(Color clr) : r(clr.r()), g(clr.g()), b(clr.b()), a(clr.a()) {}
		union
		{
			struct { float r, g, b, a; };
			float flRGBA[4];
		};
	};

	class CGlowEffect : public IScreenSpaceEffect
	{
	public: //IFace
		CGlowEffect() : m_bInited(false), m_bDrawing(false), m_bEnabled(false) {}
		virtual void Init();
		virtual void Shutdown();
		virtual void SetParameters(KeyValues * pKV) {}
		virtual void Render(int iX, int iY, int iWidth, int iHeight);
		virtual void Enable(bool bEnable) { m_bEnabled = bEnable; }
		virtual bool IsEnabled() { return m_bEnabled; }

	public:
		void StartStenciling();
		void EndStenciling();
		void FASTERCALL DrawEntity(IClientNetworkable * pEntity);
		void FASTERCALL DrawToStencil(IClientNetworkable * pEntity);
		void FASTERCALL DrawToBuffer(IClientNetworkable * pEntity);
		SRGBA FASTERCALL GlowColor(IClientNetworkable * pEntity);
		bool FASTERCALL ShouldRenderGlow(IClientNetworkable * pEntity);
		void FASTERCALL RenderGlow(IClientNetworkable * pEntity);
		void BeginRenderGlow();
		void EndRenderGlow();

	private:
		bool m_bInited;
		bool m_bDrawing;
		bool m_bEnabled;
		CMaterialReference m_CMatBlit;
		CMaterialReference m_CMatBlurX;
		CMaterialReference m_CMatBlurY;
		CMaterialReference m_CMatUnlit;
		CMaterialReference m_CMatUnlitZ;
	};

	void GlowInit();
	void GlowCreate();
	void GlowDestroy();

	extern CGlowEffect m_CGlowEffect;
};

#endif