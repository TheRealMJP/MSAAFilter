cbuffer AppSettings : register(b7)
{
    int MSAAMode;
    int FilterType;
    float FilterSize;
    float GaussianSigma;
    float CubicB;
    float CubicC;
    bool UseStandardResolve;
    bool InverseLuminanceFiltering;
    bool UseExposureFiltering;
    float ExposureFilterOffset;
    bool UseGradientMipLevel;
    bool EnableTemporalAA;
    float TemporalAABlendFactor;
    bool UseTemporalColorWeighting;
    int NeighborhoodClampMode;
    float VarianceClipGamma;
    float LowFreqWeight;
    float HiFreqWeight;
    float SharpeningAmount;
    int DilationMode;
    int CurrentScene;
    float3 LightDirection;
    float3 LightColor;
    bool EnableDirectLighting;
    bool EnableAmbientLighting;
    bool RenderBackground;
    bool EnableShadows;
    bool EnableNormalMaps;
    float NormalMapIntensity;
    float DiffuseIntensity;
    float Roughness;
    float SpecularIntensity;
    float4 ModelOrientation;
    float ModelRotationSpeed;
    bool DoubleSyncInterval;
    float ExposureScale;
    float BloomExposure;
    float BloomMagnitude;
    float BloomBlurSigma;
    float ManualExposure;
}

static const int MSAAModes_MSAANone = 0;
static const int MSAAModes_MSAA2x = 1;
static const int MSAAModes_MSAA4x = 2;
static const int MSAAModes_MSAA8x = 3;

static const int FilterTypes_Box = 0;
static const int FilterTypes_Triangle = 1;
static const int FilterTypes_Gaussian = 2;
static const int FilterTypes_BlackmanHarris = 3;
static const int FilterTypes_Smoothstep = 4;
static const int FilterTypes_BSpline = 5;
static const int FilterTypes_CatmullRom = 6;
static const int FilterTypes_Mitchell = 7;
static const int FilterTypes_GeneralizedCubic = 8;
static const int FilterTypes_Sinc = 9;

static const int ClampModes_Disabled = 0;
static const int ClampModes_RGB_Clamp = 1;
static const int ClampModes_RGB_Clip = 2;
static const int ClampModes_Variance_Clip = 3;

static const int JitterModes_None = 0;
static const int JitterModes_Uniform2x = 1;
static const int JitterModes_Hammersley4x = 2;
static const int JitterModes_Hammersley8x = 3;
static const int JitterModes_Hammersley16x = 4;

static const int DilationModes_CenterAverage = 0;
static const int DilationModes_DilateNearestDepth = 1;
static const int DilationModes_DilateGreatestVelocity = 2;

static const int Scenes_RoboHand = 0;
static const int Scenes_Plane = 1;
static const int Scenes_Soldier = 2;
static const int Scenes_Tower = 3;

static const bool EnableAutoExposure = false;
static const float KeyValue = 0.1150f;
static const float AdaptationRate = 0.5000f;
