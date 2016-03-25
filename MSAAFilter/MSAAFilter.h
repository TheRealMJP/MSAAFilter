//=================================================================================================
//
//  MSAA Filtering 2.0 Sample
//  by MJP
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#pragma once

#include <PCH.h>

#include <App.h>
#include <InterfacePointers.h>
#include <Graphics\\Camera.h>
#include <Graphics\\Model.h>
#include <Graphics\\SpriteFont.h>
#include <Graphics\\SpriteRenderer.h>
#include <Graphics\\Skybox.h>
#include <Graphics\\GraphicsTypes.h>
#include <Graphics\\ShaderCompilation.h>

#include "PostProcessor.h"
#include "MeshRenderer.h"

using namespace SampleFramework11;

class MSAAFilter : public App
{

protected:

    FirstPersonCamera camera;

    SpriteFont font;
    SampleFramework11::SpriteRenderer spriteRenderer;
    Skybox skybox;

    PostProcessor postProcessor;
    DepthStencilBuffer depthBuffer;
    RenderTarget2D colorTarget;
    RenderTarget2D resolveTarget;
    RenderTarget2D prevFrameTarget;
    RenderTarget2D velocityTarget;
    uint64 frameCount = 0;

    // Model
    Model models[uint64(Scenes::NumValues)];
    MeshRenderer meshRenderer;

    Float4x4 modelTransform;
    Quaternion modelOrientations[uint64(Scenes::NumValues)];

    ID3D11ShaderResourceViewPtr envMap;
    SH9Color envMapSH;

    VertexShaderPtr resolveVS;
    PixelShaderPtr resolvePS[uint64(MSAAModes::NumValues)];

    VertexShaderPtr backgroundVelocityVS;
    PixelShaderPtr backgroundVelocityPS;
    Float4x4 prevViewProjection;

    Float2 jitterOffset;
    Float2 prevJitter;

    struct ResolveConstants
    {
        uint32 SampleRadius;
        Float2 TextureSize;
    };

    struct BackgroundVelocityConstants
    {
        Float4x4 InvViewProjection;
        Float4x4 PrevViewProjection;
        Float2 RTSize;
        Float2 JitterOffset;
    };

    ConstantBuffer<ResolveConstants> resolveConstants;
    ConstantBuffer<BackgroundVelocityConstants> backgroundVelocityConstants;

    virtual void Initialize() override;
    virtual void Render(const Timer& timer) override;
    virtual void Update(const Timer& timer) override;
    virtual void BeforeReset() override;
    virtual void AfterReset() override;

    void CreateRenderTargets();

    void RenderScene();
    void RenderBackgroundVelocity();
    void RenderAA();
    void RenderHUD();

public:

    MSAAFilter();
};
