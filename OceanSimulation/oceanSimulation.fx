//--------------------------------------------------------------------------------------
// File: Tutorial11.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D g_txDiffuse;
SamplerState samLinear
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

cbuffer cbConstant
{
    float3 vLightDir = float3(-0.577,0.577,-0.577);
};

cbuffer cbChangesEveryFrame
{
    matrix World;
    matrix View;
    matrix Projection;
    float Time;
};

cbuffer cbUserChanges
{
    float Waviness;
};


//--------------------------------------------------------------------------------------
// DepthStates
//--------------------------------------------------------------------------------------
DepthStencilState EnableDepth
{
    DepthEnable = TRUE;
    DepthWriteMask = ALL;
    DepthFunc = LESS_EQUAL;
};

BlendState NoBlending
{
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = FALSE;
};


//--------------------------------------------------------------------------------------
// Input structure
//--------------------------------------------------------------------------------------
struct VSInput
{
    float3 Pos          : POSITION;        
    float3 Norm         : NORMAL;          
    float2 Tex          : TEXCOORD0;       
};

struct PSInput
{
    float4 Pos  : SV_POSITION;
    float3 Norm : TEXCOORD0;
    float2 Tex  : TEXCOORD1;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PSInput VS( VSInput input )
{
    PSInput output = (PSInput)0;
    
    output.Pos = float4(input.Pos, 1.0);
    //output.Pos    = mul( float4(input.Pos,1.0), World );
    //output.Pos.x += Waviness * sin( output.Pos.y * 0.1f + Time );
    //output.Pos = mul( output.Pos, View );
    //output.Pos = mul( output.Pos, Projection );

    output.Norm = mul( input.Norm, World );
    output.Tex = input.Tex;
    
    return output;
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS( PSInput input) : SV_Target
{
    // Calculate lighting assuming light color is <1,1,1,1>
    //float fLighting = saturate( dot( input.Norm, vLightDir ) );
    //float4 outputColor = g_txDiffuse.Sample( samLinear, input.Tex ) * fLighting;
    float4 outputColor = float4(0,1,0,1);
    return outputColor;
}


//--------------------------------------------------------------------------------------
// Technique
//--------------------------------------------------------------------------------------
technique11 Render
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS() ) );        

        SetDepthStencilState( EnableDepth, 0 );
        SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    }
}

