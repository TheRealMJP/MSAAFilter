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

#define MSAA_ (MSAASamples_ > 1)

// These are the sub-sample locations for the 2x, 4x, and 8x standard multisample patterns.
// See the MSDN documentation for the D3D11_STANDARD_MULTISAMPLE_QUALITY_LEVELS enumeration.
#if MSAASamples_ == 8
    static const float2 SubSampleOffsets[8] = {
        float2( 0.0625f, -0.1875f),
        float2(-0.0625f,  0.1875f),
        float2( 0.3125f,  0.0625f),
        float2(-0.1875f, -0.3125f),
        float2(-0.3125f,  0.3125f),
        float2(-0.4375f, -0.0625f),
        float2( 0.1875f,  0.4375f),
        float2( 0.4375f, -0.4375f),
    };
#elif MSAASamples_ == 4
    static const float2 SubSampleOffsets[4] = {
        float2(-0.125f, -0.375f),
        float2( 0.375f, -0.125f),
        float2(-0.375f,  0.125f),
        float2( 0.125f,  0.375f),
    };
#elif MSAASamples_ == 2
    static const float2 SubSampleOffsets[2] = {
        float2( 0.25f,  0.25f),
        float2(-0.25f, -0.25f),
    };
#else
    static const float2 SubSampleOffsets[1] = {
        float2(0.0f, 0.0f),
    };
#endif

//=================================================================================================
// Resources
//=================================================================================================
#if MSAA_
    Texture2DMS<float4> InputTexture : register(t0);
    Texture2DMS<float2> VelocityTexture : register(t1);
    Texture2DMS<float> DepthTexture : register(t2);
#else
    Texture2D<float4> InputTexture : register(t0);
    Texture2D<float2> VelocityTexture : register(t1);
    Texture2D<float> DepthTexture : register(t2);
#endif

Texture2D<float4> PrevFrameTexture : register(t3);

SamplerState LinearSampler : register(s0);
SamplerState PointSampler : register(s1);

cbuffer ResolveConstants : register(b0)
{
    int SampleRadius;
    float2 TextureSize;
}

#if MSAA_
    #define MSAALoad_(tex, addr, subSampleIdx) tex.Load(uint2(addr), subSampleIdx)
#else
    #define MSAALoad_(tex, addr, subSampleIdx) tex[uint2(addr)]
#endif

// All filtering functions assume that 'x' is normalized to [0, 1], where 1 == FilteRadius
float FilterBox(in float x)
{
    return x <= 1.0f;
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
    float y = 0.0f;
    float x2 = x * x;
    float x3 = x * x * x;
    if(x < 1)
        y = (12 - 9 * B - 6 * C) * x3 + (-18 + 12 * B + 6 * C) * x2 + (6 - 2 * B);
    else if (x <= 2)
        y = (-B - 6 * C) * x3 + (6 * B + 30 * C) * x2 + (-12 * B - 48 * C) * x + (8 * B + 24 * C);

    return y / 6.0f;
}

float FilterSinc(in float x, in float filterRadius)
{
    float s;

    x *= filterRadius * 2.0f;

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

float Filter(in float x, in int filterType, in float filterRadius, in bool rescaleCubic)
{
    // Cubic filters naturually work in a [-2, 2] domain. For the resolve case we
    // want to rescale the filter so that it works in [-1, 1] instead
    float cubicX = rescaleCubic ? x * 2.0f : x;

    if(filterType == FilterTypes_Box)
        return FilterBox(x);
    else if(filterType == FilterTypes_Triangle)
        return FilterTriangle(x);
    else if(filterType == FilterTypes_Gaussian)
        return FilterGaussian(x);
    else if(filterType == FilterTypes_BlackmanHarris)
        return FilterBlackmanHarris(x);
    else if(filterType == FilterTypes_Smoothstep)
        return FilterSmoothstep(x);
    else if(filterType == FilterTypes_BSpline)
        return FilterCubic(cubicX, 1.0, 0.0f);
    else if(filterType == FilterTypes_CatmullRom)
        return FilterCubic(cubicX, 0, 0.5f);
    else if(filterType == FilterTypes_Mitchell)
        return FilterCubic(cubicX, 1 / 3.0f, 1 / 3.0f);
    else if(filterType == FilterTypes_GeneralizedCubic)
        return FilterCubic(cubicX, CubicB, CubicC);
    else if(filterType == FilterTypes_Sinc)
        return FilterSinc(x, filterRadius);
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

// From "Temporal Reprojection Anti-Aliasing"
// https://github.com/playdeadgames/temporal
float3 ClipAABB(float3 aabbMin, float3 aabbMax, float3 prevSample, float3 avg)
{
    #if 1
        // note: only clips towards aabb center (but fast!)
        float3 p_clip = 0.5 * (aabbMax + aabbMin);
        float3 e_clip = 0.5 * (aabbMax - aabbMin);

        float3 v_clip = prevSample - p_clip;
        float3 v_unit = v_clip.xyz / e_clip;
        float3 a_unit = abs(v_unit);
        float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));

        if (ma_unit > 1.0)
            return p_clip + v_clip / ma_unit;
        else
            return prevSample;// point inside aabb
    #else
        float3 r = prevSample - avg;
        float3 rmax = aabbMax - avg.xyz;
        float3 rmin = aabbMin - avg.xyz;

        const float eps = 0.000001f;

        if (r.x > rmax.x + eps)
            r *= (rmax.x / r.x);
        if (r.y > rmax.y + eps)
            r *= (rmax.y / r.y);
        if (r.z > rmax.z + eps)
            r *= (rmax.z / r.z);

        if (r.x < rmin.x - eps)
            r *= (rmin.x / r.x);
        if (r.y < rmin.y - eps)
            r *= (rmin.y / r.y);
        if (r.z < rmin.z - eps)
            r *= (rmin.z / r.z);

        return avg + r;
    #endif
}

float3 Reproject(in float2 pixelPos)
{
    float2 velocity = 0.0f;
    if(DilationMode == DilationModes_CenterAverage)
    {
        [unroll]
        for(uint vsIdx = 0; vsIdx < MSAASamples_; ++vsIdx)
            velocity += MSAALoad_(VelocityTexture, pixelPos, vsIdx);
        velocity /= MSAASamples_;
    }
    else if(DilationMode == DilationModes_DilateNearestDepth)
    {
        float closestDepth = 10.0f;
        for(int vy = -1; vy <= 1; ++vy)
        {
            for(int vx = -1; vx <= 1; ++vx)
            {
                [unroll]
                for(uint vsIdx = 0; vsIdx < MSAASamples_; ++vsIdx)
                {
                    float2 neighborVelocity = MSAALoad_(VelocityTexture, pixelPos + int2(vx, vy), vsIdx);
                    float neighborDepth = MSAALoad_(DepthTexture, pixelPos + int2(vx, vy), vsIdx);
                    if(neighborDepth < closestDepth)
                    {
                        velocity = neighborVelocity;
                        closestDepth = neighborDepth;
                    }
                }
            }
        }
    }
    else if(DilationMode == DilationModes_DilateGreatestVelocity)
    {
        float greatestVelocity = -1.0f;
        for(int vy = -1; vy <= 1; ++vy)
        {
            for(int vx = -1; vx <= 1; ++vx)
            {
                [unroll]
                for(uint vsIdx = 0; vsIdx < MSAASamples_; ++vsIdx)
                {
                    float2 neighborVelocity = MSAALoad_(VelocityTexture, pixelPos + int2(vx, vy), vsIdx);
                    float neighborVelocityMag = dot(neighborVelocity, neighborVelocity);
                    if(dot(neighborVelocity, neighborVelocity) > greatestVelocity)
                    {
                        velocity = neighborVelocity;
                        greatestVelocity = neighborVelocityMag;
                    }
                }
            }
        }
    }

    velocity *= TextureSize;
    float2 reprojectedPos = pixelPos - velocity;
    float2 reprojectedUV = reprojectedPos / TextureSize;

    if(UseStandardReprojection)
    {
        return PrevFrameTexture.SampleLevel(LinearSampler, reprojectedUV, 0.0f).xyz;
    }
    else
    {
        float3 sum = 0.0f;
        float totalWeight = 0.0f;

        for(int ty = -1; ty <= 2; ++ty)
        {
            for(int tx = -1; tx <= 2; ++tx)
            {
                float2 samplePos = floor(reprojectedPos + float2(tx, ty)) + 0.5f;
                float3 reprojectedSample = PrevFrameTexture[int2(samplePos)].xyz;

                float2 sampleDist = abs(samplePos - reprojectedPos);
                float filterWeight = Filter(sampleDist.x, ReprojectionFilter, 1.0f, false) *
                                     Filter(sampleDist.y, ReprojectionFilter, 1.0f, false);

                float sampleLum = Luminance(reprojectedSample);

                if(UseExposureFiltering)
                    sampleLum *= exp2(ManualExposure - ExposureScale + ExposureFilterOffset);

                if(InverseLuminanceFiltering)
                    filterWeight *= 1.0f / (1.0f + sampleLum);

                sum += reprojectedSample * filterWeight;
                totalWeight += filterWeight;
            }
        }

        return max(sum / totalWeight, 0.0f);
    }
}

float4 ResolvePS(in float4 Position : SV_Position) : SV_Target0
{
    float2 pixelPos = Position.xy;
    float3 sum = 0.0f;
    float totalWeight = 0.0f;

    float3 clrMin = 99999999.0f;
    float3 clrMax = -99999999.0f;

    float3 m1 = 0.0f;
    float3 m2 = 0.0f;
    float mWeight = 0.0f;

    #if MSAA_
        const int SampleRadius_ = SampleRadius;
    #else
        const int SampleRadius_ = 1;
    #endif

    const float filterRadius = ResolveFilterDiameter / 2.0f;

    for(int y = -SampleRadius_; y <= SampleRadius_; ++y)
    {
        for(int x = -SampleRadius_; x <= SampleRadius_; ++x)
        {
            float2 sampleOffset = float2(x, y);
            float2 samplePos = pixelPos + sampleOffset;
            samplePos = clamp(samplePos, 0, TextureSize - 1.0f);

            [unroll]
            for(uint subSampleIdx = 0; subSampleIdx < MSAASamples_; ++subSampleIdx)
            {
                float2 subSampleOffset = SubSampleOffsets[subSampleIdx].xy;
                float2 sampleDist = abs(sampleOffset + subSampleOffset) / (ResolveFilterDiameter / 2.0f);

                #if MSAA_
                    bool useSample = all(sampleDist <= 1.0f);
                #else
                    bool useSample = true;
                #endif

                if(useSample)
                {
                    float3 sample = MSAALoad_(InputTexture, samplePos, subSampleIdx).xyz;
                    sample = max(sample, 0.0f);

                    float weight = Filter(sampleDist.x, ResolveFilterType, filterRadius, true) *
                                   Filter(sampleDist.y, ResolveFilterType, filterRadius, true);
                    clrMin = min(clrMin, sample);
                    clrMax = max(clrMax, sample);

                    float sampleLum = Luminance(sample);

                    if(UseExposureFiltering)
                        sampleLum *= exp2(ManualExposure - ExposureScale + ExposureFilterOffset);

                    if(InverseLuminanceFiltering)
                        weight *= 1.0f / (1.0f + sampleLum);

                    sum += sample * weight;
                    totalWeight += weight;

                    m1 += sample;
                    m2 += sample * sample;
                    mWeight += 1.0f;
                }
            }
        }
    }

    #if MSAA_
        float3 output = sum / max(totalWeight, 0.00001f);
    #else
        float3 output = InputTexture[uint2(pixelPos)].xyz;
    #endif

    output = max(output, 0.0f);

    if(EnableTemporalAA)
    {
        float3 currColor = output;
        float3 prevColor = Reproject(pixelPos);

        if(NeighborhoodClampMode == ClampModes_RGB_Clamp)
        {
            prevColor = clamp(prevColor, clrMin, clrMax);
        }
        else if(NeighborhoodClampMode == ClampModes_RGB_Clip)
        {
            prevColor = ClipAABB(clrMin, clrMax, prevColor, m1 / mWeight);
        }
        else if(NeighborhoodClampMode == ClampModes_Variance_Clip)
        {
            float3 mu = m1 / mWeight;
            float3 sigma = sqrt(abs(m2 / mWeight - mu * mu));
            float3 minc = mu - VarianceClipGamma * sigma;
            float3 maxc = mu + VarianceClipGamma * sigma;
            prevColor = ClipAABB(minc, maxc, prevColor, mu);
        }

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