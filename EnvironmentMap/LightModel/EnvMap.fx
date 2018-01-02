
Texture2D g_txDiffuse;
TextureCube g_txEnvMap;

SamplerState samLinear
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

SamplerState samLinearClamp
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};

#define LIGHTS_NUM 10
struct LightSource
{
    float3 intensity;
    float3 position;
};

cbuffer cbConstant
{
    float3 vLightDir = float3(-0.577, 0.577, -0.577);
    float3 Lambient = float3(0.1, 0.1, 0.1);
    float3 Lintensity = float3(0.125, 0.643, 0.6423);
    float3 Lpos = float3(0.012, 1.123, 1.132);
    float3 Kdiffuse = float3(0.55231, 0.232, 0.5612);
    float3 Kspecular = float3(0.262344, 0.623421, 0.1233);
    float3 Kambient = float3(0.072, 0.060, 0.015);
    LightSource lights[LIGHTS_NUM];
    int lightSize;
};

cbuffer cbChangesEveryFrame
{
    matrix World;
    matrix View;
    matrix Projection;
    float3 CameraPosWorld;
    float Shininess;
	bool SpinBackground;
};

cbuffer cbUserChanges
{
    float Waviness;
};



DepthStencilState EnableDepth
{
    DepthEnable = TRUE;
    DepthWriteMask = ALL;
    DepthFunc = LESS_EQUAL;
};

DepthStencilState NoEnableDepth
{
    DepthEnable = FALSE;
    DepthWriteMask = ALL;
    DepthFunc = LESS;
};

BlendState NoBlending
{
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = FALSE;
};

#include "PhongRender.fx"
#include "simpleRender.fx"
#include "BackgroundRender.fx"
#include "3DScanModelRender.fx"

struct EnvMapVSInput
{
    float3 Pos          : POSITION;         //position
    float3 Norm         : NORMAL;           //normal
    float2 Tex          : TEXCOORD0;        //texture coordinate
};

struct EnvMapPSInput
{
    float4 Pos    : SV_POSITION;
    float3 Norm   : TEXCOORD0;
    float2 Tex    : TEXCOORD1;
    float3 ViewR  : TEXCOORD2;
};

EnvMapPSInput EnvMapVS(EnvMapVSInput input)
{
    EnvMapPSInput output;
    output.Pos = mul(float4(input.Pos, 1), World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);

    output.Norm = mul(input.Norm, (float3x3)World);
    output.Tex = input.Tex;

    // Calculate the reflection vector
    float3 viewNorm = mul(output.Norm, (float3x3)View);
    output.ViewR    = reflect(viewNorm, float3(0, 0, -1.0));
    return output;
}

float4 EnvMapPS(EnvMapPSInput input) : SV_Target
{
    // Calculate lighting assuming light color is <1,1,1,1>
    float fLighting = saturate(dot(input.Norm, vLightDir));

    // Load the environment map texture
    float4 cReflect = g_txEnvMap.Sample(samLinearClamp, input.ViewR);
    // Load the diffuse texture and multiply by the lighting amount
    float4 cDiffuse = g_txDiffuse.Sample(samLinear, input.Tex) * fLighting;
    // Add diffuse to reflection and go
    float4 cTotal = cDiffuse + cReflect;
    cTotal.a = 1;
    return cTotal;
}

technique11 EnvMapRender
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_4_0, EnvMapVS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, EnvMapPS()));

        SetDepthStencilState(EnableDepth, 0);
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
    }
}
