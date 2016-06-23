//=================================================================================================
//
//  MSAA Filtering 2.0 Sample
//  by MJP
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#include <PPIncludes.hlsl>
#include "SharedConstants.h"
#include "AppSettings.hlsl"

//=================================================================================================
// Constants
//=================================================================================================
cbuffer PPConstants : register(b1)
{
    float TimeDelta;
    bool EnableAdaptation;
};

//=================================================================================================
// Helper Functions
//=================================================================================================

// Approximates luminance from an RGB value
float CalcLuminance(float3 color)
{
    return dot(color, float3(0.299f, 0.587f, 0.114f));
}

// Retrieves the log-average luminance from the texture
float GetAvgLuminance(Texture2D lumTex)
{
    return lumTex.Load(uint3(0, 0, 0)).x;
}

// Applies the filmic curve from John Hable's presentation
float3 ToneMapFilmicALU(in float3 color)
{
    color = max(0, color - 0.004f);
    color = (color * (6.2f * color + 0.5f)) / (color * (6.2f * color + 1.7f)+ 0.06f);

    // Result has 1/2.2 baked in
    return pow(color, 2.2f);
}

// Determines the color based on exposure settings
float3 CalcExposedColor(float3 color, float avgLuminance, float offset, out float exposure)
{
    // Use geometric mean
    avgLuminance = max(avgLuminance, 0.001f);
    float linearExposure = (KeyValue / avgLuminance);
    if(EnableAutoExposure)
        exposure = log2(max(linearExposure, 0.0001f));
    else
        exposure = ManualExposure;
    exposure += offset;
    exposure -= ExposureScale;
    return exp2(exposure) * color;
}

// Applies exposure and tone mapping to the specific color, and applies
// the threshold to the exposure value.
float3 ToneMap(float3 color, float avgLuminance, float threshold, out float exposure)
{
    color = CalcExposedColor(color, avgLuminance, threshold, exposure);
    color = ToneMapFilmicALU(color);
    return color;
}

// Calculates the gaussian blur weight for a given distance and sigmas
float CalcGaussianWeight(int sampleDist, float sigma)
{
    float g = 1.0f / sqrt(2.0f * 3.14159 * sigma * sigma);
    return (g * exp(-(sampleDist * sampleDist) / (2 * sigma * sigma)));
}

// Performs a gaussian blur in one direction
float4 Blur(in PSInput input, float2 texScale, float sigma)
{
    float weightSum = 0.0f;
    float4 color = 0;
    for (int i = -10; i < 10; i++)
    {
        float weight = CalcGaussianWeight(i, sigma);
        weightSum += weight;
        float2 texCoord = input.TexCoord;
        texCoord += (i / InputSize0) * texScale;
        float4 sample = InputTexture0.Sample(PointSampler, texCoord);
        color += sample * weight;
    }

    color /= weightSum;

    return color;
}

// ================================================================================================
// Shader Entry Points
// ================================================================================================

Texture2D<float4> BloomInput : register(t0);

// Initial pass for bloom
float4 Bloom(in PSInput input) : SV_Target
{
    float4 reds = BloomInput.GatherRed(LinearSampler, input.TexCoord);
    float4 greens = BloomInput.GatherGreen(LinearSampler, input.TexCoord);
    float4 blues = BloomInput.GatherBlue(LinearSampler, input.TexCoord);

    float3 result = 0.0f;
    float weightSum = 0.0f;

    [unroll]
    for(uint i = 0; i < 4; ++i)
    {
        float3 color = float3(reds[i], greens[i], blues[i]);

        // Apply exposure offset
        float avgLuminance = GetAvgLuminance(InputTexture1);
        float exposure = 0;
        result += CalcExposedColor(color, avgLuminance, BloomExposure, exposure);

        // float weight = 1.0f / (1.0f + CalcLuminance(color));
        float weight = 1.0f;
        result *= weight;
        weightSum += weight;
    }

    // result /= 4.0f;
    result /= weightSum;

    return float4(result, 1.0f);
}

// Uses hw bilinear filtering for upscaling or downscaling
float4 Scale(in PSInput input) : SV_Target
{
    return InputTexture0.Sample(PointSampler, input.TexCoord);
}

// Horizontal gaussian blur
float4 BlurH(in PSInput input) : SV_Target
{
    return Blur(input, float2(1, 0), BloomBlurSigma);
}

// Vertical gaussian blur
float4 BlurV(in PSInput input) : SV_Target
{
    return Blur(input, float2(0, 1), BloomBlurSigma);
}

// Applies exposure and tone mapping to the input
float4 ToneMap(in PSInput input) : SV_Target0
{
    if(EnableZoom)
    {
        float2 uv = input.TexCoord * 2.0f - 1.0f;
        uv /= 4.0f;
        input.TexCoord = uv * 0.5f + 0.5f;
    }

    // Tone map the primary input
    float avgLuminance = GetAvgLuminance(InputTexture1);
    float3 color = InputTexture0.Sample(PointSampler, input.TexCoord).rgb;

    color += InputTexture2.Sample(LinearSampler, input.TexCoord).xyz * BloomMagnitude;

    float exposure = 0;
    color = ToneMap(color, avgLuminance, 0, exposure);

    return float4(color, 1.0f);
}

float4 Sharpen(in PSInput input) : SV_Target0
{
    float2 pixelPos = input.PositionSS.xy;
    float3 inputColor = InputTexture0[uint2(pixelPos)].xyz;
    float inputLuminance = CalcLuminance(inputColor);

    float avgLuminance = 0.0f;

    for(float y = -1.0f; y <= 1.0f; ++y)
    {
        for(float x = -1.0f; x <= 1.0f; ++x)
        {
            avgLuminance += CalcLuminance(InputTexture0[uint2(pixelPos + float2(x, y))].xyz);
        }
    }

    avgLuminance /= 9.0f;

    float sharpenedLuminance = inputLuminance - avgLuminance;
    float finalLuminance = inputLuminance + sharpenedLuminance * SharpeningAmount;
    float3 finalColor = inputColor * (finalLuminance / inputLuminance);
    return float4(finalColor, 1.0f);
}