//==========================================================//
struct ThreeDScanModelVSInput
{
    float3 Pos          : POSITION;         //position
    float3 Norm         : NORMAL;           //normal
};

struct ThreeDScanModelPSInput
{
    float4 Pos       : SV_POSITION;
    float3 WorldPos  : POSITION;
    float3 Normal    : TEXCOORD0;
};

// Vertex Shader
ThreeDScanModelPSInput ThreeDScanModelVS(ThreeDScanModelVSInput input)
{
    ThreeDScanModelPSInput output;
    float4 temp = mul(float4(input.Pos, 1.0f), World);
        output.WorldPos = temp.xyz;
    output.Pos = temp;
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Normal = mul(input.Norm, (float3x3)World);

    return output;
}


// Pixel Shader
float4 ThreeDScanModelPS(ThreeDScanModelPSInput Input) : SV_Target
{
    float3 N = normalize(Input.Normal);
    float3 V = normalize(CameraPosWorld - Input.WorldPos);
    float3 L = normalize(Lpos - Input.WorldPos);
    float3 R = normalize(2 * dot(L, N) * N - L);

    float3 color = max(pow(dot(R, V), Shininess), 0.0) * Kspecular * Lintensity + Kdiffuse * Lintensity * max(dot(N, L), 0.0f) + Kambient * Lambient;
    return float4(color, 1.0);
}

technique11 ThreeDScanModelRender
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_4_0, ThreeDScanModelVS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, ThreeDScanModelPS()));

        SetDepthStencilState(EnableDepth, 0);
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
    }
}