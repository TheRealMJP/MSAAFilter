//=================================================================================================
//
//  MSAA Filtering 2.0 Sample
//  by MJP
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

//=================================================================================================
// Includes
//=================================================================================================
#include "SharedConstants.h"
#include "AppSettings.hlsl"
#include <Constants.hlsl>

//=================================================================================================
// Constants
//=================================================================================================
#ifndef MSAASamples_
    #define MSAASamples_ 1
#endif

#ifndef FilterType_
    #define FilterType_ 0
#endif

#define MSAA_ (MSAASamples_ > 1)

#if MSAASamples_ == 8
    static const float2 SampleOffsets[8] = {
        float2(0.580f, 0.298f),
        float2(0.419f, 0.698f),
        float2(0.819f, 0.580f),
        float2(0.298f, 0.180f),
        float2(0.180f, 0.819f),
        float2(0.058f, 0.419f),
        float2(0.698f, 0.941f),
        float2(0.941f, 0.058f),
    };
#elif MSAASamples_ == 4
    static const float2 SampleOffsets[4] = {
        float2(0.380f, 0.141f),
        float2(0.859f, 0.380f),
        float2(0.141f, 0.620f),
        float2(0.619f, 0.859f),
    };
#elif MSAASamples_ == 2
    static const float2 SampleOffsets[2] = {
        float2(0.741f, 0.741f),
        float2(0.258f, 0.258f),
    };
#else
    static const float2 SampleOffsets[1] = {
        float2(0.5f, 0.5f),
    };
#endif

//=================================================================================================
// Resources
//=================================================================================================
#if MSAA_
    Texture2DMS<float4> InputTexture : register(t0);
#else
    Texture2D<float4> InputTexture : register(t0);
#endif

Texture2D<float4> PrevFrameTexture : register(t1);
Texture2D<float2> VelocityTexture : register(t2);

SamplerState LinearSampler : register(s0);

cbuffer ResolveConstants : register(b0)
{
    int SampleRadius;
    float2 TextureSize;
}

float3 SampleTexture(in float2 samplePos, in uint subSampleIdx)
{
    #if MSAA_
        return InputTexture.Load(uint2(samplePos), subSampleIdx).xyz;
    #else
        return InputTexture[uint2(samplePos)].xyz;
    #endif
}

float FilterBox(in float x)
{
    return 1.0f;
}

static float FilterTriangle(in float x)
{
    return saturate(1.0f - x);
}

static float FilterGaussian(in float x)
{
    const float sigma = GaussianSigma;
    const float g = 1.0f / sqrt(2.0f * 3.14159f * sigma * sigma);
    return (g * exp(-(x * x) / (2 * sigma * sigma)));
}

 float FilterCubic(in float x, in float B, in float C)
{
    // Rescale from [-2, 2] range to [-FilterWidth, FilterWidth]
    x *= 2.0f;

    float y = 0.0f;
    float x2 = x * x;
    float x3 = x * x * x;
    if(x < 1)
        y = (12 - 9 * B - 6 * C) * x3 + (-18 + 12 * B + 6 * C) * x2 + (6 - 2 * B);
    else if (x <= 2)
        y = (-B - 6 * C) * x3 + (6 * B + 30 * C) * x2 + (-12 * B - 48 * C) * x + (8 * B + 24 * C);

    return y / 6.0f;
}

float FilterSinc(in float x)
{
    float s;

    x *= FilterSize;

    if(x < 0.001f)
        s = 1.0f;
    else
        s = sin(x * Pi) / (x * Pi);

    return s;
}

float FilterBlackmanHarris(in float x)
{
    x = 1.0f - x;

    const float a0 = 0.35875f;
    const float a1 = 0.48829f;
    const float a2 = 0.14128f;
    const float a3 = 0.01168f;
    return saturate(a0 - a1 * cos(Pi * x) + a2 * cos(2 * Pi * x) - a3 * cos(3 * Pi * x));
}

float FilterSmoothstep(in float x)
{
    return 1.0f - smoothstep(0.0f, 1.0f, x);
}

float Filter(in float x)
{
    if(FilterType == FilterTypes_Box)
        return FilterBox(x);
    else if(FilterType == FilterTypes_Triangle)
        return FilterTriangle(x);
    else if(FilterType == FilterTypes_Gaussian)
        return FilterGaussian(x);
    else if(FilterType == FilterTypes_BlackmanHarris)
        return FilterBlackmanHarris(x);
    else if(FilterType == FilterTypes_Smoothstep)
        return FilterSmoothstep(x);
    else if(FilterType == FilterTypes_BSpline)
        return FilterCubic(x, 1.0, 0.0f);
    else if(FilterType == FilterTypes_CatmullRom)
        return FilterCubic(x, 0, 0.5f);
    else if(FilterType == FilterTypes_Mitchell)
        return FilterCubic(x, 1 / 3.0f, 1 / 3.0f);
    else if(FilterType == FilterTypes_GeneralizedCubic)
        return FilterCubic(x, CubicB, CubicC);
    else if(FilterType == FilterTypes_Sinc)
        return FilterSinc(x);
    else
        return 1.0f;
}

float4 ResolveVS(in uint VertexID : SV_VertexID) : SV_Position
{
    float4 output = 0.0f;

    if(VertexID == 0)
        output = float4(-1.0f, 1.0f, 1.0f, 1.0f);
    else if(VertexID == 1)
        output = float4(3.0f, 1.0f, 1.0f, 1.0f);
    else
        output = float4(-1.0f, -3.0f, 1.0f, 1.0f);

    return output;
}

float Luminance(in float3 clr)
{
    return dot(clr, float3(0.299f, 0.587f, 0.114f));
}

float4 ResolvePS(in float4 Position : SV_Position) : SV_Target0
{
    float2 pixelPos = Position.xy;
    float3 sum = 0.0f;
    float totalWeight = 0.0f;

    float3 clrMin = 99999999.0f;
    float3 clrMax = -99999999.0f;

    for(int y = -SampleRadius; y <= SampleRadius; ++y)
    {
        for(int x = -SampleRadius; x <= SampleRadius; ++x)
        {
            float2 sampleOffset = float2(x, y);
            float2 samplePos = pixelPos + sampleOffset;
            samplePos = clamp(samplePos, 0, TextureSize - 1.0f);

            [unroll]
            for(uint subSampleIdx = 0; subSampleIdx < MSAASamples_; ++subSampleIdx)
            {
                sampleOffset += SampleOffsets[subSampleIdx].xy - 0.5f;

                float sampleDist = length(sampleOffset) / (FilterSize / 2.0f);

                [branch]
                if(sampleDist <= 1.0f)
                {
                    float3 sample = SampleTexture(samplePos, subSampleIdx);
                    sample = max(sample, 0.0f);

                    float weight = Filter(sampleDist);
                    clrMin = min(clrMin, sample);
                    clrMax = max(clrMax, sample);

                    float sampleLum = Luminance(sample);

                    if(UseExposureFiltering)
                        sampleLum *= exp2(ManualExposure - ExposureScale + ExposureFilterOffset);

                    if(InverseLuminanceFiltering)
                        weight *= 1.0f / (1.0f + sampleLum);

                    sum += sample * weight;
                    totalWeight += weight;
                }
            }
        }
    }

    float3 output = sum / max(totalWeight, 0.00001f);
    output = max(output, 0.0f);

    if(EnableTemporalAA)
    {
        float3 currColor = output;

        float2 velocity = VelocityTexture[pixelPos];
        float2 prevPixelPos = pixelPos - velocity;
        float2 prevUV = prevPixelPos / TextureSize;

        float3 prevColor = PrevFrameTexture.SampleLevel(LinearSampler, prevUV, 0.0f).xyz;

        if(ClampPrevColor)
            prevColor = clamp(prevColor, clrMin, clrMax);

        float3 weightA = saturate(1.0f - TemporalAABlendFactor);
        float3 weightB = saturate(TemporalAABlendFactor);

        if(UseTemporalColorWeighting)
        {
            float3 temporalWeight = saturate(abs(clrMax - clrMin) / currColor);
            weightB = saturate(lerp(LowFreqWeight, HiFreqWeight, temporalWeight));
            weightA = 1.0f - weightB;
        }

        if(InverseLuminanceFiltering)
        {
            weightA *= 1.0f / (1.0f + Luminance(currColor));
            weightB *= 1.0f / (1.0f + Luminance(prevColor));
        }

        output = (currColor * weightA + prevColor * weightB) / (weightA + weightB);
    }

    return float4(output, 1.0f);
}