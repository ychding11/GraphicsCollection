
//==========================================================//
struct PhongVSInput
{
    float3 Pos       : POSITION;         //position
    float3 Norm      : NORMAL;           //normal
    float2 Tex       : TEXCOORD0;        //texture coordinate
};
struct PhongPSInput
{
    float4 Pos       : SV_POSITION;
    float3 WorldPos  : POSITION;
    float3 Normal    : TEXCOORD0;
    float2 Tex       : TEXCOORD1;
};

// Vertex Shader
PhongPSInput PhongVS(PhongVSInput input)
{
    PhongPSInput output;
    float4 temp = mul(float4(input.Pos, 1.0f), World);
        output.WorldPos = temp.xyz;
    output.Pos = temp;
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Normal = mul(input.Norm, (float3x3)World);
    output.Tex = input.Tex;

    return output;
}


// Pixel Shader
float4 PhongPS(PhongPSInput input) : SV_Target
{
    float3 N = normalize(input.Normal);
    float3 V = normalize(CameraPosWorld - input.WorldPos);
    float3 L = normalize(Lpos - input.WorldPos);
    float3 R = normalize(2 * dot(L, N) * N - L);

    float3 color = max(pow(dot(R, V), Shininess), 0.0) * Kspecular * Lintensity + Kdiffuse * Lintensity * max(dot(N, L), 0.0f) + Kambient * Lambient;
    float3 lpos = float3(1.512, 1.123, 0.132);
    float3 ll = float3(0.125, 0.643, 0.6423);
    L = normalize(lpos - input.WorldPos);
    R = normalize(2 * dot(L, N) * N - L);
    color += max(pow(dot(R, V), Shininess), 0.0) * Kspecular * ll + Kdiffuse * ll * max(dot(N, L), 0.0f);

    lpos = float3(-1.512, -1.1123, -1.312);
    ll = float3(0.123, 0.6236, 0.623);
    L = normalize(lpos - input.WorldPos);
    R = normalize(2 * dot(L, N) * N - L);
    color += max(pow(dot(R, V), Shininess), 0.0) * Kspecular * ll + Kdiffuse * ll * max(dot(N, L), 0.0f);

    lpos = float3(-1.512, 1.1123, -1.312);
    ll = float3(0.123, 0.6236, 0.623);
    L = normalize(lpos - input.WorldPos);
    R = normalize(2 * dot(L, N) * N - L);
    color += max(pow(dot(R, V), Shininess), 0.0) * Kspecular * ll + Kdiffuse * ll * max(dot(N, L), 0.0f);

    lpos = float3(1.512, -1.1123, -1.112);
    ll = float3(0.123, 0.6236, 0.623);
    L = normalize(lpos - input.WorldPos);
    R = normalize(2 * dot(L, N) * N - L);
    color += max(pow(dot(R, V), Shininess), 0.0) * Kspecular * ll + Kdiffuse * ll * max(dot(N, L), 0.0f);
    return float4(color, 1.0);
}

technique11 PhongRender
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_4_0, PhongVS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, PhongPS()));

        SetDepthStencilState(EnableDepth, 0);
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
    }
}
