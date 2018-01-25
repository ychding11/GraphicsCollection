//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D txDiffuse    : register( t0 );
SamplerState samLinear : register( s0 );

cbuffer cbChangesEveryFrame : register( b0 )
{
    matrix World;
    matrix mViewProj;  // Render camera
    float4 vMeshColor;
};



//--------------------------------------------------------------------------------------
struct VSInput
{
    float4 Pos    : POSITION;
    //float3 Normal : NORMAL;
    //float2 Tex    : TEXCOORD;
};

struct PSInput
{
    float4 Pos : SV_POSITION;
    //float2 Tex : TEXCOORD0;
};

PSInput VS(VSInput input)
{
    PSInput output = (PSInput)0;
    output.Pos = input.Pos;
    output.Pos = mul(output.Pos, mViewProj);
    return output;
}

float4 PS(PSInput input) : SV_Target
{
    return  vMeshColor;
}