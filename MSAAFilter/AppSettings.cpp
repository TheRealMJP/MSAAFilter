#include <PCH.h>
#include <TwHelper.h>
#include "AppSettings.h"

using namespace SampleFramework11;

static const char* MSAAModesLabels[4] =
{
    "None",
    "2x",
    "4x",
    "8x",
};

static const char* FilterTypesLabels[10] =
{
    "Box",
    "Triangle",
    "Gaussian",
    "Blackman-Harris",
    "Smoothstep",
    "B-Spline",
    "Catmull-Rom",
    "Mitchell",
    "Generalized Cubic",
    "Sinc",
};

static const char* ClampModesLabels[4] =
{
    "Disabled",
    "RGB Clamp",
    "RGB Clip",
    "Variance Clip",
};

static const char* JitterModesLabels[5] =
{
    "None",
    "Uniform 2x",
    "Hammersley 4x",
    "Hammersley 8x",
    "Hammersley 16x",
};

static const char* DilationModesLabels[3] =
{
    "Center Average",
    "Dilate - Nearest Depth",
    "Dilate - Greatest Velocity",
};

static const char* ScenesLabels[5] =
{
    "RoboHand",
    "Plane (Bricks)",
    "Plane (UI)",
    "Soldier",
    "Tower",
};

namespace AppSettings
{
    MSAAModesSetting MSAAMode;
    FilterTypesSetting ResolveFilterType;
    FloatSetting ResolveFilterDiameter;
    FloatSetting GaussianSigma;
    FloatSetting CubicB;
    FloatSetting CubicC;
    BoolSetting UseStandardResolve;
    BoolSetting InverseLuminanceFiltering;
    BoolSetting UseExposureFiltering;
    FloatSetting ExposureFilterOffset;
    BoolSetting UseGradientMipLevel;
    BoolSetting CentroidSampling;
    BoolSetting EnableTemporalAA;
    FloatSetting TemporalAABlendFactor;
    BoolSetting UseTemporalColorWeighting;
    ClampModesSetting NeighborhoodClampMode;
    FloatSetting VarianceClipGamma;
    JitterModesSetting JitterMode;
    FloatSetting JitterScale;
    FloatSetting LowFreqWeight;
    FloatSetting HiFreqWeight;
    FloatSetting SharpeningAmount;
    DilationModesSetting DilationMode;
    FloatSetting MipBias;
    FilterTypesSetting ReprojectionFilter;
    BoolSetting UseStandardReprojection;
    ScenesSetting CurrentScene;
    DirectionSetting LightDirection;
    ColorSetting LightColor;
    BoolSetting EnableDirectLighting;
    BoolSetting EnableAmbientLighting;
    BoolSetting RenderBackground;
    BoolSetting EnableShadows;
    BoolSetting EnableNormalMaps;
    FloatSetting NormalMapIntensity;
    FloatSetting DiffuseIntensity;
    FloatSetting Roughness;
    FloatSetting SpecularIntensity;
    OrientationSetting ModelOrientation;
    FloatSetting ModelRotationSpeed;
    BoolSetting DoubleSyncInterval;
    FloatSetting ExposureScale;
    BoolSetting EnableZoom;
    FloatSetting BloomExposure;
    FloatSetting BloomMagnitude;
    FloatSetting BloomBlurSigma;
    FloatSetting ManualExposure;

    ConstantBuffer<AppSettingsCBuffer> CBuffer;

    void Initialize(ID3D11Device* device)
    {
        TwBar* tweakBar = Settings.TweakBar();

        MSAAMode.Initialize(tweakBar, "MSAAMode", "Anti Aliasing", "MSAAMode", "", MSAAModes::MSAA4x, 4, MSAAModesLabels);
        Settings.AddSetting(&MSAAMode);

        ResolveFilterType.Initialize(tweakBar, "ResolveFilterType", "Anti Aliasing", "Resolve Filter Type", "", FilterTypes::BSpline, 10, FilterTypesLabels);
        Settings.AddSetting(&ResolveFilterType);

        ResolveFilterDiameter.Initialize(tweakBar, "ResolveFilterDiameter", "Anti Aliasing", "Resolve Filter Diameter", "", 2.0000f, 0.0000f, 6.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&ResolveFilterDiameter);

        GaussianSigma.Initialize(tweakBar, "GaussianSigma", "Anti Aliasing", "Gaussian Sigma", "", 0.5000f, 0.0100f, 1.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&GaussianSigma);

        CubicB.Initialize(tweakBar, "CubicB", "Anti Aliasing", "Cubic B", "", 0.3300f, 0.0000f, 1.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&CubicB);

        CubicC.Initialize(tweakBar, "CubicC", "Anti Aliasing", "Cubic C", "", 0.3300f, 0.0000f, 1.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&CubicC);

        UseStandardResolve.Initialize(tweakBar, "UseStandardResolve", "Anti Aliasing", "Use Standard Resolve", "", false);
        Settings.AddSetting(&UseStandardResolve);

        InverseLuminanceFiltering.Initialize(tweakBar, "InverseLuminanceFiltering", "Anti Aliasing", "Inverse Luminance Filtering", "", true);
        Settings.AddSetting(&InverseLuminanceFiltering);

        UseExposureFiltering.Initialize(tweakBar, "UseExposureFiltering", "Anti Aliasing", "Use Exposure Filtering", "", true);
        Settings.AddSetting(&UseExposureFiltering);

        ExposureFilterOffset.Initialize(tweakBar, "ExposureFilterOffset", "Anti Aliasing", "Exposure Filter Offset", "", 2.0000f, -16.0000f, 16.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&ExposureFilterOffset);

        UseGradientMipLevel.Initialize(tweakBar, "UseGradientMipLevel", "Anti Aliasing", "Use Gradient Mip Level", "", false);
        Settings.AddSetting(&UseGradientMipLevel);

        CentroidSampling.Initialize(tweakBar, "CentroidSampling", "Anti Aliasing", "Centroid Sampling", "", true);
        Settings.AddSetting(&CentroidSampling);

        EnableTemporalAA.Initialize(tweakBar, "EnableTemporalAA", "Anti Aliasing", "Enable Temporal AA", "", true);
        Settings.AddSetting(&EnableTemporalAA);

        TemporalAABlendFactor.Initialize(tweakBar, "TemporalAABlendFactor", "Anti Aliasing", "Temporal AABlend Factor", "", 0.9000f, 0.0000f, 1.0000f, 0.0010f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&TemporalAABlendFactor);

        UseTemporalColorWeighting.Initialize(tweakBar, "UseTemporalColorWeighting", "Anti Aliasing", "Use Temporal Color Weighting", "", false);
        Settings.AddSetting(&UseTemporalColorWeighting);

        NeighborhoodClampMode.Initialize(tweakBar, "NeighborhoodClampMode", "Anti Aliasing", "Neighborhood Clamp Mode", "", ClampModes::Variance_Clip, 4, ClampModesLabels);
        Settings.AddSetting(&NeighborhoodClampMode);

        VarianceClipGamma.Initialize(tweakBar, "VarianceClipGamma", "Anti Aliasing", "Variance Clip Gamma", "", 1.5000f, 0.0000f, 2.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&VarianceClipGamma);

        JitterMode.Initialize(tweakBar, "JitterMode", "Anti Aliasing", "Jitter Mode", "", JitterModes::Hammersley4x, 5, JitterModesLabels);
        Settings.AddSetting(&JitterMode);

        JitterScale.Initialize(tweakBar, "JitterScale", "Anti Aliasing", "Jitter Scale", "", 1.0000f, 0.0000f, 340282300000000000000000000000000000000.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&JitterScale);

        LowFreqWeight.Initialize(tweakBar, "LowFreqWeight", "Anti Aliasing", "Low Freq Weight", "", 0.2500f, 0.0000f, 100.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&LowFreqWeight);

        HiFreqWeight.Initialize(tweakBar, "HiFreqWeight", "Anti Aliasing", "Hi Freq Weight", "", 0.8500f, 0.0000f, 100.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&HiFreqWeight);

        SharpeningAmount.Initialize(tweakBar, "SharpeningAmount", "Anti Aliasing", "Sharpening Amount", "", 0.0000f, 0.0000f, 1.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&SharpeningAmount);

        DilationMode.Initialize(tweakBar, "DilationMode", "Anti Aliasing", "Dilation Mode", "", DilationModes::DilateNearestDepth, 3, DilationModesLabels);
        Settings.AddSetting(&DilationMode);

        MipBias.Initialize(tweakBar, "MipBias", "Anti Aliasing", "Mip Bias", "", 0.0000f, -340282300000000000000000000000000000000.0000f, 0.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&MipBias);

        ReprojectionFilter.Initialize(tweakBar, "ReprojectionFilter", "Anti Aliasing", "Reprojection Filter", "", FilterTypes::CatmullRom, 10, FilterTypesLabels);
        Settings.AddSetting(&ReprojectionFilter);

        UseStandardReprojection.Initialize(tweakBar, "UseStandardReprojection", "Anti Aliasing", "Use Standard Reprojection", "", false);
        Settings.AddSetting(&UseStandardReprojection);

        CurrentScene.Initialize(tweakBar, "CurrentScene", "Scene Controls", "Current Scene", "", Scenes::RoboHand, 5, ScenesLabels);
        Settings.AddSetting(&CurrentScene);

        LightDirection.Initialize(tweakBar, "LightDirection", "Scene Controls", "Light Direction", "The direction of the light", Float3(-0.7500f, 0.9770f, -0.4000f));
        Settings.AddSetting(&LightDirection);

        LightColor.Initialize(tweakBar, "LightColor", "Scene Controls", "Light Color", "The color of the light", Float3(20.0000f, 16.0000f, 10.0000f), true, 0.0000f, 20.0000f, 0.1000f, ColorUnit::None);
        Settings.AddSetting(&LightColor);

        EnableDirectLighting.Initialize(tweakBar, "EnableDirectLighting", "Scene Controls", "Enable Direct Lighting", "Enables direct lighting", true);
        Settings.AddSetting(&EnableDirectLighting);

        EnableAmbientLighting.Initialize(tweakBar, "EnableAmbientLighting", "Scene Controls", "Enable Ambient Lighting", "Enables ambient lighting from the environment", true);
        Settings.AddSetting(&EnableAmbientLighting);

        RenderBackground.Initialize(tweakBar, "RenderBackground", "Scene Controls", "Render Background", "", true);
        Settings.AddSetting(&RenderBackground);

        EnableShadows.Initialize(tweakBar, "EnableShadows", "Scene Controls", "Enable Shadows", "", true);
        Settings.AddSetting(&EnableShadows);

        EnableNormalMaps.Initialize(tweakBar, "EnableNormalMaps", "Scene Controls", "Enable Normal Maps", "", true);
        Settings.AddSetting(&EnableNormalMaps);

        NormalMapIntensity.Initialize(tweakBar, "NormalMapIntensity", "Scene Controls", "Normal Map Intensity", "", 1.0000f, 0.0000f, 1.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&NormalMapIntensity);

        DiffuseIntensity.Initialize(tweakBar, "DiffuseIntensity", "Scene Controls", "Diffuse Intensity", "Diffuse albedo intensity parameter for the material", 0.5000f, 0.0000f, 1.0000f, 0.0010f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&DiffuseIntensity);

        Roughness.Initialize(tweakBar, "Roughness", "Scene Controls", "Roughness", "Specular roughness parameter for the material", 0.1000f, 0.0010f, 1.0000f, 0.0010f, ConversionMode::Square, 1.0000f);
        Settings.AddSetting(&Roughness);

        SpecularIntensity.Initialize(tweakBar, "SpecularIntensity", "Scene Controls", "Specular Intensity", "Specular intensity parameter for the material", 0.0500f, 0.0000f, 1.0000f, 0.0010f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&SpecularIntensity);

        ModelOrientation.Initialize(tweakBar, "ModelOrientation", "Scene Controls", "Model Orientation", "", Quaternion(0.4100f, -0.5500f, -0.2900f, 0.6700f));
        Settings.AddSetting(&ModelOrientation);

        ModelRotationSpeed.Initialize(tweakBar, "ModelRotationSpeed", "Scene Controls", "Model Rotation Speed", "", 0.0000f, 0.0000f, 10.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&ModelRotationSpeed);

        DoubleSyncInterval.Initialize(tweakBar, "DoubleSyncInterval", "Scene Controls", "Double Sync Interval", "", false);
        Settings.AddSetting(&DoubleSyncInterval);

        ExposureScale.Initialize(tweakBar, "ExposureScale", "Scene Controls", "Exposure Scale", "", 0.0000f, -16.0000f, 16.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&ExposureScale);

        EnableZoom.Initialize(tweakBar, "EnableZoom", "Scene Controls", "Enable Zoom", "", false);
        Settings.AddSetting(&EnableZoom);

        BloomExposure.Initialize(tweakBar, "BloomExposure", "Post Processing", "Bloom Exposure Offset", "Exposure offset applied to generate the input of the bloom pass", -4.0000f, -10.0000f, 0.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&BloomExposure);

        BloomMagnitude.Initialize(tweakBar, "BloomMagnitude", "Post Processing", "Bloom Magnitude", "Scale factor applied to the bloom results when combined with tone-mapped result", 1.0000f, 0.0000f, 2.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&BloomMagnitude);

        BloomBlurSigma.Initialize(tweakBar, "BloomBlurSigma", "Post Processing", "Bloom Blur Sigma", "Sigma parameter of the Gaussian filter used in the bloom pass", 5.0000f, 0.5000f, 5.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&BloomBlurSigma);

        ManualExposure.Initialize(tweakBar, "ManualExposure", "Post Processing", "Manual Exposure", "Manual exposure value when auto-exposure is disabled", -2.5000f, -10.0000f, 10.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&ManualExposure);

        TwHelper::SetOpened(tweakBar, "Anti Aliasing", true);

        TwHelper::SetOpened(tweakBar, "Scene Controls", true);

        TwHelper::SetOpened(tweakBar, "Post Processing", true);

        CBuffer.Initialize(device);
    }

    void Update()
    {
    }

    void UpdateCBuffer(ID3D11DeviceContext* context)
    {
        CBuffer.Data.MSAAMode = MSAAMode;
        CBuffer.Data.ResolveFilterType = ResolveFilterType;
        CBuffer.Data.ResolveFilterDiameter = ResolveFilterDiameter;
        CBuffer.Data.GaussianSigma = GaussianSigma;
        CBuffer.Data.CubicB = CubicB;
        CBuffer.Data.CubicC = CubicC;
        CBuffer.Data.UseStandardResolve = UseStandardResolve;
        CBuffer.Data.InverseLuminanceFiltering = InverseLuminanceFiltering;
        CBuffer.Data.UseExposureFiltering = UseExposureFiltering;
        CBuffer.Data.ExposureFilterOffset = ExposureFilterOffset;
        CBuffer.Data.UseGradientMipLevel = UseGradientMipLevel;
        CBuffer.Data.EnableTemporalAA = EnableTemporalAA;
        CBuffer.Data.TemporalAABlendFactor = TemporalAABlendFactor;
        CBuffer.Data.UseTemporalColorWeighting = UseTemporalColorWeighting;
        CBuffer.Data.NeighborhoodClampMode = NeighborhoodClampMode;
        CBuffer.Data.VarianceClipGamma = VarianceClipGamma;
        CBuffer.Data.LowFreqWeight = LowFreqWeight;
        CBuffer.Data.HiFreqWeight = HiFreqWeight;
        CBuffer.Data.SharpeningAmount = SharpeningAmount;
        CBuffer.Data.DilationMode = DilationMode;
        CBuffer.Data.MipBias = MipBias;
        CBuffer.Data.ReprojectionFilter = ReprojectionFilter;
        CBuffer.Data.UseStandardReprojection = UseStandardReprojection;
        CBuffer.Data.CurrentScene = CurrentScene;
        CBuffer.Data.LightDirection = LightDirection;
        CBuffer.Data.LightColor = LightColor;
        CBuffer.Data.EnableDirectLighting = EnableDirectLighting;
        CBuffer.Data.EnableAmbientLighting = EnableAmbientLighting;
        CBuffer.Data.RenderBackground = RenderBackground;
        CBuffer.Data.EnableShadows = EnableShadows;
        CBuffer.Data.EnableNormalMaps = EnableNormalMaps;
        CBuffer.Data.NormalMapIntensity = NormalMapIntensity;
        CBuffer.Data.DiffuseIntensity = DiffuseIntensity;
        CBuffer.Data.Roughness = Roughness;
        CBuffer.Data.SpecularIntensity = SpecularIntensity;
        CBuffer.Data.ModelOrientation = ModelOrientation;
        CBuffer.Data.ModelRotationSpeed = ModelRotationSpeed;
        CBuffer.Data.DoubleSyncInterval = DoubleSyncInterval;
        CBuffer.Data.ExposureScale = ExposureScale;
        CBuffer.Data.EnableZoom = EnableZoom;
        CBuffer.Data.BloomExposure = BloomExposure;
        CBuffer.Data.BloomMagnitude = BloomMagnitude;
        CBuffer.Data.BloomBlurSigma = BloomBlurSigma;
        CBuffer.Data.ManualExposure = ManualExposure;

        CBuffer.ApplyChanges(context);
        CBuffer.SetVS(context, 7);
        CBuffer.SetHS(context, 7);
        CBuffer.SetDS(context, 7);
        CBuffer.SetGS(context, 7);
        CBuffer.SetPS(context, 7);
        CBuffer.SetCS(context, 7);
    }
}

// ================================================================================================

namespace AppSettings
{
    void UpdateUI()
    {
        ExposureFilterOffset.SetEditable(UseExposureFiltering.Value() ? true : false);

        bool enableTemporal = EnableTemporalAA.Value() ? true : false;
        JitterMode.SetEditable(enableTemporal);
        UseTemporalColorWeighting.SetEditable(enableTemporal);
        NeighborhoodClampMode.SetEditable(enableTemporal);
        LowFreqWeight.SetEditable(enableTemporal);
        HiFreqWeight.SetEditable(enableTemporal);
        DilationMode.SetEditable(enableTemporal);

        bool generalCubic = ResolveFilterType == FilterTypes::GeneralizedCubic;
        CubicB.SetEditable(generalCubic);
        CubicC.SetEditable(generalCubic);
        GaussianSigma.SetEditable(ResolveFilterType == FilterTypes::Gaussian);
    }
}
