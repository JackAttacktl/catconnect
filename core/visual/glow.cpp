#include <stdint.h>
#include <memory>
#include "glow.h"
#include "tier1/KeyValues.h"
#include "icliententity.h"
#include "public/imaterialsystemfixed.h"
#include "materialsystem/itexture.h"
#include "materialsystem/imaterialvar.h"
#include "icliententitylist.h"
#include "cbasecombatweapon.h"
#include "catconnect.h"
#include "ivrenderview.h"
#include "ivmodelrender.h"
#include "settings/settings.h"

NSCore::CSetting glow_showoncats(xorstr_("catconnect.glow.show.cats"), xorstr_("10"));
NSCore::CSetting glow_how(xorstr_("catconnect.glow.howrender"), xorstr_("1"));

CScreenSpaceEffectRegistration * CScreenSpaceEffectRegistration::s_pHead = nullptr;
IScreenSpaceEffectManager * g_pScreenSpaceEffects = nullptr;
CScreenSpaceEffectRegistration ** g_ppScreenSpaceRegistrationHead  = nullptr;

static CScreenSpaceEffectRegistration * g_pGlowEffect = nullptr;

struct SShaderStencilState
{
    bool m_bEnable;
    StencilOperation_t m_FailOp;
    StencilOperation_t m_ZFailOp;
    StencilOperation_t m_PassOp;
    StencilComparisonFunction_t m_CompareFunc;
    int m_nReferenceValue;
    uint32 m_nTestMask;
    uint32 m_nWriteMask;

    SShaderStencilState()
    {
        Reset();
    }

    inline void Reset()
    {
        m_bEnable = false;
        m_PassOp = m_FailOp = m_ZFailOp = STENCILOPERATION_KEEP;
        m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
        m_nReferenceValue = 0;
        m_nTestMask = m_nWriteMask = 0xFFFFFFFF;
    }

    inline void SetStencilState(CMatRenderContextPtr &pRenderContext) const
    {
        pRenderContext->SetStencilEnable(m_bEnable);
        pRenderContext->SetStencilFailOperation(m_FailOp);
        pRenderContext->SetStencilZFailOperation(m_ZFailOp);
        pRenderContext->SetStencilPassOperation(m_PassOp);
        pRenderContext->SetStencilCompareFunction(m_CompareFunc);
        pRenderContext->SetStencilReferenceValue(m_nReferenceValue);
        pRenderContext->SetStencilTestMask(m_nTestMask);
        pRenderContext->SetStencilWriteMask(m_nWriteMask);
    }
};

static CTextureReference s_CTextBuffers[2];

ITexture * GetBuffer(int i)
{
    if (!s_CTextBuffers[i])
    {
        ITexture * pFullFrame;
        pFullFrame = NSInterfaces::g_pMaterialSystemFixed->FindTexture(xorstr_("_rt_FullFrameFB"), xorstr_(TEXTURE_GROUP_RENDER_TARGET));
        std::unique_ptr<char[]> pNewName(new char[32]);
        std::string sName = xorstr_("_cat_buff") + std::to_string(i);
        strncpy(pNewName.get(), sName.c_str(), 30);

        int iTextureFlags = TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT | TEXTUREFLAGS_EIGHTBITALPHA;
        int iRenderTargetFlags = CREATERENDERTARGETFLAGS_HDR;

        ITexture * pTexture;
        pTexture = NSInterfaces::g_pMaterialSystemFixed->CreateNamedRenderTargetTextureEx(pNewName.get(), pFullFrame->GetActualWidth(), pFullFrame->GetActualHeight(), (RenderTargetSizeMode_t)RT_SIZE_LITERAL, IMAGE_FORMAT_RGBA8888, (MaterialRenderTargetDepth_t)MATERIAL_RT_DEPTH_SEPARATE, iTextureFlags, iRenderTargetFlags);
        s_CTextBuffers[i].Init(pTexture);
    }
    return s_CTextBuffers[i];
}

static SShaderStencilState SS_NeverSolid;
static SShaderStencilState SS_InvisibleSolid;
static SShaderStencilState SS_Null;
static SShaderStencilState SS_Drawing;

CScreenSpaceEffectRegistration::CScreenSpaceEffectRegistration(const char * pName, IScreenSpaceEffect * pEffect)
{
    m_pEffectName = pName;
    m_pEffect = pEffect;
    m_pNext = *g_ppScreenSpaceRegistrationHead;
    *g_ppScreenSpaceRegistrationHead = this;
}

void NSCore::CGlowEffect::Init()
{
    if (m_bInited)
        return;

    KeyValues * pKV = nullptr;

    pKV = new KeyValues(xorstr_("UnlitGeneric"));
    pKV->SetString(xorstr_("$basetexture"), xorstr_("vgui/white_additive"));
    pKV->SetInt(xorstr_("$ignorez"), 0);
    m_CMatUnlit.Init(xorstr_("__cat_glow_unlit"), pKV);
    m_CMatUnlit->Refresh();

    pKV = new KeyValues(xorstr_("UnlitGeneric"));
    pKV->SetString(xorstr_("$basetexture"), xorstr_("vgui/white_additive"));
    pKV->SetInt(xorstr_("$ignorez"), 1);
    m_CMatUnlitZ.Init(xorstr_("__cat_glow_unlit_z"), pKV);
    m_CMatUnlitZ->Refresh();

    // Initialize 2 buffers
    GetBuffer(0);
    GetBuffer(1);

    pKV = new KeyValues(xorstr_("UnlitGeneric"));
    pKV->SetString(xorstr_("$basetexture"), xorstr_("_cat_buff0"));
    pKV->SetInt(xorstr_("$additive"), 1);
    m_CMatBlit.Init(xorstr_("__cat_glow_blit"), xorstr_(TEXTURE_GROUP_CLIENT_EFFECTS), pKV);
    m_CMatBlit->Refresh();

    pKV = new KeyValues(xorstr_("BlurFilterX"));
    pKV->SetString(xorstr_("$basetexture"), xorstr_("_cat_buff0"));
    pKV->SetInt(xorstr_("$ignorez"), 1);
    pKV->SetInt(xorstr_("$translucent"), 1);
    pKV->SetInt(xorstr_("$alphatest"), 1);
    m_CMatBlurX.Init(xorstr_("_cat_blurx"), pKV);
    m_CMatBlurX->Refresh();

    pKV = new KeyValues(xorstr_("BlurFilterY"));
    pKV->SetString(xorstr_("$basetexture"), xorstr_("_cat_buff1"));
    pKV->SetInt(xorstr_("$bloomamount"), 5);
    pKV->SetInt(xorstr_("$ignorez"), 1);
    pKV->SetInt(xorstr_("$translucent"), 1);
    pKV->SetInt(xorstr_("$alphatest"), 1);
    m_CMatBlurY.Init(xorstr_("_cat_blury"), pKV);
    m_CMatBlurY->Refresh();

    SS_NeverSolid.m_bEnable = true;
    SS_NeverSolid.m_PassOp = STENCILOPERATION_REPLACE;
    SS_NeverSolid.m_FailOp = STENCILOPERATION_KEEP;
    SS_NeverSolid.m_ZFailOp = STENCILOPERATION_KEEP;
    SS_NeverSolid.m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
    SS_NeverSolid.m_nWriteMask = 1;
    SS_NeverSolid.m_nReferenceValue = 1;

    SS_InvisibleSolid.m_bEnable = true;
    SS_InvisibleSolid.m_PassOp = STENCILOPERATION_REPLACE;
    SS_InvisibleSolid.m_FailOp = STENCILOPERATION_KEEP;
    SS_InvisibleSolid.m_ZFailOp = STENCILOPERATION_KEEP;
    SS_InvisibleSolid.m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
    SS_InvisibleSolid.m_nWriteMask = 1;
    SS_InvisibleSolid.m_nReferenceValue = 1;

    SS_Drawing.m_bEnable = true;
    SS_Drawing.m_nReferenceValue = 0;
    SS_Drawing.m_nTestMask = 1;
    SS_Drawing.m_CompareFunc = STENCILCOMPARISONFUNCTION_EQUAL;
    SS_Drawing.m_PassOp = STENCILOPERATION_ZERO;

    m_bInited = true;
}

void NSCore::CGlowEffect::Shutdown()
{
    if (!m_bInited)
        return;

    m_CMatUnlit.Shutdown();
    m_CMatUnlitZ.Shutdown();
    m_CMatBlit.Shutdown();
    m_CMatBlurX.Shutdown();
    m_CMatBlurY.Shutdown();
    m_bInited = false;
}

NSCore::SRGBA NSCore::CGlowEffect::GlowColor(IClientNetworkable * pClientNetworkable)
{
    auto GetColorFromRGBA = [](SRGBA sClr) -> SRGBA
    {
        sClr.r /= 255.0;
        sClr.g /= 255.0;
        sClr.b /= 255.0;
        sClr.a /= 255.0;
        return sClr;
    };

    IClientUnknown * pUnk = pClientNetworkable->GetIClientUnknown();
    static const SRGBA SDefaultColor = SRGBA{ 1.0, 1.0, 1.0, 1.0 };
    static SRGBA SReturnColor = SDefaultColor;
    if (!pUnk || pClientNetworkable->IsDormant()) return SDefaultColor;
    NSReclass::CBaseEntity * pEnt = (NSReclass::CBaseEntity *)pUnk->GetBaseEntity();
    if (!pEnt) return SDefaultColor;

    if (((NSReclass::CBaseCombatWeapon *)pEnt)->IsBaseCombatWeapon())
        return SDefaultColor; //white color for weapons

    if (pEnt->IsPlayer())
    {
        SReturnColor = GetColorFromRGBA(SRGBA(CCatConnect::GetStateColor(CatState_Cat)));
        return SReturnColor; //we draw only cats
    }
    
    return SDefaultColor;
}

bool NSCore::CGlowEffect::ShouldRenderGlow(IClientNetworkable * pClientNetworkable)
{
    if (!glow_showoncats.GetBool() || !glow_how.GetBool())
        return false;

    if (pClientNetworkable->entindex() < 0)
        return false;

    IClientUnknown * pUnk = pClientNetworkable->GetIClientUnknown();

    if (!pUnk || pClientNetworkable->IsDormant())
        return false;

    NSReclass::CBaseEntity * pEnt = (NSReclass::CBaseEntity *)pUnk->GetBaseEntity();

    if (!pEnt)
        return false;

    if (!pEnt->IsPlayer() && !((NSReclass::CBaseCombatWeapon *)pEnt)->IsBaseCombatWeapon())
        return false; //draw only cats and their weapons

    if (pEnt->IsPlayer())
    {
        ECatState eState = CCatConnect::GetClientState(pClientNetworkable->entindex());
        if (eState != ECatState::CatState_Cat)
            return false;
        if (!pEnt->IsPlayerAlive())
            return false;
        return true;
    }
    else //weapon
    {
        int iIndex = 0;
        NSReclass::CBaseEntity * pOwner = pEnt->GetOwner(&iIndex);
        if (!pOwner || !iIndex || iIndex > MAX_EDICTS)
            return false;
        return ShouldRenderGlow(NSInterfaces::g_pClientEntityList->GetClientNetworkable(iIndex));
    }
}

void NSCore::CGlowEffect::BeginRenderGlow()
{
    m_bDrawing = true;
    CMatRenderContextPtr pRenderContext(NSInterfaces::g_pMaterialSystemFixed->GetRenderContext());
    pRenderContext->ClearColor4ub(0, 0, 0, 0);
    pRenderContext->PushRenderTargetAndViewport();
    NSInterfaces::g_pModelRender->SuppressEngineLighting(true);
    pRenderContext->SetRenderTarget(GetBuffer(0));
    pRenderContext->OverrideAlphaWriteEnable(true, true);
    NSInterfaces::g_pRenderView->SetBlend(0.99f);
    pRenderContext->ClearBuffers(true, false);
    m_CMatUnlitZ->AlphaModulate(1.0f);
    pRenderContext->DepthRange(0.0f, 0.01f);
}

void NSCore::CGlowEffect::EndRenderGlow()
{
    m_bDrawing = false;
    CMatRenderContextPtr ptr(NSInterfaces::g_pMaterialSystemFixed->GetRenderContext());
    ptr->DepthRange(0.0f, 1.0f);
    NSInterfaces::g_pModelRender->ForcedMaterialOverride(nullptr);
    NSInterfaces::g_pModelRender->SuppressEngineLighting(false);
    ptr->PopRenderTargetAndViewport();
}

void NSCore::CGlowEffect::StartStenciling()
{
    static SShaderStencilState sState;
    sState.Reset();
    sState.m_bEnable = true;
    CMatRenderContextPtr pRenderContext(NSInterfaces::g_pMaterialSystemFixed->GetRenderContext());
    NSInterfaces::g_pRenderView->SetBlend(0.0f);
    switch (glow_how.GetInt())
    {
        case 1:
            SS_NeverSolid.SetStencilState(pRenderContext);
            pRenderContext->DepthRange(0.0f, 0.01f);
            NSInterfaces::g_pModelRender->ForcedMaterialOverride(m_CMatUnlitZ);
            break;
        case 2:
            SS_InvisibleSolid.SetStencilState(pRenderContext);
            pRenderContext->DepthRange(0.0f, 1.0f);
            m_CMatUnlit->AlphaModulate(1.0f);
            NSInterfaces::g_pModelRender->ForcedMaterialOverride(m_CMatUnlit);
            break;
    }
}

void NSCore::CGlowEffect::EndStenciling()
{
    static SShaderStencilState sState;
    sState.Reset();
    NSInterfaces::g_pModelRender->ForcedMaterialOverride(nullptr);
    CMatRenderContextPtr pRenderContext(NSInterfaces::g_pMaterialSystemFixed->GetRenderContext());
    sState.SetStencilState(pRenderContext);
    pRenderContext->DepthRange(0.0f, 1.0f);
    NSInterfaces::g_pRenderView->SetBlend(1.0f);
}

void NSCore::CGlowEffect::DrawToStencil(IClientNetworkable * pEnt) { DrawEntity(pEnt); }
void NSCore::CGlowEffect::DrawToBuffer(IClientNetworkable * pEnt) { }

void NSCore::CGlowEffect::RenderGlow(IClientNetworkable * pEnt)
{
    CMatRenderContextPtr ptr(NSInterfaces::g_pMaterialSystemFixed->GetRenderContext());
    NSInterfaces::g_pRenderView->SetColorModulation((const float *)&GlowColor(pEnt));
    NSInterfaces::g_pModelRender->ForcedMaterialOverride(m_CMatUnlitZ);
    DrawEntity(pEnt);
}

void NSCore::CGlowEffect::DrawEntity(IClientNetworkable * pEnt)
{
    static IClientEntity * pAttach;
    static int iPasses;
    iPasses = 0;

    IClientEntity * pClEnt = pEnt->GetIClientUnknown()->GetIClientEntity();
    pClEnt->DrawModel(1);
    static unsigned int s_iCollisionOffset = 0;
    if (!s_iCollisionOffset)
        s_iCollisionOffset = NETVARS_GET_OFFSET(xorstr_("DT_BaseEntity"), xorstr_("m_Collision"));
    pAttach = NSInterfaces::g_pClientEntityList->GetClientEntity(*(int *)((char *)pClEnt + s_iCollisionOffset - 24) & 0xFFF);
    while (pAttach && iPasses++ < 32)
    {
        if (pAttach->ShouldDraw())
        {
            if (((NSReclass::CBaseCombatWeapon *)pAttach)->IsBaseCombatWeapon())
            {
                float flTemp[4], flWhite[4] = { 255.0, 255.0, 255.0, 255.0 };
                NSInterfaces::g_pRenderView->GetColorModulation(flTemp);
                NSInterfaces::g_pRenderView->SetColorModulation(flWhite);
                pAttach->DrawModel(1);
                NSInterfaces::g_pRenderView->SetColorModulation(flTemp);
            }
            else
            {
                pAttach->DrawModel(1);
            }
        }
        pAttach = NSInterfaces::g_pClientEntityList->GetClientEntity(*(int *)((char *)pAttach + s_iCollisionOffset - 20) & 0xFFF);
    }
}

void NSCore::CGlowEffect::Render(int iX, int iY, int iWidth, int iHeight)
{
    if (!glow_showoncats.GetBool() || !glow_how.GetBool())
        return;

    static ITexture * pOriginal;
    static IClientEntity * pEnt;
    static IMaterialVar * pBluryBloomamout;
    //static IMaterialVar * pBlurXBloomscale;

    if (!m_bInited)
        Init();

    CMatRenderContextPtr pRenderContext(NSInterfaces::g_pMaterialSystemFixed->GetRenderContext());
    pOriginal = pRenderContext->GetRenderTarget();

    BeginRenderGlow();
    for (int i = 1; i <= MAX_EDICTS - 1; i++)
    {
        pEnt = NSInterfaces::g_pClientEntityList->GetClientEntity(i);
        if (pEnt && !pEnt->IsDormant() && ShouldRenderGlow(pEnt))
            RenderGlow(pEnt->GetClientNetworkable());
    }
    EndRenderGlow();

    pRenderContext->ClearStencilBufferRectangle(iX, iY, iWidth, iHeight, 0);

    StartStenciling();
    for (int i = 1; i <= MAX_EDICTS - 1; i++)
    {
        pEnt = NSInterfaces::g_pClientEntityList->GetClientEntity(i);
        if (pEnt && !pEnt->IsDormant() && ShouldRenderGlow(pEnt))
            DrawToStencil(pEnt);
    }
    EndStenciling();

    pRenderContext->SetRenderTarget(GetBuffer(1));
    pRenderContext->Viewport(iX, iY, iWidth, iHeight);
    pRenderContext->ClearBuffers(true, false);
    pRenderContext->DrawScreenSpaceRectangle(m_CMatBlurX, iX, iY, iWidth, iHeight, 0, 0, iWidth - 1, iHeight - 1, iWidth, iHeight);
    pRenderContext->SetRenderTarget(GetBuffer(0));

    pBluryBloomamout = m_CMatBlurY->FindVar(xorstr_("$bloomamount"), nullptr);
    pBluryBloomamout->SetFloatValue(glow_showoncats.GetFloat() / 2);
    //pBlurXBloomscale = m_CMatBlurX->FindVar(xorstr_("$bloomscale"), nullptr);
    //pBlurXBloomscale->SetFloatValue(glow_showoncats.GetFloat() / 2);

    pRenderContext->DrawScreenSpaceRectangle(m_CMatBlurY, iX, iY, iWidth, iHeight, 0, 0, iWidth - 1, iHeight - 1, iWidth, iHeight);
    pRenderContext->Viewport(iX, iY, iWidth, iHeight);
    pRenderContext->SetRenderTarget(pOriginal);

    NSInterfaces::g_pRenderView->SetBlend(0.0f);
    SS_Drawing.SetStencilState(pRenderContext);
    pRenderContext->DrawScreenSpaceRectangle(m_CMatBlit, iX, iY, iWidth, iHeight, 0, 0, iWidth - 1, iHeight - 1, iWidth, iHeight);
    SS_Null.SetStencilState(pRenderContext);
}

void NSCore::GlowInit()
{
    g_pGlowEffect = new CScreenSpaceEffectRegistration(xorstr_("_catglow"), &m_CGlowEffect);
    g_pScreenSpaceEffects->EnableScreenSpaceEffect(xorstr_("_catglow")); //we also need to call DisableScreenSpaceEffect. Also better do it in OnMapStart
}

void NSCore::GlowCreate()
{
    m_CGlowEffect.Init();
}

void NSCore::GlowDestroy()
{
    m_CGlowEffect.Shutdown();
}

NSCore::CGlowEffect NSCore::m_CGlowEffect;