//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------

cbuffer cbWireframe : register( b0 )
{
    float4 vMeshColor;
};

struct VSInput
{
    float4 Pos    : POSITION;
};

struct PSInput
{
    float4 Pos : SV_POSITION;
};

PSInput VS( VSInput input )
{
    PSInput output = (PSInput)0;
    output.Pos = input.Pos;
    return output;
}

float4 PS( PSInput input) : SV_Target
{
    return  vMeshColor;
}
