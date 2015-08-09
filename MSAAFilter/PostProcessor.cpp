//=================================================================================================
//
//  MSAA Filtering 2.0 Sample
//  by MJP
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#include <PCH.h>

#include <Graphics\\ShaderCompilation.h>
#include <Graphics\\Profiler.h>

#include "PostProcessor.h"
#include "SharedConstants.h"

// Constants
static const uint32 TGSize = 16;
static const uint32 LumMapSize = 1024;

void PostProcessor::Initialize(ID3D11Device* device)
{
    PostProcessorBase::Initialize(device);

    constantBuffer.Initialize(device);

    // Load the shaders
    toneMap = CompilePSFromFile(device, L"PostProcessing.hlsl", "ToneMap");
    scale = CompilePSFromFile(device, L"PostProcessing.hlsl", "Scale");
    blurH = CompilePSFromFile(device, L"PostProcessing.hlsl", "BlurH");
    blurV = CompilePSFromFile(device, L"PostProcessing.hlsl", "BlurV");
    bloom = CompilePSFromFile(device, L"PostProcessing.hlsl", "Bloom");
    sharpen = CompilePSFromFile(device, L"PostProcessing.hlsl", "Sharpen");

    reduceLuminanceInitial = CompileCSFromFile(device, L"LuminanceReduction.hlsl",
                                               "LuminanceReductionInitialCS", "cs_5_0");
    reduceLuminance = CompileCSFromFile(device, L"LuminanceReduction.hlsl", "LuminanceReductionCS");

    CompileOptions opts;
    opts.Add("FinalPass_", 1);
    reduceLuminanceFinal = CompileCSFromFile(device, L"LuminanceReduction.hlsl", "LuminanceReductionCS",
                                             "cs_5_0", opts);
}

void PostProcessor::AfterReset(uint32 width, uint32 height)
{
    PostProcessorBase::AfterReset(width, height);

    reductionTargets.clear();

    uint32 w = width;
    uint32 h = height;

    while(w > 1 || h > 1)
    {
        w = DispatchSize(ReductionTGSize, w);
        h = DispatchSize(ReductionTGSize, h);

        RenderTarget2D rt;
        rt.Initialize(device, w, h, DXGI_FORMAT_R32_FLOAT, 1, 1, 0, false, true);
        reductionTargets.push_back(rt);
    }

    adaptedLuminance = reductionTargets[reductionTargets.size() - 1].SRView;

    constantBuffer.Data.EnableAdaptation = false;
}

void PostProcessor::Render(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView* input,
                           ID3D11RenderTargetView* output, float deltaSeconds)
{
    PostProcessorBase::Render(deviceContext, input, output);

    constantBuffer.Data.TimeDelta = deltaSeconds;

    constantBuffer.ApplyChanges(deviceContext);
    constantBuffer.SetPS(deviceContext, 1);

    if(AppSettings::EnableAutoExposure)
        CalcAvgLuminance(input);

    TempRenderTarget* bloom = Bloom(input);

    const bool applySharpening = AppSettings::SharpeningAmount > 0.0f && AppSettings::UseStandardResolve == false;
    TempRenderTarget* toneMapTarget = nullptr;
    ID3D11RenderTargetView* toneMapOutput = output;
    if(applySharpening)
    {
        toneMapTarget = GetTempRenderTarget(inputWidth, inputHeight, DXGI_FORMAT_R10G10B10A2_UNORM);
        toneMapOutput = toneMapTarget->RTView;
    }

    // Apply tone mapping
    ToneMap(input, bloom->SRView, toneMapOutput);

    if(applySharpening)
    {
        PostProcess(toneMapTarget->SRView, output, sharpen, L"Sharpening");
        toneMapTarget->InUse = false;
    }

    bloom->InUse = false;
    constantBuffer.Data.EnableAdaptation = true;
}

void PostProcessor::CalcAvgLuminance(ID3D11ShaderResourceView* input)
{
    // Calculate the geometric mean of luminance through reduction
    PIXEvent pixEvent(L"Average Luminance Calculation");

    constantBuffer.SetCS(context, 0);

    ID3D11UnorderedAccessView* uavs[1] = { reductionTargets[0].UAView };
    context->CSSetUnorderedAccessViews(0, 1, uavs, NULL);

    ID3D11ShaderResourceView* srvs[1] = { input };
    context->CSSetShaderResources(0, 1, srvs);

    context->CSSetShader(reduceLuminanceInitial, NULL, 0);

    uint32 dispatchX = reductionTargets[0].Width;
    uint32 dispatchY = reductionTargets[0].Height;
    context->Dispatch(dispatchX, dispatchY, 1);

    uavs[0] = NULL;
    context->CSSetUnorderedAccessViews(0, 1, uavs, NULL);

    srvs[0] = NULL;
    context->CSSetShaderResources(0, 1, srvs);

    for(uint32 i = 1; i < reductionTargets.size(); ++i)
    {
        if(i == reductionTargets.size() - 1)
            context->CSSetShader(reduceLuminanceFinal, NULL, 0);
        else
            context->CSSetShader(reduceLuminance, NULL, 0);

        uavs[0] = reductionTargets[i].UAView;
        context->CSSetUnorderedAccessViews(0, 1, uavs, NULL);

        srvs[0] = reductionTargets[i - 1].SRView;
        context->CSSetShaderResources(0, 1, srvs);

        dispatchX = reductionTargets[i].Width;
        dispatchY = reductionTargets[i].Height;
        context->Dispatch(dispatchX, dispatchY, 1);

        uavs[0] = NULL;
        context->CSSetUnorderedAccessViews(0, 1, uavs, NULL);

        srvs[0] = NULL;
        context->CSSetShaderResources(0, 1, srvs);
    }
}

TempRenderTarget* PostProcessor::Bloom(ID3D11ShaderResourceView* input)
{
    PIXEvent pixEvent(L"Bloom");

    TempRenderTarget* bloomTarget = GetTempRenderTarget(inputWidth / 2, inputHeight / 2, DXGI_FORMAT_R11G11B10_FLOAT);
    inputs.push_back(input);
    inputs.push_back(adaptedLuminance);
    outputs.push_back(bloomTarget->RTView);
    PostProcess(bloom, L"Bloom Initial Pass");

    // Blur it
    for(uint64 i = 0; i < 2; ++i)
    {
        TempRenderTarget* blurTemp = GetTempRenderTarget(bloomTarget->Width, bloomTarget->Height, bloomTarget->Format);
        PostProcess(bloomTarget->SRView, blurTemp->RTView, blurH, L"Horizontal Bloom Blur");

        PostProcess(blurTemp->SRView, bloomTarget->RTView, blurV, L"Vertical Bloom Blur");
        blurTemp->InUse = false;
    }

    return bloomTarget;
}

void PostProcessor::ToneMap(ID3D11ShaderResourceView* input,
                            ID3D11ShaderResourceView* bloom,
                            ID3D11RenderTargetView* output)
{
    inputs.push_back(input);
    inputs.push_back(adaptedLuminance);
    inputs.push_back(bloom);
    outputs.push_back(output);

    PostProcess(toneMap, L"Tone Mapping");
}