//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D txDiffuse    : register( t0 );
SamplerState samLinear : register( s0 );

cbuffer cbChangesEveryFrame : register( b0 )
{
    matrix World;
    matrix View;
    matrix Proj;
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


//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PSInput VS( VSInput input )
{
    PSInput output = (PSInput)0;
    //output.Pos = float4(input.Pos, 1.0);
    output.Pos = input.Pos;
   // output.Pos = mul( input.Pos, View);
   // output.Pos = mul( input.Pos, mInvProjectionView);
    //output.Pos = mul( float4(input.Pos, 1.0), mInvProjectionView);
   // output.Pos = mul( input.Pos, mInvProjectionView);
   // output.Tex = input.Tex;
    
    return output;
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS( PSInput input) : SV_Target
{
    return  vMeshColor;
}
