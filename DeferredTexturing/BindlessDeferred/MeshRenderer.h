//=================================================================================================
//
//  Bindless Deferred Texturing Sample
//  by MJP
//  http://mynameismjp.wordpress.com/
//
//  All code and content licensed under the MIT license
//
//=================================================================================================

#pragma once

#include <PCH.h>

#include <Graphics/Model.h>
#include <Graphics/GraphicsTypes.h>
#include <Graphics/Camera.h>
#include <Graphics/ShaderCompilation.h>
#include <Graphics/ShadowHelper.h>
#include <Graphics/SH.h>

#include "AppSettings.h"
#include "SharedTypes.h"

using namespace SampleFramework12;

namespace SampleFramework12
{
    struct SkyCache;
}

struct MainPassData
{
    const SkyCache* SkyCache = nullptr;
    const Texture* DecalTextures = nullptr;
    const StructuredBuffer* DecalBuffer = nullptr;
    Decal CursorDecal;
    float CursorDecalIntensity = 0.0f;
    const RawBuffer* DecalClusterBuffer = nullptr;
    const ConstantBuffer* SpotLightBuffer = nullptr;
    const RawBuffer* SpotLightClusterBuffer = nullptr;
};

struct ShadingConstants
{
    Float4Align Float3 SunDirectionWS;
    float CosSunAngularRadius = 0.0f;
    Float4Align Float3 SunIrradiance;
    float SinSunAngularRadius = 0.0f;
    Float4Align Float3 CameraPosWS;

    Float4Align Float3 CursorDecalPos;
    float CursorDecalIntensity = 0.0f;
    Float4Align Quaternion CursorDecalOrientation;
    Float4Align Float3 CursorDecalSize;
    uint32 CursorDecalTexIdx = uint32(-1);
    uint32 NumXTiles = 0;
    uint32 NumXYTiles = 0;
    float NearClip = 0.0f;
    float FarClip = 0.0f;

    Float4Align ShaderSH9Color SkySH;
};

class MeshRenderer
{


public:

    MeshRenderer();

    void Initialize(const Model* sceneModel);
    void Shutdown();

    void CreatePSOs(DXGI_FORMAT mainRTFormat, DXGI_FORMAT depthFormat, const DXGI_FORMAT* gBufferFormats,
                    uint64 numGBuffers, uint32 numMSAASamples);
    void DestroyPSOs();

    void RenderMainPass(ID3D12GraphicsCommandList* cmdList, const Camera& camera, const MainPassData& mainPassData);
    void RenderGBuffer(ID3D12GraphicsCommandList* cmdList, const Camera& camera);

    void RenderDepthPrepass(ID3D12GraphicsCommandList* cmdList, const Camera& camera);
    void RenderSunShadowDepth(ID3D12GraphicsCommandList* cmdList, const OrthographicCamera& camera);
    void RenderSpotLightShadowDepth(ID3D12GraphicsCommandList* cmdList, const Camera& camera);

    void RenderSunShadowMap(ID3D12GraphicsCommandList* cmdList, const Camera& camera);
    void RenderSunShadowMapByBatch(ID3D12GraphicsCommandList* cmdList, const Camera& camera);

    void RenderSpotLightShadowMap(ID3D12GraphicsCommandList* cmdList, const Camera& camera);
    void RenderSpotLightShadowMapByBatch(ID3D12GraphicsCommandList* cmdList, const Camera& camera);

    const DepthBuffer& SunShadowMap() const { return sunShadowMap; }
    const DepthBuffer& SpotLightShadowMap() const { return spotLightShadowMap; }
    const Float4x4* SpotLightShadowMatrices() const { return spotLightShadowMatrices; }
    const StructuredBuffer& MaterialTextureIndicesBuffer() const { return materialTextureIndices; }
    const SunShadowConstantsDepthMap& SunShadowConstantData() { return sunShadowConstants; }

protected:

    void LoadShaders();
    void RenderDepth(ID3D12GraphicsCommandList* cmdList, const Camera& camera, ID3D12PipelineState* pso, uint64 numVisible);

    void BatchIndexBufferForAllLights(void);
    void BatchIndexForLight(uint64 idxLight, uint64 numVisible);

    void BatchIndexBufferForAllCascades(void);
    void BatchIndexForCacade(uint64 cascadeIndex, uint64 numVisible);

    const Model* model = nullptr;

    DepthBuffer sunShadowMap;
    DepthBuffer spotLightShadowMap;
    Float4x4 spotLightShadowMatrices[AppSettings::MaxSpotLights];

    StructuredBuffer materialTextureIndices;

    CompiledShaderPtr meshVS;
    CompiledShaderPtr meshPSForward;
    CompiledShaderPtr meshPSGBuffer[2];
    ID3D12PipelineState* mainPassPSO = nullptr;
    ID3D12RootSignature* mainPassRootSignature = nullptr;
    ID3D12PipelineState* gBufferPSO = nullptr;
    ID3D12RootSignature* gBufferRootSignature = nullptr;

    CompiledShaderPtr meshDepthVS;
    ID3D12PipelineState* depthPSO = nullptr;
    ID3D12PipelineState* sunShadowPSO = nullptr;
    ID3D12PipelineState* spotLightShadowPSO = nullptr;
    ID3D12RootSignature* depthRootSignature = nullptr;

    Array<DirectX::BoundingBox> meshBoundingBoxes;
    Array<uint32> meshDrawIndices;
    Array<float> meshZDepths;

    //// Index Buffer batch for all spot lights
	Array<uint32>   batchedIndicesForSpotLights;
    Array<uint32>   batchedOffsetPerSpotLight;
	Array<uint32>   batchedIndexPerSpotLight;
	FormattedBuffer batchedIndexBufferForSpotLights;
	uint32          batchedIndexCountForSpotLights = 0;
    uint32          numIndexInModel = 0;

    //// Index Buffer batch for all sun cascades 
	Array<uint32>   batchedIndicesForSun;
    Array<uint32>   batchedOffsetPerCascade;
	Array<uint32>   batchedIndexPerCascade;
	FormattedBuffer batchedIndexBufferForSun;
	uint32          batchedIndexCountForCascades = 0;

    SunShadowConstantsDepthMap sunShadowConstants;
    OrthographicCamera cascadeCameras[NumCascades];
};