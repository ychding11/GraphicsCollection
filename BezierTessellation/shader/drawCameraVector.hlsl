
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

struct VS_INPUT
{
    float3 vPosition        : POSITION;
};

struct VS_OUTPUT
{
    float3 vPosition        : POSITION;
};

struct HS_OUTPUT
{
};

struct DS_OUTPUT
{
};

struct GS_OUTPUT
{
    float3 vPosition    : POSITION;
    float4 vColor       : COLOR;
};

/////////////////////////////////////////////////////////////////////////////
// Vertex 
/////////////////////////////////////////////////////////////////////////////
VS_OUTPUT VSMain( VS_INPUT Input )
{
    VS_OUTPUT output;
    output.vPosition = mul(float4(Input.vPosition,1.0), g_mWorld).xyz;
    return output;
}

/////////////////////////////////////////////////////////////////////////////
// Tessellation
/////////////////////////////////////////////////////////////////////////////
struct HS_CONSTANT_DATA_OUTPUT
{
    float Edges[4]    : SV_TessFactor;
    float Inside[2]   : SV_InsideTessFactor;
};

HS_CONSTANT_DATA_OUTPUT ConstantHS(InputPatch<VS_OUTPUT, INPUT_PATCH_SIZE> ip, uint PatchID : SV_PrimitiveID)
{
    HS_CONSTANT_DATA_OUTPUT output;

    float TessAmount = cbTessellationFactor;
    output.Edges[0]  = output.Edges[1] = output.Edges[2] = output.Edges[3] = TessAmount;
    output.Inside[0] = output.Inside[1] = TessAmount;

    return output;
}

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(OUTPUT_PATCH_SIZE)]
[patchconstantfunc("ConstantHS")]
HS_OUTPUT HSMain(InputPatch<VS_OUTPUT, INPUT_PATCH_SIZE> p, uint i : SV_OutputControlPointID, uint PatchID : SV_PrimitiveID)
{
    HS_OUTPUT output;
    return output;
}

[domain("quad")]
DS_OUTPUT DSMain(HS_CONSTANT_DATA_OUTPUT input, float2 uv : SV_DomainLocation, const OutputPatch<HS_OUTPUT, OUTPUT_PATCH_SIZE> quad)
{
    DS_OUTPUT output;
    return output;
}

/////////////////////////////////////////////////////////////////////////////
// Geometry 
/////////////////////////////////////////////////////////////////////////////
GS_OUTPUT GSMain( VS_OUTPUT input )
{
    GS_OUTPUT output;    
    output.vColor = float4(1.f, 0.f, 0.f, 1.f);
    return output;
}

/////////////////////////////////////////////////////////////////////////////
// Pixel 
/////////////////////////////////////////////////////////////////////////////
float4 PSMain( GS_OUTPUT input ) : SV_TARGET
{
    return input.vColor;
}

void DiagPSMain()
{

}