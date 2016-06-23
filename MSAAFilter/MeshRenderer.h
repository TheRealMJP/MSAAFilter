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

#include <Graphics\\Model.h>
#include <Graphics\\GraphicsTypes.h>
#include <Graphics\\DeviceStates.h>
#include <Graphics\\Camera.h>
#include <Graphics\\SH.h>
#include <Graphics\\ShaderCompilation.h>

#include "AppSettings.h"

using namespace SampleFramework11;

struct BakeData;

class MeshRenderer
{

protected:

    // Constants
    static const uint32 NumCascades = 4;
    static const uint32 ReadbackLatency = 1;

public:

    void Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    void SetModel(const Model* model);

    void RenderDepth(ID3D11DeviceContext* context, const Camera& camera, const Float4x4& world,
                     bool shadowRendering);
    void Render(ID3D11DeviceContext* context, const Camera& camera, const Float4x4& world,
                ID3D11ShaderResourceView* envMap, const SH9Color& envMapSH,
                Float2 JitterOffset);

    void Update();

    void CreateReductionTargets(uint32 width, uint32 height);

    void ReduceDepth(ID3D11DeviceContext* context, DepthStencilBuffer& depthBuffer,
                     const Camera& camera);

    void ComputeShadowDepthBounds(const Camera& camera);

    void RenderShadowMap(ID3D11DeviceContext* context, const Camera& camera,
                         const Float4x4& world);

protected:

    void LoadShaders();
    void CreateShadowMaps();
    void ConvertToEVSM(ID3D11DeviceContext* context, uint32 cascadeIdx, Float3 cascadeScale);

    ID3D11DevicePtr device;

    BlendStates blendStates;
    RasterizerStates rasterizerStates;
    DepthStencilStates depthStencilStates;
    SamplerStates samplerStates;

    const Model* model = nullptr;

    DepthStencilBuffer shadowMap;
    RenderTarget2D  varianceShadowMap;
    RenderTarget2D tempVSM;

    ID3D11RasterizerStatePtr shadowRSState;
    ID3D11SamplerStatePtr evsmSampler;

    std::vector<ID3D11InputLayoutPtr> meshInputLayouts;
    VertexShaderPtr meshVS[2];
    PixelShaderPtr meshPS[2][2];

    std::vector<ID3D11InputLayoutPtr> meshDepthInputLayouts;
    VertexShaderPtr meshDepthVS;

    VertexShaderPtr fullScreenVS;
    PixelShaderPtr evsmConvertPS;
    PixelShaderPtr evsmBlurH;
    PixelShaderPtr evsmBlurV;

    ComputeShaderPtr depthReductionInitialCS[2];
    ComputeShaderPtr depthReductionCS;
    std::vector<RenderTarget2D> depthReductionTargets;
    StagingTexture2D reductionStagingTextures[ReadbackLatency];
    uint32 currFrame = 0;

    Float2 shadowDepthBounds = Float2(0.0f, 1.0f);

    ID3D11ShaderResourceViewPtr specularLookupTexture;

    Float4x4 prevWVP;

    ID3D11SamplerStatePtr mipBiasSampler;
    float currMipBias = 0.0f;

    // Constant buffers
    struct MeshVSConstants
    {
        Float4Align Float4x4 World;
        Float4Align Float4x4 View;
        Float4Align Float4x4 WorldViewProjection;
        Float4Align Float4x4 PrevWorldViewProjection;
    };

    struct MeshPSConstants
    {
        Float4Align Float3 CameraPosWS;

        Float4Align Float4x4 ShadowMatrix;
        Float4Align float CascadeSplits[NumCascades];

        Float4Align Float4 CascadeOffsets[NumCascades];
        Float4Align Float4 CascadeScales[NumCascades];

        float OffsetScale;
        float PositiveExponent;
        float NegativeExponent;
        float LightBleedingReduction;

        Float4Align Float4x4 Projection;

        Float4Align ShaderSH9Color EnvironmentSH;

        Float2 RTSize;
        Float2 JitterOffset;
    };

    struct EVSMConstants
    {
        Float3 CascadeScale;
        float PositiveExponent;
        float NegativeExponent;
        float FilterSize;
        Float2 ShadowMapSize;
    };

    struct ReductionConstants
    {
        Float4x4 Projection;
        float NearClip;
        float FarClip;
        Uint2 TextureSize;
        uint32 NumSamples;
    };

    ConstantBuffer<MeshVSConstants> meshVSConstants;
    ConstantBuffer<MeshPSConstants> meshPSConstants;
    ConstantBuffer<EVSMConstants> evsmConstants;
    ConstantBuffer<ReductionConstants> reductionConstants;
};