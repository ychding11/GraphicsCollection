
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

struct VS_INPUT
{
    float3 Pos          : POSITION;         //position
    float3 Norm         : NORMAL;           //normal
    float2 Tex          : TEXCOORD0;        //texture coordinate
};

struct PS_INPUT
{
    float4 Pos    : SV_POSITION;
    float3 Norm   : TEXCOORD0;
    float2 Tex    : TEXCOORD1;
    float3 ViewR  : TEXCOORD2;
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

PS_INPUT VS( VS_INPUT input )
{
    PS_INPUT output;
    output.Pos = mul( float4(input.Pos,1), World );
    output.Pos = mul( output.Pos, View );
    output.Pos = mul( output.Pos, Projection );
    output.Norm = mul( input.Norm, (float3x3)World );
    output.Tex = input.Tex;
    
    // Calculate the reflection vector
    float3 viewNorm = mul( output.Norm, (float3x3)View );
    output.ViewR = reflect( viewNorm, float3(0,0,-1.0) );
    return output;
}


float4 PS( PS_INPUT input) : SV_Target
{
    // Calculate lighting assuming light color is <1,1,1,1>
    float fLighting = saturate( dot( input.Norm, vLightDir ) );
   
    // Load the environment map texture
    float4 cReflect = g_txEnvMap.Sample( samLinearClamp, input.ViewR );
    
    // Load the diffuse texture and multiply by the lighting amount
    float4 cDiffuse = g_txDiffuse.Sample( samLinear, input.Tex ) * fLighting;
    
    // Add diffuse to reflection and go
    float4 cTotal = cDiffuse + cReflect;
    cTotal.a = 1;
    return cTotal;
}

technique11 EnvMapRender
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VS()));
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS()));

        SetDepthStencilState( EnableDepth, 0 );
        SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
}

//==========================================================//
struct PhongPS_INPUT
{
    float4 Pos       : SV_POSITION;
    float3 WorldPos  : POSITION;
    float3 Normal    : TEXCOORD0;
    float2 Tex       : TEXCOORD1;
};

// Vertex Shader
PhongPS_INPUT PhongVS(VS_INPUT input)
{
    PhongPS_INPUT output;
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
float4 PhongPS(PhongPS_INPUT Input) : SV_Target
{
    float3 N = normalize(Input.Normal);
    float3 V = normalize(CameraPosWorld - Input.WorldPos);
    float3 L = normalize(Lpos - Input.WorldPos);
    float3 R = normalize(2 * dot(L, N) * N - L);

    float3 color = max(pow(dot(R, V), Shininess), 0.0) * Kspecular * Lintensity + Kdiffuse * Lintensity * max(dot(N, L), 0.0f) + Kambient * Lambient;
    float3 lpos = float3(1.512, 1.123, 0.132);
    float3 ll = float3(0.125, 0.643, 0.6423);
    L = normalize(lpos - Input.WorldPos);
    R = normalize(2 * dot(L, N) * N - L);
    color += max(pow(dot(R, V), Shininess), 0.0) * Kspecular * ll + Kdiffuse * ll * max(dot(N, L), 0.0f);

    lpos = float3(-1.512, -1.1123, -1.312);
    ll   = float3(0.123, 0.6236, 0.623);
    L = normalize(lpos - Input.WorldPos);
    R = normalize(2 * dot(L, N) * N - L);
    color += max(pow(dot(R, V), Shininess), 0.0) * Kspecular * ll + Kdiffuse * ll * max(dot(N, L), 0.0f);

    lpos = float3(-1.512, 1.1123, -1.312);
    ll   = float3(0.123, 0.6236, 0.623);
    L = normalize(lpos - Input.WorldPos);
    R = normalize(2 * dot(L, N) * N - L);
    color += max(pow(dot(R, V), Shininess), 0.0) * Kspecular * ll + Kdiffuse * ll * max(dot(N, L), 0.0f);

    lpos = float3(1.512, -1.1123, -1.112);
    ll   = float3(0.123, 0.6236, 0.623);
    L = normalize(lpos - Input.WorldPos);
    R = normalize(2 * dot(L, N) * N - L);
    color += max(pow(dot(R, V), Shininess), 0.0) * Kspecular * ll + Kdiffuse * ll * max(dot(N, L), 0.0f);
    return float4(color, 1.0);
}

technique11 PhongRender
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0,PhongVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PhongPS() ) );

        SetDepthStencilState( EnableDepth, 0 );
        SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
}

//==========================================================//
struct SimpleVS_INPUT
{
     float3 Pos : POSITION;
};

struct SimplePS_INPUT
{
     float4 Pos : SV_POSITION;
};

SimplePS_INPUT SimpleVS( SimpleVS_INPUT input )
{
    SimplePS_INPUT output;
    output.Pos = mul( float4(input.Pos,1), World );
    output.Pos = mul( output.Pos, View );
    output.Pos = mul( output.Pos, Projection );
    return output;
}

float4 SimplePS( SimplePS_INPUT input) : SV_Target
{
	float4 cTotal = float4(1,0.9,0.6,1);
    return cTotal;
}

technique11 SimpleRender
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, SimpleVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, SimplePS() ) );

        SetDepthStencilState( EnableDepth, 0 );
        SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
}

struct SkyboxVS_Input
{
	float4 Pos : POSITION;
};

struct SkyboxVS_Output
{
	float4 Pos : SV_POSITION;
	float3 Tex : TEXCOORD0;
};

SkyboxVS_Output SkyboxVS(SkyboxVS_Input Input)
{
	SkyboxVS_Output Output;
	Output.Pos = Input.Pos ;
	if (SpinBackground)
	{
		Output.Pos = mul(Input.Pos, World);
	}
	Output.Tex = normalize(Output.Pos);
	Output.Pos = Input.Pos;

	return Output;
}

float4 SkyboxPS(SkyboxVS_Output Input) : SV_TARGET
{
	float4 color = g_txEnvMap.Sample(samLinear, Input.Tex);
	return color;
}

technique11 BackgroundRender
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, SkyboxVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, SkyboxPS() ) );
        
        SetDepthStencilState( NoEnableDepth, 0 );
        //SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
}

//==========================================================//
struct ThreeDScanModel_VS_INPUT
{
    float3 Pos          : POSITION;         //position
    float3 Norm         : NORMAL;           //normal
};

struct ThreeDScanModel_PS_INPUT
{
    float4 Pos       : SV_POSITION;
    float3 WorldPos  : POSITION;
    float3 Normal    : TEXCOORD0;
};

// Vertex Shader
ThreeDScanModel_PS_INPUT ThreeDScanModel_VS(ThreeDScanModel_VS_INPUT input)
{
    ThreeDScanModel_PS_INPUT output;
    float4 temp = mul(float4(input.Pos, 1.0f), World);
    output.WorldPos = temp.xyz;
    output.Pos = temp;
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Normal = mul(input.Norm, (float3x3)World);

    return output;
}


// Pixel Shader
float4 ThreeDScanModel_PS(ThreeDScanModel_PS_INPUT Input) : SV_Target
{
    float3 N = normalize(Input.Normal);
    float3 V = normalize(CameraPosWorld - Input.WorldPos);
    float3 L = normalize(Lpos - Input.WorldPos);
    float3 R = normalize(2 * dot(L, N) * N - L);

    float3 color = max(pow(dot(R, V), Shininess), 0.0) * Kspecular * Lintensity + Kdiffuse * Lintensity * max(dot(N, L), 0.0f) + Kambient * Lambient;
    float3 lpos = float3(1.512, 1.123, 0.132);
    float3 ll = float3(0.125, 0.643, 0.6423);
    L = normalize(lpos - Input.WorldPos);
    R = normalize(2 * dot(L, N) * N - L);
    color += max(pow(dot(R, V), Shininess), 0.0) * Kspecular * ll + Kdiffuse * ll * max(dot(N, L), 0.0f);

    lpos = float3(-1.512, -1.1123, -1.312);
    ll   = float3(0.123, 0.6236, 0.623);
    L = normalize(lpos - Input.WorldPos);
    R = normalize(2 * dot(L, N) * N - L);
    color += max(pow(dot(R, V), Shininess), 0.0) * Kspecular * ll + Kdiffuse * ll * max(dot(N, L), 0.0f);

    lpos = float3(-1.512, 1.1123, -1.312);
    ll   = float3(0.123, 0.6236, 0.623);
    L = normalize(lpos - Input.WorldPos);
    R = normalize(2 * dot(L, N) * N - L);
    color += max(pow(dot(R, V), Shininess), 0.0) * Kspecular * ll + Kdiffuse * ll * max(dot(N, L), 0.0f);

    lpos = float3(1.512, -1.1123, -1.112);
    ll   = float3(0.123, 0.6236, 0.623);
    L = normalize(lpos - Input.WorldPos);
    R = normalize(2 * dot(L, N) * N - L);
    color += max(pow(dot(R, V), Shininess), 0.0) * Kspecular * ll + Kdiffuse * ll * max(dot(N, L), 0.0f);
    return float4(color, 1.0);
}

technique11 ThreeDScanModelRender
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0,ThreeDScanModel_VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, ThreeDScanModel_PS() ) );

        SetDepthStencilState( EnableDepth, 0 );
        SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
}

//==========================================================//
