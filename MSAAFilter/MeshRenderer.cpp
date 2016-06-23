//=================================================================================================
//
//  MSAA Filtering 2.0 Sample
//  by MJP
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#include "PCH.h"

#include "MeshRenderer.h"

#include <Exceptions.h>
#include <Utility.h>
#include <Graphics\\ShaderCompilation.h>
#include <App.h>
#include <Graphics\\Textures.h>

#include "AppSettings.h"
#include "SharedConstants.h"

// Constants
static const float ShadowNearClip = 1.0f;
static const float FilterSize = 7.0f;
static const uint32 SampleRadius = 3;
static const float OffsetScale = 0.0f;
static const float LightBleedingReduction = 0.10f;
static const float PositiveExponent = 40.0f;
static const float NegativeExponent = 8.0f;
static const uint32 ShadowMapSize = 1024;
static const uint32 ShadowMSAASamples = 4;
static const uint32 ShadowAnisotropy = 16;
static const bool EnableShadowMips = true;

void MeshRenderer::LoadShaders()
{
    // Load the mesh shaders
    meshDepthVS = CompileVSFromFile(device, L"DepthOnly.hlsl", "VS", "vs_5_0");

    CompileOptions opts;
    opts.Add("UseNormalMapping_", 0);

    meshVS[0] = CompileVSFromFile(device, L"Mesh.hlsl", "VS", "vs_5_0", opts);

    opts.Reset();
    opts.Add("UseNormalMapping_", 1);
    meshVS[1] = CompileVSFromFile(device, L"Mesh.hlsl", "VS", "vs_5_0", opts);

    for(uint32 useNormalMapping = 0; useNormalMapping < 2; ++useNormalMapping)
    {
        for(uint32 centroidSampling = 0; centroidSampling < 2; ++centroidSampling)
        {
            opts.Reset();
            opts.Add("UseNormalMapping_", useNormalMapping);
            opts.Add("CentroidSampling_", centroidSampling);
            meshPS[useNormalMapping][centroidSampling] = CompilePSFromFile(device, L"Mesh.hlsl", "PS", "ps_5_0", opts);
        }
    }

    fullScreenVS = CompileVSFromFile(device, L"EVSMConvert.hlsl", "FullScreenVS");

    opts.Reset();
    opts.Add("MSAASamples_", ShadowMSAASamples);
    evsmConvertPS = CompilePSFromFile(device, L"EVSMConvert.hlsl", "ConvertToEVSM", "ps_5_0", opts);

    opts.Reset();
    opts.Add("Horizontal_", 1);
    opts.Add("Vertical_", 0);
    opts.Add("SampleRadius_", SampleRadius);
    evsmBlurH = CompilePSFromFile(device, L"EVSMConvert.hlsl", "BlurEVSM", "ps_5_0", opts);

    opts.Reset();
    opts.Add("Horizontal_", 0);
    opts.Add("Vertical_", 1);
    opts.Add("SampleRadius_", SampleRadius);
    evsmBlurV = CompilePSFromFile(device, L"EVSMConvert.hlsl", "BlurEVSM", "ps_5_0", opts);

    opts.Reset();
    opts.Add("MSAA_", 0);
    depthReductionInitialCS[0] = CompileCSFromFile(device, L"DepthReduction.hlsl", "DepthReductionInitialCS", "cs_5_0", opts);

    opts.Reset();
    opts.Add("MSAA_", 1);
    depthReductionInitialCS[1] = CompileCSFromFile(device, L"DepthReduction.hlsl", "DepthReductionInitialCS", "cs_5_0", opts);

    depthReductionCS = CompileCSFromFile(device, L"DepthReduction.hlsl", "DepthReductionCS");
}

void MeshRenderer::CreateShadowMaps()
{
    // Create the shadow map as a texture array
    shadowMap.Initialize(device, ShadowMapSize, ShadowMapSize, DXGI_FORMAT_D24_UNORM_S8_UINT, true,
                         ShadowMSAASamples, 0, 1);

    DXGI_FORMAT smFmt = DXGI_FORMAT_R32G32B32A32_FLOAT;
    uint32 numMips = EnableShadowMips ? 0 : 1;
    varianceShadowMap.Initialize(device, ShadowMapSize, ShadowMapSize, smFmt, numMips, 1, 0,
                                 EnableShadowMips, false, NumCascades, false);

    tempVSM.Initialize(device, ShadowMapSize, ShadowMapSize, smFmt, 1, 1, 0, false, false, 1, false);
}

void MeshRenderer::SetModel(const Model* model)
{
    this->model = model;

    meshInputLayouts.clear();
    meshDepthInputLayouts.clear();

    VertexShaderPtr vs = AppSettings::UseNormalMapping() ? meshVS[1] : meshVS[0];

    for(uint64 i = 0; i < model->Meshes().size(); ++i)
    {
        const Mesh& mesh = model->Meshes()[i];
        ID3D11InputLayoutPtr inputLayout;
        DXCall(device->CreateInputLayout(mesh.InputElements(), mesh.NumInputElements(),
               vs->ByteCode->GetBufferPointer(), vs->ByteCode->GetBufferSize(), &inputLayout));
        meshInputLayouts.push_back(inputLayout);

        DXCall(device->CreateInputLayout(mesh.InputElements(), mesh.NumInputElements(),
               meshDepthVS->ByteCode->GetBufferPointer(), meshDepthVS->ByteCode->GetBufferSize(), &inputLayout));
        meshDepthInputLayouts.push_back(inputLayout);
    }
}

// Loads resources
void MeshRenderer::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
    this->device = device;

    blendStates.Initialize(device);
    rasterizerStates.Initialize(device);
    depthStencilStates.Initialize(device);
    samplerStates.Initialize(device);

    meshVSConstants.Initialize(device);
    meshPSConstants.Initialize(device);
    evsmConstants.Initialize(device);
    reductionConstants.Initialize(device);

    LoadShaders();

    D3D11_RASTERIZER_DESC rsDesc = RasterizerStates::NoCullDesc();
    rsDesc.DepthClipEnable = false;
    DXCall(device->CreateRasterizerState(&rsDesc, &shadowRSState));

    D3D11_SAMPLER_DESC sampDesc = SamplerStates::AnisotropicDesc();
    sampDesc.MaxAnisotropy = ShadowAnisotropy;
    DXCall(device->CreateSamplerState(&sampDesc, &evsmSampler));

    // Create the staging textures for reading back the reduced depth buffer
    for(uint32 i = 0; i < ReadbackLatency; ++i)
        reductionStagingTextures[i].Initialize(device, 1, 1, DXGI_FORMAT_R16G16_UNORM);

    specularLookupTexture = LoadTexture(device, L"..\\Content\\Textures\\SpecularLookup.dds");

    CreateShadowMaps();
}

void MeshRenderer::Update()
{
}

void MeshRenderer::CreateReductionTargets(uint32 width, uint32 height)
{
    depthReductionTargets.clear();

    uint32 w = width;
    uint32 h = height;

    while(w > 1 || h > 1)
    {
        w = DispatchSize(ReductionTGSize, w);
        h = DispatchSize(ReductionTGSize, h);

        RenderTarget2D rt;
        rt.Initialize(device, w, h, DXGI_FORMAT_R16G16_UNORM, 1, 1, 0, false, true);
        depthReductionTargets.push_back(rt);
    }
}

void MeshRenderer::ReduceDepth(ID3D11DeviceContext* context, DepthStencilBuffer& depthBuffer,
                                const Camera& camera)
{
    PIXEvent event(L"Depth Reduction");

    reductionConstants.Data.Projection = Float4x4::Transpose(camera.ProjectionMatrix());
    reductionConstants.Data.NearClip = camera.NearClip();
    reductionConstants.Data.FarClip = camera.FarClip();
    reductionConstants.Data.TextureSize.x = depthBuffer.Width;
    reductionConstants.Data.TextureSize.y = depthBuffer.Height;
    reductionConstants.Data.NumSamples = depthBuffer.MultiSamples;
    reductionConstants.ApplyChanges(context);
    reductionConstants.SetCS(context, 0);

    ID3D11RenderTargetView* rtvs[1] = { NULL };
    context->OMSetRenderTargets(1, rtvs, NULL);

    ID3D11UnorderedAccessView* uavs[1] = { depthReductionTargets[0].UAView };
    context->CSSetUnorderedAccessViews(0, 1, uavs, NULL);

    ID3D11ShaderResourceView* srvs[1] = { depthBuffer.SRView };
    context->CSSetShaderResources(0, 1, srvs);

    const bool msaa = depthBuffer.MultiSamples > 1;

    ID3D11ComputeShader* shader = depthReductionInitialCS[msaa ? 1 : 0];
    context->CSSetShader(shader, NULL, 0);

    uint32 dispatchX = depthReductionTargets[0].Width;
    uint32 dispatchY = depthReductionTargets[0].Height;
    context->Dispatch(dispatchX, dispatchY, 1);

    uavs[0] = NULL;
    context->CSSetUnorderedAccessViews(0, 1, uavs, NULL);

    srvs[0] = NULL;
    context->CSSetShaderResources(0, 1, srvs);

    context->CSSetShader(depthReductionCS, NULL, 0);

    for(uint32 i = 1; i < depthReductionTargets.size(); ++i)
    {
        RenderTarget2D& srcTexture = depthReductionTargets[i - 1];
        reductionConstants.Data.TextureSize.x = srcTexture.Width;
        reductionConstants.Data.TextureSize.y = srcTexture.Height;
        reductionConstants.Data.NumSamples = srcTexture.MultiSamples;
        reductionConstants.ApplyChanges(context);

        uavs[0] = depthReductionTargets[i].UAView;
        context->CSSetUnorderedAccessViews(0, 1, uavs, NULL);

        srvs[0] = srcTexture.SRView;
        context->CSSetShaderResources(0, 1, srvs);

        dispatchX = depthReductionTargets[i].Width;
        dispatchY = depthReductionTargets[i].Height;
        context->Dispatch(dispatchX, dispatchY, 1);

        uavs[0] = NULL;
        context->CSSetUnorderedAccessViews(0, 1, uavs, NULL);

        srvs[0] = NULL;
        context->CSSetShaderResources(0, 1, srvs);
    }

    // Copy to a staging texture
    ID3D11Texture2D* lastTarget = depthReductionTargets[depthReductionTargets.size() - 1].Texture;
    context->CopyResource(reductionStagingTextures[currFrame % ReadbackLatency].Texture, lastTarget);

    ++currFrame;

    if(currFrame >= ReadbackLatency)
    {
        StagingTexture2D& stagingTexture = reductionStagingTextures[currFrame % ReadbackLatency];

        uint32 pitch;
        const uint16* texData = reinterpret_cast<uint16*>(stagingTexture.Map(context, 0, pitch));
        shadowDepthBounds.x = texData[0] / static_cast<float>(0xffff);
        shadowDepthBounds.y = texData[1] / static_cast<float>(0xffff);

        stagingTexture.Unmap(context, 0);
    }
    else
    {
        shadowDepthBounds = Float2(0.0f, 1.0f);
    }
}

// Computes shadow depth bounds on the CPU using the mesh vertex positions
void MeshRenderer::ComputeShadowDepthBounds(const Camera& camera)
{
    Float4x4 viewMatrix = camera.ViewMatrix();
    const float nearClip = camera.NearClip();
    const float farClip = camera.FarClip();
    const float clipDist = farClip - nearClip;

    float minDepth = 1.0f;
    float maxDepth = 0.0f;
    const uint64 numMeshes = model->Meshes().size();
    for(uint64 meshIdx = 0; meshIdx < numMeshes; ++meshIdx)
    {
        const Mesh& mesh = model->Meshes()[meshIdx];
        const uint64 numVerts = mesh.NumVertices();
        const uint64 stride = mesh.VertexStride();
        const uint8* vertices = mesh.Vertices();
        for(uint64 i = 0; i < numVerts; ++i)
        {
            const Float3& position = *reinterpret_cast<const Float3*>(vertices);
            float viewSpaceZ = Float3::Transform(position, viewMatrix).z;
            float depth = Saturate((viewSpaceZ - nearClip) / clipDist);
            minDepth = std::min(minDepth, depth);
            maxDepth = std::max(maxDepth, depth);
            vertices += stride;
        }
    }

    shadowDepthBounds = Float2(minDepth, maxDepth);
}

// Convert to an EVSM map
void MeshRenderer::ConvertToEVSM(ID3D11DeviceContext* context, uint32 cascadeIdx, Float3 cascadeScale)
{
    PIXEvent event(L"EVSM Conversion");

    float blendFactor[4] = {1, 1, 1, 1};
    context->OMSetBlendState(blendStates.BlendDisabled(), blendFactor, 0xFFFFFFFF);
    context->RSSetState(rasterizerStates.NoCull());
    context->OMSetDepthStencilState(depthStencilStates.DepthDisabled(), 0);
    ID3D11Buffer* vbs[1] = { NULL };
    uint32 strides[1] = { 0 };
    uint32 offsets[1] = { 0 };
    context->IASetVertexBuffers(0, 1, vbs, strides, offsets);
    context->IASetIndexBuffer(NULL, DXGI_FORMAT_R32_UINT, 0);
    context->IASetInputLayout(NULL);

    D3D11_VIEWPORT vp;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.Width = static_cast<float>(varianceShadowMap.Width);
    vp.Height = static_cast<float>(varianceShadowMap.Height);
    context->RSSetViewports(1, &vp);

    evsmConstants.Data.PositiveExponent = PositiveExponent;
    evsmConstants.Data.NegativeExponent = NegativeExponent;
    evsmConstants.Data.CascadeScale = meshPSConstants.Data.CascadeScales[cascadeIdx].To3D();
    evsmConstants.Data.FilterSize = 1.0f;
    evsmConstants.Data.ShadowMapSize.x = float(varianceShadowMap.Width);
    evsmConstants.Data.ShadowMapSize.y = float(varianceShadowMap.Height);
    evsmConstants.ApplyChanges(context);
    evsmConstants.SetPS(context, 0);

    context->VSSetShader(fullScreenVS, NULL, 0);
    context->PSSetShader(evsmConvertPS, NULL, 0);

    ID3D11RenderTargetView* rtvs[1] = { varianceShadowMap.RTVArraySlices[cascadeIdx] };
    context->OMSetRenderTargets(1, rtvs, NULL);

    ID3D11ShaderResourceView* srvs[1] = { shadowMap.SRView };
    context->PSSetShaderResources(0, 1, srvs);

    context->Draw(3, 0);

    srvs[0] = NULL;
    context->PSSetShaderResources(0, 1, srvs);

    const float FilterSizeU = std::max(FilterSize * cascadeScale.x, 1.0f);
    const float FilterSizeV = std::max(FilterSize * cascadeScale.y, 1.0f);

    if(FilterSizeU > 1.0f || FilterSizeV > 1.0f)
    {
        // Horizontal pass
        evsmConstants.Data.FilterSize = FilterSizeU;
        evsmConstants.ApplyChanges(context);

        uint32 sampleRadiusU = static_cast<uint32>((FilterSizeU / 2) + 0.499f);

        rtvs[0] = tempVSM.RTView;
        context->OMSetRenderTargets(1, rtvs, NULL);

        srvs[0] = varianceShadowMap.SRVArraySlices[cascadeIdx];
        context->PSSetShaderResources(0, 1, srvs);

        context->PSSetShader(evsmBlurH, NULL, 0);

        context->Draw(3, 0);

        srvs[0] = NULL;
        context->PSSetShaderResources(0, 1, srvs);

        // Vertical pass
        evsmConstants.Data.FilterSize = FilterSizeV;
        evsmConstants.ApplyChanges(context);

        uint32 sampleRadiusV = static_cast<uint32>((FilterSizeV / 2) + 0.499f);

        rtvs[0] = varianceShadowMap.RTVArraySlices[cascadeIdx];
        context->OMSetRenderTargets(1, rtvs, NULL);

        srvs[0] = tempVSM.SRView;
        context->PSSetShaderResources(0, 1, srvs);

        context->PSSetShader(evsmBlurV, NULL, 0);

        context->Draw(3, 0);

        srvs[0] = NULL;
        context->PSSetShaderResources(0, 1, srvs);
    }

    if(EnableShadowMips && cascadeIdx == NumCascades - 1)
        context->GenerateMips(varianceShadowMap.SRView);
}

// Renders all meshes in the model, with shadows
void MeshRenderer::Render(ID3D11DeviceContext* context, const Camera& camera, const Float4x4& world,
                          ID3D11ShaderResourceView* envMap, const SH9Color& envMapSH,
                          Float2 jitterOffset)
{
    PIXEvent event(L"Mesh Rendering");

    // Set states
    float blendFactor[4] = {1, 1, 1, 1};
    context->OMSetBlendState(blendStates.BlendDisabled(), blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(depthStencilStates.DepthEnabled(), 0);
    context->RSSetState(rasterizerStates.BackFaceCull());

    if(mipBiasSampler == nullptr || AppSettings::MipBias.Changed())
    {
        D3D11_SAMPLER_DESC desc = SamplerStates::AnisotropicDesc();
        desc.MipLODBias = AppSettings::MipBias;
        DXCall(device->CreateSamplerState(&desc, &mipBiasSampler));
    }

    ID3D11SamplerState* sampStates[3] = {
        AppSettings::EnableTemporalAA ?  mipBiasSampler : samplerStates.Anisotropic(),
        evsmSampler,
        samplerStates.LinearClamp(),
    };

    context->PSSetSamplers(0, 3, sampStates);

    // Set constant buffers
    meshVSConstants.Data.World = Float4x4::Transpose(world);
    meshVSConstants.Data.View = Float4x4::Transpose(camera.ViewMatrix());
    meshVSConstants.Data.WorldViewProjection = Float4x4::Transpose(world * camera.ViewProjectionMatrix());
    meshVSConstants.Data.PrevWorldViewProjection = prevWVP;
    meshVSConstants.ApplyChanges(context);
    meshVSConstants.SetVS(context, 0);

    prevWVP = meshVSConstants.Data.WorldViewProjection;

    meshPSConstants.Data.CameraPosWS = camera.Position();
    meshPSConstants.Data.OffsetScale = OffsetScale;
    meshPSConstants.Data.PositiveExponent = PositiveExponent;
    meshPSConstants.Data.NegativeExponent = NegativeExponent;
    meshPSConstants.Data.LightBleedingReduction = LightBleedingReduction;
    meshPSConstants.Data.Projection = Float4x4::Transpose(camera.ProjectionMatrix());
    meshPSConstants.Data.EnvironmentSH = envMapSH;
    meshPSConstants.Data.RTSize.x = float(GlobalApp->DeviceManager().BackBufferWidth());
    meshPSConstants.Data.RTSize.y = float(GlobalApp->DeviceManager().BackBufferHeight());
    meshPSConstants.Data.JitterOffset = jitterOffset;
    meshPSConstants.ApplyChanges(context);
    meshPSConstants.SetPS(context, 0);

    const uint64 nmlMapIdx = AppSettings::UseNormalMapping() ? 1 : 0;
    const uint64 centroidIdx = AppSettings::CentroidSampling ? 1 : 0;

    // Set shaders
    context->DSSetShader(nullptr, nullptr, 0);
    context->HSSetShader(nullptr, nullptr, 0);
    context->GSSetShader(nullptr, nullptr, 0);
    context->VSSetShader(meshVS[nmlMapIdx], nullptr, 0);
    context->PSSetShader(meshPS[nmlMapIdx][centroidIdx], nullptr, 0);

    // Draw all meshes
    uint32 partCount = 0;
    for(uint64 meshIdx = 0; meshIdx < model->Meshes().size(); ++meshIdx)
    {
        const Mesh& mesh = model->Meshes()[meshIdx];

        // Set the vertices and indices
        ID3D11Buffer* vertexBuffers[1] = { mesh.VertexBuffer() };
        UINT vertexStrides[1] = { mesh.VertexStride() };
        UINT offsets[1] = { 0 };
        context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, offsets);
        context->IASetIndexBuffer(mesh.IndexBuffer(), mesh.IndexBufferFormat(), 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Set the input layout
        context->IASetInputLayout(meshInputLayouts[meshIdx]);

        // Draw all parts
        for(uint64 partIdx = 0; partIdx < mesh.MeshParts().size(); ++partIdx)
        {
            const MeshPart& part = mesh.MeshParts()[partIdx];
            const MeshMaterial& material = model->Materials()[part.MaterialIdx];

            // Set the textures
            ID3D11ShaderResourceView* psTextures[5] =
            {
                material.DiffuseMap,
                material.NormalMap,
                varianceShadowMap.SRView,
                envMap,
                specularLookupTexture
            };

            context->PSSetShaderResources(0, 5, psTextures);
            context->DrawIndexed(part.IndexCount, part.IndexStart, 0);
        }
    }

    ID3D11ShaderResourceView* nullSRVs[5] = { nullptr };
    context->PSSetShaderResources(0, 5, nullSRVs);
}

// Renders all meshes using depth-only rendering
void MeshRenderer::RenderDepth(ID3D11DeviceContext* context, const Camera& camera,
    const Float4x4& world, bool shadowRendering)
{
    PIXEvent event(L"Mesh Depth Rendering");

    // Set states
    float blendFactor[4] = {1, 1, 1, 1};
    context->OMSetBlendState(blendStates.ColorWriteDisabled(), blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(depthStencilStates.DepthWriteEnabled(), 0);

    if(shadowRendering)
        context->RSSetState(shadowRSState);
    else
        context->RSSetState(rasterizerStates.BackFaceCull());

    // Set constant buffers
    meshVSConstants.Data.World = Float4x4::Transpose(world);
    meshVSConstants.Data.View = Float4x4::Transpose(camera.ViewMatrix());
    meshVSConstants.Data.WorldViewProjection = Float4x4::Transpose(world * camera.ViewProjectionMatrix());
    meshVSConstants.ApplyChanges(context);
    meshVSConstants.SetVS(context, 0);

    // Set shaders
    context->VSSetShader(meshDepthVS, nullptr, 0);
    context->PSSetShader(nullptr, nullptr, 0);
    context->GSSetShader(nullptr, nullptr, 0);
    context->DSSetShader(nullptr, nullptr, 0);
    context->HSSetShader(nullptr, nullptr, 0);


    uint32 partCount = 0;
    for(uint32 meshIdx = 0; meshIdx < model->Meshes().size(); ++meshIdx)
    {
        const Mesh& mesh = model->Meshes()[meshIdx];

        // Set the vertices and indices
        ID3D11Buffer* vertexBuffers[1] = { mesh.VertexBuffer() };
        UINT vertexStrides[1] = { mesh.VertexStride() };
        UINT offsets[1] = { 0 };
        context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, offsets);
        context->IASetIndexBuffer(mesh.IndexBuffer(), mesh.IndexBufferFormat(), 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Set the input layout
        context->IASetInputLayout(meshDepthInputLayouts[meshIdx]);

        // Draw all parts
        for(uint64 partIdx = 0; partIdx < mesh.MeshParts().size(); ++partIdx)
        {
            const MeshPart& part = mesh.MeshParts()[partIdx];
            context->DrawIndexed(part.IndexCount, part.IndexStart, 0);
        }
    }
}

// Renders meshes using cascaded shadow mapping
void MeshRenderer::RenderShadowMap(ID3D11DeviceContext* context, const Camera& camera,
                                   const Float4x4& world)
{
    PIXEvent event(L"Mesh Shadow Map Rendering");

    // Get the current render targets + viewport
    ID3D11RenderTargetView* renderTargets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = { NULL };
    ID3D11DepthStencilView* depthStencil = NULL;
    context->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, renderTargets, &depthStencil);

    uint32 numViewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT oldViewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    context->RSGetViewports(&numViewports, oldViewports);

    const float sMapSize = static_cast<float>(ShadowMapSize);

    const float MinDistance = shadowDepthBounds.x;
    const float MaxDistance = shadowDepthBounds.y;

    // Compute the split distances based on the partitioning mode
    float CascadeSplits[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    {
        float lambda = 1.0f;

        float nearClip = camera.NearClip();
        float farClip = camera.FarClip();
        float clipRange = farClip - nearClip;

        float minZ = nearClip + MinDistance * clipRange;
        float maxZ = nearClip + MaxDistance * clipRange;

        float range = maxZ - minZ;
        float ratio = maxZ / minZ;

        for(uint32 i = 0; i < NumCascades; ++i)
        {
            float p = (i + 1) / static_cast<float>(NumCascades);
            float log = minZ * std::pow(ratio, p);
            float uniform = minZ + range * p;
            float d = lambda * (log - uniform) + uniform;
            CascadeSplits[i] = (d - nearClip) / clipRange;
        }
    }

    Float3 c0Extents;
    Float4x4 c0Matrix;

    // Render the meshes to each cascade
    for(uint32 cascadeIdx = 0; cascadeIdx < NumCascades; ++cascadeIdx)
    {
        PIXEvent cascadeEvent((L"Rendering Shadow Map Cascade " + ToString(cascadeIdx)).c_str());

        // Set the viewport
        D3D11_VIEWPORT viewport;
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = sMapSize;
        viewport.Height = sMapSize;
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        context->RSSetViewports(1, &viewport);

        // Set the shadow map as the depth target
        ID3D11DepthStencilView* dsv = shadowMap.DSView;
        ID3D11RenderTargetView* nullRenderTargets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = { NULL };
        context->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, nullRenderTargets, dsv);
        context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

        // Get the 8 points of the view frustum in world space
        XMVECTOR frustumCornersWS[8] =
        {
            XMVectorSet(-1.0f,  1.0f, 0.0f, 1.0f),
            XMVectorSet( 1.0f,  1.0f, 0.0f, 1.0f),
            XMVectorSet( 1.0f, -1.0f, 0.0f, 1.0f),
            XMVectorSet(-1.0f, -1.0f, 0.0f, 1.0f),
            XMVectorSet(-1.0f,  1.0f, 1.0f, 1.0f),
            XMVectorSet( 1.0f,  1.0f, 1.0f, 1.0f),
            XMVectorSet( 1.0f, -1.0f, 1.0f, 1.0f),
            XMVectorSet(-1.0f, -1.0f, 1.0f, 1.0f),
        };

        float prevSplitDist = cascadeIdx == 0 ? MinDistance : CascadeSplits[cascadeIdx - 1];
        float splitDist = CascadeSplits[cascadeIdx];

        XMVECTOR det;
        XMMATRIX invViewProj = XMMatrixInverse(&det, camera.ViewProjectionMatrix().ToSIMD());
        for(uint32 i = 0; i < 8; ++i)
            frustumCornersWS[i] = XMVector3TransformCoord(frustumCornersWS[i], invViewProj);

        // Get the corners of the current cascade slice of the view frustum
        for(uint32 i = 0; i < 4; ++i)
        {
            XMVECTOR cornerRay = XMVectorSubtract(frustumCornersWS[i + 4], frustumCornersWS[i]);
            XMVECTOR nearCornerRay = XMVectorScale(cornerRay, prevSplitDist);
            XMVECTOR farCornerRay = XMVectorScale(cornerRay, splitDist);
            frustumCornersWS[i + 4] = XMVectorAdd(frustumCornersWS[i], farCornerRay);
            frustumCornersWS[i] = XMVectorAdd(frustumCornersWS[i], nearCornerRay);
        }

        // Calculate the centroid of the view frustum slice
        XMVECTOR frustumCenterVec = XMVectorZero();
        for(uint32 i = 0; i < 8; ++i)
            frustumCenterVec = XMVectorAdd(frustumCenterVec, frustumCornersWS[i]);
        frustumCenterVec = XMVectorScale(frustumCenterVec, 1.0f / 8.0f);
        Float3 frustumCenter = frustumCenterVec;

        // Pick the up vector to use for the light camera
        Float3 upDir = camera.Right();

        Float3 minExtents;
        Float3 maxExtents;

        {
            // Create a temporary view matrix for the light
            Float3 lightCameraPos = frustumCenter;
            Float3 lookAt = frustumCenter - AppSettings::LightDirection;
            XMMATRIX lightView = XMMatrixLookAtLH(lightCameraPos.ToSIMD(), lookAt.ToSIMD(), upDir.ToSIMD());

            // Calculate an AABB around the frustum corners
            XMVECTOR mins = XMVectorSet(REAL_MAX, REAL_MAX, REAL_MAX, REAL_MAX);
            XMVECTOR maxes = XMVectorSet(-REAL_MAX, -REAL_MAX, -REAL_MAX, -REAL_MAX);
            for(uint32 i = 0; i < 8; ++i)
            {
                XMVECTOR corner = XMVector3TransformCoord(frustumCornersWS[i], lightView);
                mins = XMVectorMin(mins, corner);
                maxes = XMVectorMax(maxes, corner);
            }

            minExtents = mins;
            maxExtents = maxes;
        }

        // Adjust the min/max to accommodate the filtering size
        float scale = (ShadowMapSize + FilterSize) / static_cast<float>(ShadowMapSize);
        minExtents.x *= scale;
        minExtents.y *= scale;
        maxExtents.x *= scale;
        maxExtents.x *= scale;

        Float3 cascadeExtents = maxExtents - minExtents;

        // Get position of the shadow camera
        Float3 shadowCameraPos = frustumCenter + AppSettings::LightDirection.Value() * -minExtents.z;

        // Come up with a new orthographic camera for the shadow caster
        OrthographicCamera shadowCamera(minExtents.x, minExtents.y, maxExtents.x,
            maxExtents.y, 0.0f, cascadeExtents.z);
        shadowCamera.SetLookAt(shadowCameraPos, frustumCenter, upDir);

        // Draw the mesh with depth only, using the new shadow camera
        RenderDepth(context, shadowCamera, world, true);

        // Apply the scale/offset matrix, which transforms from [-1,1]
        // post-projection space to [0,1] UV space
        XMMATRIX texScaleBias;
        texScaleBias.r[0] = XMVectorSet(0.5f,  0.0f, 0.0f, 0.0f);
        texScaleBias.r[1] = XMVectorSet(0.0f, -0.5f, 0.0f, 0.0f);
        texScaleBias.r[2] = XMVectorSet(0.0f,  0.0f, 1.0f, 0.0f);
        texScaleBias.r[3] = XMVectorSet(0.5f,  0.5f, 0.0f, 1.0f);
        XMMATRIX shadowMatrix = shadowCamera.ViewProjectionMatrix().ToSIMD();
        shadowMatrix = XMMatrixMultiply(shadowMatrix, texScaleBias);

        // Store the split distance in terms of view space depth
        const float clipDist = camera.FarClip() - camera.NearClip();
        meshPSConstants.Data.CascadeSplits[cascadeIdx] = camera.NearClip() + splitDist * clipDist;

        if(cascadeIdx == 0)
        {
            c0Extents = cascadeExtents;
            c0Matrix = shadowMatrix;
            meshPSConstants.Data.ShadowMatrix = XMMatrixTranspose(shadowMatrix);
            meshPSConstants.Data.CascadeOffsets[0] = Float4(0.0f, 0.0f, 0.0f, 0.0f);
            meshPSConstants.Data.CascadeScales[0] = Float4(1.0f, 1.0f, 1.0f, 1.0f);
        }
        else
        {
            // Calculate the position of the lower corner of the cascade partition, in the UV space
            // of the first cascade partition
            Float4x4 invCascadeMat = Float4x4::Invert(shadowMatrix);
            Float3 cascadeCorner = Float3::Transform(Float3(0.0f, 0.0f, 0.0f), invCascadeMat);
            cascadeCorner = Float3::Transform(cascadeCorner, c0Matrix);

            // Do the same for the upper corner
            Float3 otherCorner = Float3::Transform(Float3(1.0f, 1.0f, 1.0f), invCascadeMat);
            otherCorner = Float3::Transform(otherCorner, c0Matrix);

            // Calculate the scale and offset
            Float3 cascadeScale = Float3(1.0f, 1.0f, 1.f) / (otherCorner - cascadeCorner);
            meshPSConstants.Data.CascadeOffsets[cascadeIdx] = Float4(-cascadeCorner, 0.0f);
            meshPSConstants.Data.CascadeScales[cascadeIdx] = Float4(cascadeScale, 1.0f);
        }

        ConvertToEVSM(context, cascadeIdx, meshPSConstants.Data.CascadeScales[cascadeIdx].To3D());
    }

    // Restore the previous render targets and viewports
    context->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, renderTargets, depthStencil);
    context->RSSetViewports(D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, oldViewports);

    for(uint32 i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        if(renderTargets[i] != NULL)
            renderTargets[i]->Release();
    if(depthStencil != NULL)
        depthStencil->Release();
}