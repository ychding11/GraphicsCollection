
cbuffer cbDrawFrustum : register( b0 )
{
    matrix mInvViewProj;  // Rendering camera
    matrix mViewProj;     // Observe camera
    float4 vMeshColor;
};

struct VSInput
{ float4 Pos    : POSITION; };
struct PSInput
{ float4 Pos : SV_POSITION; };

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
float4 DrawFustumVS( float4 pos : POSITION ) : SV_POSITION
{
    float4 ret = mul(pos, mInvViewProj);
    ret = mul(ret, mViewProj);
    return ret;
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 DrawFustumPS( float4 pos : SV_POSITION  ) : SV_Target
{
    return  vMeshColor;
}
