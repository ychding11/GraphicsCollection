//==========================================================//
struct BackgroundVSInput
{
    float4 Pos : POSITION;
};

struct BackgroundPSInput
{
    float4 Pos : SV_POSITION;
    float3 Tex : TEXCOORD0;
};

BackgroundPSInput BackgroundVS(BackgroundVSInput input)
{
    BackgroundPSInput output;
    output.Pos = input.Pos;
    if (SpinBackground)
    {
        output.Pos = mul(input.Pos, World);
    }
    output.Tex = normalize(output.Pos);
    output.Pos = input.Pos;

    return output;
}

float4 BackgroundPS(BackgroundPSInput input) : SV_TARGET
{
    float4 color = g_txEnvMap.Sample(samLinear, input.Tex);
    return color;
}

technique11 BackgroundRender
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_4_0, BackgroundVS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, BackgroundPS()));

        SetDepthStencilState(NoEnableDepth, 0);
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
    }
}