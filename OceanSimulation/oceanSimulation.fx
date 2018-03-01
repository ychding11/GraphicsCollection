//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D txDiffuse    : register( t0 );
SamplerState samLinear : register( s0 );

cbuffer cbChangesEveryFrame : register( b0 )
{
    matrix World;
    matrix mViewProj;  // Render camera
    float3 vEyePos;
    float4 vMeshColor;
};



//--------------------------------------------------------------------------------------
struct VSInput
{
    float4 Pos    : POSITION;
    float3 Normal : NORMAL;
    float2 Tex    : TEXCOORD;
};

struct PSInput
{
    float4 Pos       : SV_POSITION;
    float4 worldPos  : POSITION;
    float3 Normal    : NORMAL;
};

PSInput VS(VSInput input)
{
    PSInput output = (PSInput)0;
    output.Pos = input.Pos;
    output.worldPos = input.Pos;
    output.Normal = input.Normal;
    output.Pos = mul(output.Pos, mViewProj);
    return output;
}

float4 PS(PSInput input) : SV_Target
{
    float3 v = vEyePos - input.worldPos;
    return  vMeshColor * dot(v, input.Normal);
}