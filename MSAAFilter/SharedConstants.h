//=================================================================================================
//
//  MSAA Filtering 2.0 Sample
//  by MJP
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#if _WINDOWS

#pragma once

typedef SampleFramework11::Float2 float2;
typedef SampleFramework11::Float3 float3;
typedef SampleFramework11::Float4 float4;

typedef uint32 uint;
typedef SampleFramework11::Uint2 uint2;
typedef SampleFramework11::Uint3 uint3;
typedef SampleFramework11::Uint4 uint4;

#endif

static const uint ReductionTGSize = 16;
static const uint ConvolveTGSize = 16;

// Info about a active sample point on the light map
struct SamplePoint
{
    float3 Position;
    float3 Normal;
    float3 Tangent;
    float3 Bitangent;
    uint Coverage;
    uint2 TexelPos;
};

// Single vertex in the mesh vertex buffer
struct Vertex
{
    float3 Position;
    float3 Normal;
    float2 TexCoord;
    float2 LightMapUV;
	float3 Tangent;
	float3 Bitangent;
};

#if _WINDOWS

// Operators for Vertex structure
// operators
inline Vertex operator+(const Vertex& a, const Vertex& b)
{
    Vertex c;
    c.Position = a.Position + b.Position;
    c.Normal = a.Normal + b.Normal;
    c.TexCoord = a.TexCoord + b.TexCoord;
    c.LightMapUV = a.LightMapUV + b.LightMapUV;
    c.Tangent = a.Tangent + b.Tangent;
    c.Bitangent = a.Bitangent + b.Bitangent;
    return c;
}

inline Vertex operator-(const Vertex& a, const Vertex& b)
{
    Vertex c;
    c.Position = a.Position - b.Position;
    c.Normal = a.Normal - b.Normal;
    c.TexCoord = a.TexCoord - b.TexCoord;
    c.LightMapUV = a.LightMapUV - b.LightMapUV;
    c.Tangent = a.Tangent - b.Tangent;
    c.Bitangent = a.Bitangent - b.Bitangent;
    return c;
}

inline Vertex operator*(const Vertex& a, float b)
{
    Vertex c;
    c.Position = a.Position * b;
    c.Normal = a.Normal * b;
    c.TexCoord = a.TexCoord * b;
    c.LightMapUV = a.LightMapUV * b;
    c.Tangent = a.Tangent * b;
    c.Bitangent = a.Bitangent * b;
    return c;
}

#endif