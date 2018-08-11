
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
    matrix g_mWorld; // column_major is default.
    matrix g_mViewProjection;
    float3 cbCameraPosWorld;
    float  cbTessellationFactor;
    int   cbWireframeOn;
    int   cbHeightMapOn;
    int   cbDiagType;
    float cbTexelCellU;
    float cbTexelCellV;
    float cbWorldCell;

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
VS_CONTROL_POINT_OUTPUT VSMain( VS_CONTROL_POINT_INPUT Input )
{
    VS_CONTROL_POINT_OUTPUT Output;
    Output.vPosition = mul(float4(Input.vPosition,1.0), g_mWorld).xyz;
    return Output;
}

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

    float TessAmount = cbTessellationFactor;
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
HS_OUTPUT HSMain( InputPatch<VS_CONTROL_POINT_OUTPUT, INPUT_PATCH_SIZE> p, 
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
    float3 vNormal          : NORMAL;
    float3 vTangent         : TANGENT;
    float3 vBiTangent       : BITANGENT;
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
DS_OUTPUT DSMain( HS_CONSTANT_DATA_OUTPUT input, 
                    float2 uv : SV_DomainLocation,
                    const OutputPatch<HS_OUTPUT, OUTPUT_PATCH_SIZE> quad)
{
    DS_OUTPUT Output;

    Output.vtex = uv;
    float3 WorldPos = lerp( lerp(quad[0].vPosition, quad[1].vPosition, uv.x), lerp(quad[2].vPosition, quad[3].vPosition, uv.x), uv.y);
    if (cbHeightMapOn)
    {
        WorldPos.y = heightMap.SampleLevel(samLinear, uv, 0).r;

        float2 leftTex   = uv + float2(-cbTexelCellU, 0.0f);
        float2 rightTex  = uv + float2( cbTexelCellU, 0.0f);
        float2 bottomTex = uv + float2(0.0f,  cbTexelCellV);
        float2 topTex    = uv + float2(0.0f, -cbTexelCellV);

        float leftY   = heightMap.SampleLevel(samLinear, leftTex,   0).r;
        float rightY  = heightMap.SampleLevel(samLinear, rightTex,  0).r;
        float bottomY = heightMap.SampleLevel(samLinear, bottomTex, 0).r;
        float topY    = heightMap.SampleLevel(samLinear, topTex,    0).r;

        Output.vTangent   = normalize(float3(2.0f * cbWorldCell, rightY - leftY, 0.0f));
        Output.vBiTangent = normalize(float3(0.0f, bottomY - topY, -2.0f * cbWorldCell));
        Output.vNormal    = cross(Output.vTangent, Output.vBiTangent);
    }
    else
    {
        Output.vNormal   = float3(0.f, 1.f, 0.f);
        Output.vTangent  = float3(1.f, 0.f, 0.f);
        Output.vBiTangent= float3(0.f, 0.f, 1.f);
    }
    Output.vWorldPos = WorldPos;
    Output.vPosition = mul( float4(Output.vWorldPos, 1.0), g_mViewProjection );

    return Output;    
}

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
float4 PSMain( DS_OUTPUT input ) : SV_TARGET
{
    if (cbWireframeOn)
        return float4( 0.f, 1.0f, 0.f, 1.0 );
    float v = dot(input.vNormal, cbCameraPosWorld - input.vWorldPos);
    return float4( v, v, v, 1.0 );
}

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
float4 DiagPSMain( DS_OUTPUT input ) : SV_TARGET
{
    float4 color;
    if      (cbDiagType == 0) color = float4(input.vNormal, 1.0f);
    else if (cbDiagType == 1) color = float4(input.vTangent, 1.0f);
    else if (cbDiagType == 2) color = float4(input.vBiTangent, 1.0f);
    else if (cbDiagType == 3) color = input.vPosition;
    else;

    return color;

}

struct GS_INPUT
{
    float4 gstemp1 : GSINPUT;
};

struct GS_OUTPUT
{
    float4 gstemp2;
};

/////////////////////////////////////////////////////////////////////////////
// Geometry 
/////////////////////////////////////////////////////////////////////////////
[maxvertexcount(6)]
void GSMain(point GS_INPUT inPoint[1], inout LineStream<GS_OUTPUT> lineStream)
{

}