#pragma once

#include <PCH.h>
#include <Settings.h>
#include <Graphics\GraphicsTypes.h>

using namespace SampleFramework11;

enum class MSAAModes
{
    MSAANone = 0,
    MSAA2x = 1,
    MSAA4x = 2,
    MSAA8x = 3,

    NumValues
};

typedef EnumSettingT<MSAAModes> MSAAModesSetting;

enum class FilterTypes
{
    Box = 0,
    Triangle = 1,
    Gaussian = 2,
    BlackmanHarris = 3,
    Smoothstep = 4,
    BSpline = 5,
    CatmullRom = 6,
    Mitchell = 7,
    GeneralizedCubic = 8,
    Sinc = 9,

    NumValues
};

typedef EnumSettingT<FilterTypes> FilterTypesSetting;

enum class ClampModes
{
    Disabled = 0,
    RGB_Clamp = 1,
    RGB_Clip = 2,
    Variance_Clip = 3,

    NumValues
};

typedef EnumSettingT<ClampModes> ClampModesSetting;

enum class JitterModes
{
    None = 0,
    Uniform2x = 1,
    Hammersley4x = 2,
    Hammersley8x = 3,
    Hammersley16x = 4,

    NumValues
};

typedef EnumSettingT<JitterModes> JitterModesSetting;

enum class DilationModes
{
    CenterAverage = 0,
    DilateNearestDepth = 1,
    DilateGreatestVelocity = 2,

    NumValues
};

typedef EnumSettingT<DilationModes> DilationModesSetting;

enum class Scenes
{
    RoboHand = 0,
    BrickPlane = 1,
    UIPlane = 2,
    Soldier = 3,
    Tower = 4,

    NumValues
};

typedef EnumSettingT<Scenes> ScenesSetting;

namespace AppSettings
{
    static const bool EnableAutoExposure = false;
    static const float KeyValue = 0.1150f;
    static const float AdaptationRate = 0.5000f;

    extern MSAAModesSetting MSAAMode;
    extern FilterTypesSetting ResolveFilterType;
    extern FloatSetting ResolveFilterDiameter;
    extern FloatSetting GaussianSigma;
    extern FloatSetting CubicB;
    extern FloatSetting CubicC;
    extern BoolSetting UseStandardResolve;
    extern BoolSetting InverseLuminanceFiltering;
    extern BoolSetting UseExposureFiltering;
    extern FloatSetting ExposureFilterOffset;
    extern BoolSetting UseGradientMipLevel;
    extern BoolSetting CentroidSampling;
    extern BoolSetting EnableTemporalAA;
    extern FloatSetting TemporalAABlendFactor;
    extern BoolSetting UseTemporalColorWeighting;
    extern ClampModesSetting NeighborhoodClampMode;
    extern FloatSetting VarianceClipGamma;
    extern JitterModesSetting JitterMode;
    extern FloatSetting JitterScale;
    extern FloatSetting LowFreqWeight;
    extern FloatSetting HiFreqWeight;
    extern FloatSetting SharpeningAmount;
    extern DilationModesSetting DilationMode;
    extern FloatSetting MipBias;
    extern FilterTypesSetting ReprojectionFilter;
    extern BoolSetting UseStandardReprojection;
    extern ScenesSetting CurrentScene;
    extern DirectionSetting LightDirection;
    extern ColorSetting LightColor;
    extern BoolSetting EnableDirectLighting;
    extern BoolSetting EnableAmbientLighting;
    extern BoolSetting RenderBackground;
    extern BoolSetting EnableShadows;
    extern BoolSetting EnableNormalMaps;
    extern FloatSetting NormalMapIntensity;
    extern FloatSetting DiffuseIntensity;
    extern FloatSetting Roughness;
    extern FloatSetting SpecularIntensity;
    extern OrientationSetting ModelOrientation;
    extern FloatSetting ModelRotationSpeed;
    extern BoolSetting DoubleSyncInterval;
    extern FloatSetting ExposureScale;
    extern BoolSetting EnableZoom;
    extern FloatSetting BloomExposure;
    extern FloatSetting BloomMagnitude;
    extern FloatSetting BloomBlurSigma;
    extern FloatSetting ManualExposure;

    struct AppSettingsCBuffer
    {
        int32 MSAAMode;
        int32 ResolveFilterType;
        float ResolveFilterDiameter;
        float GaussianSigma;
        float CubicB;
        float CubicC;
        bool32 UseStandardResolve;
        bool32 InverseLuminanceFiltering;
        bool32 UseExposureFiltering;
        float ExposureFilterOffset;
        bool32 UseGradientMipLevel;
        bool32 EnableTemporalAA;
        float TemporalAABlendFactor;
        bool32 UseTemporalColorWeighting;
        int32 NeighborhoodClampMode;
        float VarianceClipGamma;
        float LowFreqWeight;
        float HiFreqWeight;
        float SharpeningAmount;
        int32 DilationMode;
        float MipBias;
        int32 ReprojectionFilter;
        bool32 UseStandardReprojection;
        int32 CurrentScene;
        Float3 LightDirection;
        Float4Align Float3 LightColor;
        bool32 EnableDirectLighting;
        bool32 EnableAmbientLighting;
        bool32 RenderBackground;
        bool32 EnableShadows;
        bool32 EnableNormalMaps;
        float NormalMapIntensity;
        float DiffuseIntensity;
        float Roughness;
        float SpecularIntensity;
        Float4Align Quaternion ModelOrientation;
        float ModelRotationSpeed;
        bool32 DoubleSyncInterval;
        float ExposureScale;
        bool32 EnableZoom;
        float BloomExposure;
        float BloomMagnitude;
        float BloomBlurSigma;
        float ManualExposure;
    };

    extern ConstantBuffer<AppSettingsCBuffer> CBuffer;

    void Initialize(ID3D11Device* device);
    void Update();
    void UpdateCBuffer(ID3D11DeviceContext* context);
};

// ================================================================================================

namespace AppSettings
{
    inline uint32 NumMSAASamples(MSAAModes mode)
    {
        static const uint32 NumSamples[uint32(MSAAModes::NumValues)] = { 1, 2, 4, 8 };
        return NumSamples[uint32(mode)];
    }

    inline uint32 NumMSAASamples()
    {
        return NumMSAASamples(MSAAMode);
    }

    inline bool UseNormalMapping()
    {
        return CurrentScene != Scenes::RoboHand;
    }

    inline bool EnableJitter()
    {
        return JitterMode != JitterModes::None;
    }

    void UpdateUI();
}