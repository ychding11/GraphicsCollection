//==========================================================//
struct SimpleVSInput
{
    float3 Pos : POSITION;
};

struct SimplePSInput
{
    float4 Pos : SV_POSITION;
};

SimplePSInput SimpleVS(SimpleVSInput input)
{
    SimplePSInput output;
    output.Pos = mul(float4(input.Pos, 1), World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    return output;
}

float4 SimplePS(SimplePSInput input) : SV_Target
{
    return  float4(0.5, 0.5, 0.6, 1);
}

technique11 SimpleRender
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, SimpleVS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, SimplePS()));

        SetDepthStencilState(EnableDepth, 0);
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
    }
}