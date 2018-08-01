
// See the hull shader: [partitioning(BEZIER_HS_PARTITION)]
// This sample demonstrates "integer", "fractional_even", and "fractional_odd"
#ifndef BEZIER_HS_PARTITION
#define BEZIER_HS_PARTITION "integer"
#endif

// The patch size.  In this sample, it is 16 control points.
// This value should match the call to IASetPrimitiveTopology()
#define INPUT_PATCH_SIZE  4
#define OUTPUT_PATCH_SIZE 4

Texture2D    heightMap   : register(t0);
SamplerState samLinear   : register(s0);

//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------
cbuffer cbPerFrame : register( b0 )
{
    column_major matrix g_mWorld; // column_major is default.
    matrix g_mViewProjection;
    float3 g_vCameraPosWorld;
    float  g_fTessellationFactor;
    bool   cbWireframeOn;
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

// This constant hull shader is executed once per patch. SV_TessFactor and SV_InsideTessFactor for each patch.
// You might calculate a variable tessellation factor based on the camera's distance.

HS_CONSTANT_DATA_OUTPUT BezierConstantHS( InputPatch<VS_CONTROL_POINT_OUTPUT, INPUT_PATCH_SIZE> ip,
                                          uint PatchID : SV_PrimitiveID )
{    
    HS_CONSTANT_DATA_OUTPUT Output;

    float TessAmount = g_fTessellationFactor;
    Output.Edges[0]  = Output.Edges[1] = Output.Edges[2] = Output.Edges[3] = TessAmount;
    Output.Inside[0] = Output.Inside[1] = TessAmount;
    return Output;
}

// The hull shader is called once per output control point.
// For this sample, we take the control points from the vertex
// shader and pass them directly off to the domain shader.  In a more complex scene,
// you might perform a basis conversion from the input control points into a Bezier
// patch, such as the SubD11 Sample.

// The input to the hull shader comes from the vertex shader

// The output from the hull shader will go to the domain shader.
// The tessellation factor, topology, and partition mode will go to HW Tessellator. 
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

struct DS_OUTPUT
{
    float4 vPosition        : SV_POSITION;
    float3 vWorldPos        : WORLDPOS;
//    float3 vNormal          : NORMAL;
    float2 vtex             : TEX;
};

// The domain shader is run once per vertex and calculates the final vertex's position  and attributes.
// It receives the UVW from the fixed function tessellator and the control point outputs from the hull shader.
// Since we are using the DirectX 11 Tessellation pipeline, it is the domain shader's responsibility to calculate the
// final SV_POSITION for each vertex.

// The input SV_DomainLocation to the domain shader comes from fixed function tessellator.
// The OutputPatch comes from the hull shader. 
// DS calculates the final vertex position, color, texcoords, and other attributes.
[domain("quad")]
DS_OUTPUT BezierDS( HS_CONSTANT_DATA_OUTPUT input, 
                    float2 uv : SV_DomainLocation,
                    const OutputPatch<HS_OUTPUT, OUTPUT_PATCH_SIZE> quad)
{
    DS_OUTPUT Output;
    float3 WorldPos = lerp( lerp(quad[0].vPosition, quad[1].vPosition, uv.x), lerp(quad[2].vPosition, quad[3].vPosition, uv.x), uv.y);
    WorldPos.y = heightMap.SampleLevel(samLinear, uv, 0).r;
    Output.vWorldPos = WorldPos;
    Output.vtex = uv;
    Output.vPosition = mul( float4(WorldPos, 1.0), g_mViewProjection );

    return Output;    
}

//--------------------------------------------------------------------------------------
// Solid color shading pixel shader (used for wireframe overlay)
//--------------------------------------------------------------------------------------
float4 WireframePS( DS_OUTPUT Input ) : SV_TARGET
{
    return float4( 0.f, 1.0f, 0.f, 1.0 );
}

//--------------------------------------------------------------------------------------
// Solid color shading pixel shader (used for wireframe overlay)
//--------------------------------------------------------------------------------------
float4 SolidColorPS( DS_OUTPUT Input ) : SV_TARGET
{
        return float4( 0.3, 0.4, 0.4, 1.0 );
}
