//=================================================================================================
//
//  MSAA Filtering 2.0 Sample
//  by MJP
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

// ================================================================================================
// Constant buffers
// ================================================================================================
cbuffer VSConstants : register(cb0)
{
    float4x4 World;
	float4x4 View;
    float4x4 WorldViewProjection;
}

// ================================================================================================
// Input/Output structs
// ================================================================================================
struct VSInput
{
    float4 PositionOS 		: POSITION;
};

struct VSOutput
{
    float4 PositionCS 		: SV_Position;
};

// ================================================================================================
// Vertex Shader
// ================================================================================================
VSOutput VS(in VSInput input)
{
    VSOutput output;

    // Calc the clip-space position
    output.PositionCS = mul(input.PositionOS, WorldViewProjection);

    return output;
}