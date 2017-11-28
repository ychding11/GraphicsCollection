//--------------------------------------------------------------------------------------
// This sample shows an simple implementation of the DirectX 11 Hardware Tessellator
// for rendering a Bezier Patch.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

// This allows us to compile the shader with a #define to choose
// the different partition modes for the hull shader.
// See the hull shader: [partitioning(BEZIER_HS_PARTITION)]
// This sample demonstrates "integer", "fractional_even", and "fractional_odd"
#ifndef BEZIER_HS_PARTITION
#define BEZIER_HS_PARTITION "integer"
#endif // BEZIER_HS_PARTITION

// The input patch size.  In this sample, it is 16 control points.
// This value should match the call to IASetPrimitiveTopology()
#define INPUT_PATCH_SIZE 16

// The output patch size.  In this sample, it is also 16 control points.
#define OUTPUT_PATCH_SIZE 16

Texture2D txSkin       : register(t0);
SamplerState samLinear : register(s0);

//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------
cbuffer cbPerFrame : register( b0 )
{
    row_major matrix g_mWorld; // column_major is default.
    matrix g_mViewProjection;
    float3 g_vCameraPosWorld;
    float  g_fTessellationFactor;
};

cbuffer cbMaterial : register(b1)
{
	float  Ka; //ambient
	float  Kd; // diffuse
	float  Ks; // specular
	float  shininess; // shininesss
};

//--------------------------------------------------------------------------------------
// Vertex shader section
//--------------------------------------------------------------------------------------
struct VS_CONTROL_POINT_INPUT
{
    float3 vPosition        : POSITION;
};

struct VS_CONTROL_POINT_OUTPUT
{
    float3 vPosition        : POSITION;
};

// This simple vertex shader passes the control points straight through to the
// hull shader.  In a more complex scene, you might transform the control points
// or perform skinning at this step.

// The input to the vertex shader comes from the vertex buffer.
// The output from the vertex shader will go into the hull shader.
VS_CONTROL_POINT_OUTPUT BezierVS( VS_CONTROL_POINT_INPUT Input )
{
    VS_CONTROL_POINT_OUTPUT Output;
    Output.vPosition = mul(float4(Input.vPosition,1.0), g_mWorld).xyz;
    return Output;
}

//--------------------------------------------------------------------------------------
// Constant data function for the BezierHS.  This is executed once per patch.
//--------------------------------------------------------------------------------------
struct HS_CONSTANT_DATA_OUTPUT
{
    float Edges[4]             : SV_TessFactor;
    float Inside[2]            : SV_InsideTessFactor;
};

struct HS_OUTPUT
{
    float3 vPosition           : BEZIERPOS;
};

// This constant hull shader is executed once per patch.  For the simple Mobius strip
// model, it will be executed 4 times.  In this sample, we set the tessellation factor
// via SV_TessFactor and SV_InsideTessFactor for each patch.  In a more complex scene,
// you might calculate a variable tessellation factor based on the camera's distance.

HS_CONSTANT_DATA_OUTPUT BezierConstantHS( InputPatch<VS_CONTROL_POINT_OUTPUT, INPUT_PATCH_SIZE> ip,
                                          uint PatchID : SV_PrimitiveID )
{    
    HS_CONSTANT_DATA_OUTPUT Output;

    float TessAmount = g_fTessellationFactor;
    Output.Edges[0] = Output.Edges[1] = Output.Edges[2] = Output.Edges[3] = TessAmount;
    Output.Inside[0] = Output.Inside[1] = TessAmount;
    return Output;
}

// The hull shader is called once per output control point, which is specified with
// outputcontrolpoints.  For this sample, we take the control points from the vertex
// shader and pass them directly off to the domain shader.  In a more complex scene,
// you might perform a basis conversion from the input control points into a Bezier
// patch, such as the SubD11 Sample.

// The input to the hull shader comes from the vertex shader

// The output from the hull shader will go to the domain shader.
// The tessellation factor, topology, and partition mode will go to the fixed function
// tessellator stage to calculate the UVW and domain points.

[domain("quad")]
[partitioning(BEZIER_HS_PARTITION)]
[outputtopology("triangle_cw")]
[outputcontrolpoints(OUTPUT_PATCH_SIZE)]
[patchconstantfunc("BezierConstantHS")]
HS_OUTPUT BezierHS( InputPatch<VS_CONTROL_POINT_OUTPUT, INPUT_PATCH_SIZE> p, 
                    uint i : SV_OutputControlPointID,
                    uint PatchID : SV_PrimitiveID )
{
    HS_OUTPUT Output;
    Output.vPosition = p[i].vPosition;
    return Output;
}

//--------------------------------------------------------------------------------------
// Bezier evaluation domain shader section
//--------------------------------------------------------------------------------------
struct DS_OUTPUT
{
    float4 vPosition        : SV_POSITION;
    float3 vWorldPos        : WORLDPOS;
    float3 vNormal          : NORMAL;
    float2 vtex             : TEX;
};

//--------------------------------------------------------------------------------------
float4 BernsteinBasis(float t)
{
    float invT = 1.0f - t;

    return float4( invT * invT * invT,
                   3.0f * t * invT * invT,
                   3.0f * t * t * invT,
                   t * t * t );
}

//--------------------------------------------------------------------------------------
float4 dBernsteinBasis(float t)
{
    float invT = 1.0f - t;

    return float4( -3 * invT * invT,
                   3 * invT * invT - 6 * t * invT,
                   6 * t * invT - 3 * t * t,
                   3 * t * t );
}

//--------------------------------------------------------------------------------------
float3 EvaluateBezier( const OutputPatch<HS_OUTPUT, OUTPUT_PATCH_SIZE> bezpatch,
                       float4 BasisU,
                       float4 BasisV )
{
    float3 Value = float3(0,0,0);
    Value  = BasisV.x * ( bezpatch[0].vPosition * BasisU.x + bezpatch[1].vPosition * BasisU.y + bezpatch[2].vPosition * BasisU.z + bezpatch[3].vPosition * BasisU.w );
    Value += BasisV.y * ( bezpatch[4].vPosition * BasisU.x + bezpatch[5].vPosition * BasisU.y + bezpatch[6].vPosition * BasisU.z + bezpatch[7].vPosition * BasisU.w );
    Value += BasisV.z * ( bezpatch[8].vPosition * BasisU.x + bezpatch[9].vPosition * BasisU.y + bezpatch[10].vPosition * BasisU.z + bezpatch[11].vPosition * BasisU.w );
    Value += BasisV.w * ( bezpatch[12].vPosition * BasisU.x + bezpatch[13].vPosition * BasisU.y + bezpatch[14].vPosition * BasisU.z + bezpatch[15].vPosition * BasisU.w );

    return Value;
}

// The domain shader is run once per vertex and calculates the final vertex's position
// and attributes.  It receives the UVW from the fixed function tessellator and the
// control point outputs from the hull shader.  Since we are using the DirectX 11
// Tessellation pipeline, it is the domain shader's responsibility to calculate the
// final SV_POSITION for each vertex.

// In this sample, we evaluate the vertex's
// position using a Bernstein polynomial and the normal is calculated as the cross
// product of the U and V derivatives.

// The input SV_DomainLocation to the domain shader comes from fixed function tessellator.
// The OutputPatch comes from the hull shader. 
// From these, you must calculate the final vertex position, color, texcoords, and other attributes.

// The output from the domain shader will be a vertex that will go to the video card's
// rasterization pipeline and get drawn to the screen.

[domain("quad")]
DS_OUTPUT BezierDS( HS_CONSTANT_DATA_OUTPUT input, 
                    float2 UV : SV_DomainLocation,
                    const OutputPatch<HS_OUTPUT, OUTPUT_PATCH_SIZE> bezpatch )
{
    float4 BasisU = BernsteinBasis( UV.x );
    float4 BasisV = BernsteinBasis( UV.y );
    float4 dBasisU = dBernsteinBasis( UV.x );
    float4 dBasisV = dBernsteinBasis( UV.y );

    float3 WorldPos  = EvaluateBezier( bezpatch, BasisU, BasisV );
    float3 Tangent   = EvaluateBezier( bezpatch, dBasisU, BasisV );
    float3 BiTangent = EvaluateBezier( bezpatch, BasisU, dBasisV );
    float3 Norm      = normalize( cross( Tangent, BiTangent ) );

    DS_OUTPUT Output;
    Output.vWorldPos = WorldPos;
    //float4 temp = mul(float4(WorldPos,1.0), g_mWorld);
    //Output.vWorldPos = temp.xyz;
    //Output.vNormal   = mul(Norm, (float3x3)g_mWorld);
    Output.vNormal   = Norm;
    Output.vtex = UV;
    Output.vPosition = mul( float4(WorldPos, 1.0), g_mViewProjection );
    //Output.vPosition = float4(WorldPos, 1);

    return Output;    
}

//--------------------------------------------------------------------------------------
// Lambert Shading Model 
//--------------------------------------------------------------------------------------
float4 lambert( DS_OUTPUT Input )
{
    float3 N = normalize(Input.vNormal);
    float3 L = normalize(Input.vWorldPos - g_vCameraPosWorld);
    float4 tex = txSkin.Sample(samLinear, Input.vtex);
    float3 ambient = float3(0.3, 0.3, 0.2);
    float3 intensity = float3(0.9, 0.9, 0.9);
    float3 lightDir = normalize(float3(-1, 1, -1));
    float3 color = abs(dot(N, lightDir)) * Ka * intensity + Ka * ambient;
    return float4(color, 1.0) + tex;
}

float4 phong( DS_OUTPUT Input )
{
    float3 N = normalize(Input.vNormal);
    float3 V = normalize(Input.vWorldPos - g_vCameraPosWorld);
    float3 L = normalize(float3(1, -1, 1));
	float3 R = normalize(L + 2 * dot(-L,N) * N);
    float3 Lintensity = float3(1.0, 0.8, 0.7);
    float3 Lambient   = float3(0.3, 0.3, 0.2);
    float4 tex = txSkin.Sample(samLinear, Input.vtex);

    float3 color = max(pow(dot(R,V),shininess),0.0) * Ks * Lintensity +  Kd * Lintensity * max(dot(N,L), 0.0) + Ka * Lambient;
    return float4(color, 1.0);
}

float4 blinnPhong(DS_OUTPUT Input)
{
	float3 N = normalize(Input.vNormal);
	float3 V = normalize(Input.vWorldPos - g_vCameraPosWorld);
	float3 L = normalize(float3(1, -1, 1));
	float3 H = normalize(-L + V);
	float3 Lintensity = float3(1.0, 0.8, 0.7);
	float3 Lambient = float3(0.3, 0.3, 0.2);
	float4 tex = txSkin.Sample(samLinear, Input.vtex);

	float3 color = max(pow(dot(N, H), shininess), 0.0) * Ks * Lintensity + Kd * Lintensity * max(dot(N, L), 0.0) + Ka * Lambient;
	return float4(color, 1.0) * tex;
}

float4 microFacet(DS_OUTPUT Input)
{
	float3 N = normalize(Input.vNormal);
	float3 V = normalize(Input.vWorldPos - g_vCameraPosWorld);
	float3 L = normalize(float3(0, 0, -1));
	float3 H = normalize(-L + V);
	float3 Lintensity = float3(1.0, 1.0, 1.0);
	float3 Lambient = float3(0.3, 0.3, 0.2);
	float3 f0 = float3(0.972, 0.960, 0.915);
	float3 frenel = f0 + (1 - f0) * pow((1 - dot(L, H)), 5);
	float d = 1.0 /( 4 * dot(N, L) * dot(N, V));
	float4 tex = txSkin.Sample(samLinear, Input.vtex);

	float3 color = abs(dot(N, H)) * frenel * d * Lintensity;
	return float4(color, 1.0);
}
//--------------------------------------------------------------------------------------
// Smooth shading pixel shader section
//--------------------------------------------------------------------------------------
float4 BezierPS( DS_OUTPUT Input ) : SV_TARGET
{
    //return lambert(Input);
    //return phong(Input);
    //return blinnPhong(Input);
	return microFacet(Input);

}

//--------------------------------------------------------------------------------------
// Solid color shading pixel shader (used for wireframe overlay)
//--------------------------------------------------------------------------------------
float4 SolidColorPS( DS_OUTPUT Input ) : SV_TARGET
{
    return float4( 0, 1, 0, 1 );
}
