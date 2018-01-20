#include "OceanSurface.h"


#include "SDKmisc.h"
//#include "DDSTextureLoader.h"

bool OceanSurface::IntersectionTest(const Camera &renderCamera)
{
    XMFLOAT3 frustum[8] = {
            XMFLOAT3(-1, -1, 0),                                   
            XMFLOAT3(+1, -1, 0),                                   
            XMFLOAT3(-1, +1, 0),                                   
            XMFLOAT3(+1, +1, 0),                                   
            XMFLOAT3(-1, -1, +1),                                   
            XMFLOAT3(+1, -1, +1),                                   
            XMFLOAT3(-1, +1, +1),                                   
            XMFLOAT3(+1, +1, +1)                                   
    };
    int cube[] = {
    0, 1, 0, 2, 2, 3, 1, 3,
    0, 4, 2, 6, 3, 7, 1, 5,
    4, 6, 4, 5, 5, 7, 6, 7 };

    XMVECTOR worldFrustum[8];
    XMMATRIX invViewprojMat = renderCamera.GetInvViewProjMatrix();

    for (int i = 0; i < 8; ++i)
    {
        // divide w already
        worldFrustum[i] = XMVector3TransformCoord(XMLoadFloat3(&frustum[i]), invViewprojMat);
    }

    mIntersectionPoints.clear();
    for (int i = 0; i < 12; ++i)
    {
        int src = cube[i * 2], dst = cube[i * 2 + 1];
		float distUpper1 = XMPlaneDotCoord(mUpper, worldFrustum[src]).m128_f32[0];
		float distUpper2 = XMPlaneDotCoord(mUpper, worldFrustum[dst]).m128_f32[0];
		float distLow1   = XMPlaneDotCoord(mLow, worldFrustum[src]).m128_f32[0];
		float distLow2   = XMPlaneDotCoord(mLow, worldFrustum[dst]).m128_f32[0];

        if (distUpper1 * distUpper2 < 0.0f)
        {
            mIntersectionPoints.push_back( XMPlaneIntersectLine(mUpper, worldFrustum[src], worldFrustum[dst]) );
        }
        if (distLow1 * distLow2 < 0.0f)
        {
            mIntersectionPoints.push_back( XMPlaneIntersectLine(mLow, worldFrustum[src], worldFrustum[dst]) );
        }
    }
    for (int i = 0; i < 8; i++)
    {
		float distUpper = XMPlaneDotCoord(mUpper, worldFrustum[i]).m128_f32[0];
		float distLow   = XMPlaneDotCoord(mLow, worldFrustum[i]).m128_f32[0];
        if (( distUpper < 0) && ( distLow > 0))
        {
            mIntersectionPoints.push_back(worldFrustum[i]);
        }
    }
    return mIntersectionPoints.size() > 0;
}

void OceanSurface::GetSurfaceRange(const Camera &renderCamera)
{
    XMVECTOR eyePos =  XMLoadFloat3( &renderCamera.GetEyePt());
    XMVECTOR viewDir = XMVector3Normalize( XMLoadFloat3( &renderCamera.GetViewDir()) );
    XMVECTOR projectorPos = eyePos;
    XMVECTOR aimPoint1, aimPoint2;
    float eyeHeiht2Low  = XMPlaneDotCoord(mLow, eyePos).m128_f32[0];
    float eyeHeiht2Base = XMPlaneDotCoord(mBase, eyePos).m128_f32[0];
    float cosTheta      = XMPlaneDotNormal(mBase, viewDir).m128_f32[0];

    assert( projectorPos.m128_f32[3] == 0.0);
    assert( viewDir.m128_f32[3] == 0.0);

    if (eyeHeiht2Low < 8 + 7)
    {
        if (eyeHeiht2Low < 0.0f)
        {
            projectorPos += mNormal * (8 + 7 - 2.0 * eyeHeiht2Low);
        }
        else
        {
            projectorPos += mNormal * (8 + 7 - eyeHeiht2Low);
        }
    }

    if ( cosTheta < 0.0f != eyeHeiht2Base < 0.0f)
    {
        aimPoint1 = XMPlaneIntersectLine(mBase, eyePos, eyePos + viewDir *  1000);
    }
    else
    {
        XMVECTOR mirror = viewDir - 2.0f * cosTheta * mNormal;
        aimPoint1 = XMPlaneIntersectLine(mBase, eyePos, eyePos + mirror *  1000);
    }

    aimPoint2 = eyePos + viewDir * 10.0f;
    aimPoint2 = aimPoint2 - mNormal * XMPlaneDotCoord(mBase, aimPoint2).m128_f32[0];
    XMVECTOR aimPoint = cosTheta * aimPoint1 + (1.0 - cosTheta) * aimPoint2;

    XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.f };
    XMMATRIX viewMat = XMMatrixLookAtLH(projectorPos, aimPoint, up);
    XMMATRIX projMat = renderCamera.GetProjMatrix();
    XMMATRIX viewprojMat = viewMat * projMat;

	float minX = 1.0f, maxX = 0.0f, minY = 1.0f, maxY = 0.0f;
	// projector space
    int nPoints = mIntersectionPoints.size();
    for (int i = 0; i < nPoints; ++i)
    {
        XMVECTOR point = mIntersectionPoints[i];
		float height2Base = XMPlaneDotCoord(mBase, point).m128_f32[0];
        point = point - mNormal * height2Base; 
        point = XMVector3TransformCoord(point, viewprojMat);
		float x = point.m128_f32[0];
		float y = point.m128_f32[1];
		if (x < minX) minX = x;
		if (x > maxX) maxX = x;
		if (y < minY) minY = y;
		if (y > maxY) maxY = y;
    }

    float xrange = maxX - minX;
    float yrange = maxY - minY;
   // if (xrange >= 2.0) xrange = 1.9999;
   // if (yrange >= 2.0) yrange = 1.9999;
    XMMATRIX pack( xrange, 0,      0, minX,
                   0,      yrange, 0, minY,
                   0,      0,      1, 0,
                   0,      0,      0, 1);
    pack = XMMatrixTranspose(pack);


	XMFLOAT2 gridConer[4] = {
		XMFLOAT2(0, 0),
		XMFLOAT2(1, 0),
		XMFLOAT2(0, 1),
		XMFLOAT2(1, 1),
	};

	XMMATRIX invViewprojMat = pack * XMMatrixInverse(nullptr, viewprojMat);
    mGridConer[0] = getWorldGridConer(gridConer[0], invViewprojMat );
    mGridConer[1] = getWorldGridConer(gridConer[1], invViewprojMat );
    mGridConer[2] = getWorldGridConer(gridConer[2], invViewprojMat );
    mGridConer[3] = getWorldGridConer(gridConer[3], invViewprojMat );

}

void OceanSurface::TessellateSurfaceMesh(const Camera &renderCamera)
{
    int sizeX = iparameters["xsize"];//mXSize;
    int sizeY = iparameters["ysize"]; //mYSize;
    float u = 0.0f, v = 0.0f;
    float du = 1.0 / float(sizeX - 1), dv = 1.0 / float(sizeY - 1);
    XMMATRIX viewprojMat = renderCamera.GetViewProjMatrix();
    
#if 0
    assert( mGridConer[0].m128_f32[1] == 0.0 && mGridConer[0].m128_f32[3] == 1.0 );
    assert( mGridConer[1].m128_f32[1] == 0.0 && mGridConer[1].m128_f32[3] == 1.0 );
    assert( mGridConer[2].m128_f32[1] == 0.0 && mGridConer[2].m128_f32[3] == 1.0 );
    assert( mGridConer[0].m128_f32[1] == 0.0 && mGridConer[3].m128_f32[3] == 1.0 );
#endif

    mSurfaceVertex.clear();
    for (int i = 0; i < sizeY; ++i)
    {
        u = 0.0f;
        for (int j = 0; j < sizeX; ++j)
        {
            XMVECTOR xx = (1.0f - v) * ( (1.0f - u) * mGridConer[0] + u * mGridConer[1]) + v * ( (1.0 - u) * mGridConer[2] + u * mGridConer[3] );
            xx.m128_f32[1] = fparameters["max_amplitude"] * noise.GetNoiseValue(i, j);
            XMFLOAT4 temp;
            XMStoreFloat4(&temp, XMVector4Transform(xx, viewprojMat ));
            //temp.y = noise.ValueNoise_2D(temp.x, temp.z);
            SurfaceVertex vertex = {temp.x, temp.y, temp.z, temp.w };
            mSurfaceVertex.push_back(vertex);
            u += du;
        }
        v += dv;
    }

    mSurfaceIndex.clear();
    for (int i = 0; i < sizeY - 1; ++i)
    {
        for (int j = 0; j < sizeX - 1; ++j)
        {
            int a = sizeX * i + j;
            int b = sizeX * (i + 1) + j;
            int c = b + 1;
            int d = a + 1;
            
            mSurfaceIndex.push_back(a);
            mSurfaceIndex.push_back(b);
            mSurfaceIndex.push_back(c);

            mSurfaceIndex.push_back(c);
            mSurfaceIndex.push_back(d);
            mSurfaceIndex.push_back(a);
        }
    }
}

XMVECTOR OceanSurface::getWorldGridConer(XMFLOAT2 coner,  const XMMATRIX &invViewprojMat)
{
    XMVECTORF32 point1 = {coner.x, coner.y, 0.0, 1.0};
    XMVECTORF32 point2 = {coner.x, coner.y, 1.0, 1.0};
    XMVECTOR start = XMVector3TransformCoord(point1, invViewprojMat);
    XMVECTOR end   = XMVector3TransformCoord(point2, invViewprojMat);
    XMVECTOR ret = XMPlaneIntersectLine(mBase, start, end);
    return ret;
}

void OceanSurface::UpdateParameters(ID3D11DeviceContext* pd3dImmediateContext, const Camera &renderCamera)
{
    HRESULT hr;
    XMMATRIX mView = renderCamera.GetViewMatrix();
    XMMATRIX mProjection = renderCamera.GetProjMatrix();

    // Update constant buffer that changes once per frame
    D3D11_MAPPED_SUBRESOURCE MappedResource;
    V(pd3dImmediateContext->Map(mpCBChangesEveryFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
    CBChangesEveryFrame* pCB = reinterpret_cast<CBChangesEveryFrame*>(MappedResource.pData);
    XMStoreFloat4x4(&pCB->mWorld, XMMatrixTranspose(mmWorld));
    XMStoreFloat4x4(&pCB->mView, XMMatrixTranspose(mView));
    XMStoreFloat4x4(&pCB->mProj, XMMatrixTranspose(mProjection));
    pCB->vMeshColor = mvMeshColor;
    pd3dImmediateContext->Unmap(mpCBChangesEveryFrame, 0);

    V(pd3dImmediateContext->Map(mpCBWireframe, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
    CBWireframe* pCBWireframe = reinterpret_cast<CBWireframe*>(MappedResource.pData);
    pCBWireframe->vMeshColor = mvMeshColor;
    pd3dImmediateContext->Unmap(mpCBWireframe, 0);
}

HRESULT OceanSurface::CreateAndUpdateSurfaceMeshBuffer(ID3D11Device* pd3dDevice )
{
    HRESULT  hr;
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.ByteWidth = mSurfaceVertex.size() * sizeof(SurfaceVertex);

    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = &(mSurfaceVertex[0]);
    SAFE_RELEASE(mpVertexBuffer);
    V_RETURN(pd3dDevice->CreateBuffer(&bd, &InitData, &mpVertexBuffer));

    bd.ByteWidth = mSurfaceIndex.size() * sizeof(int);
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = &(mSurfaceIndex[0]);
    SAFE_RELEASE(mpIndexBuffer);
    V_RETURN(pd3dDevice->CreateBuffer(&bd, &InitData, &mpIndexBuffer));

    return S_OK;
}

HRESULT OceanSurface::CreateFrustumBuffer(ID3D11Device* pd3dDevice,  const Camera &renderCamera)
{
    HRESULT  hr;
    D3D11_BUFFER_DESC bd;

    XMMATRIX invViewprojMat = renderCamera.GetInvViewProjMatrix();
    XMMATRIX mViewProj      = mObserveCamera.GetViewProjMatrix();

    XMFLOAT3 frustum[8] = {
        XMFLOAT3(-1, -1, 0),
        XMFLOAT3(+1, -1, 0),
        XMFLOAT3(-1, +1, 0),
        XMFLOAT3(+1, +1, 0),
        XMFLOAT3(-1, -1, +1),
        XMFLOAT3(+1, -1, +1),
        XMFLOAT3(-1, +1, +1),
        XMFLOAT3(+1, +1, +1) };
    int frustumIndex[24] = {
        0, 1, 0, 2, 2, 3, 1, 3,
        0, 4, 2, 6, 3, 7, 1, 5,
        4, 6, 4, 5, 5, 7, 6, 7 };

    XMFLOAT4 frustum4[8];

    for (int i = 0; i < 8; ++i)
    {
        XMStoreFloat4(&frustum4[i], XMVector3Transform(XMLoadFloat3(&frustum[i]), invViewprojMat) );
        XMStoreFloat4(&frustum4[i], XMLoadFloat4(&frustum4[i]) / frustum4[i].w);
    }
    for (int i = 0; i < 8; ++i)
    {
        XMStoreFloat4(&frustum4[i], XMVector4Transform(XMLoadFloat4(&frustum4[i]), mViewProj) );
    }

    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.ByteWidth = 8 * 16;

    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = frustum4;
    SAFE_RELEASE(mpVertexBuffer);
    V_RETURN(pd3dDevice->CreateBuffer(&bd, &InitData, &mpVertexBuffer));

    bd.ByteWidth = 24 * sizeof(int);
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = frustumIndex;
    SAFE_RELEASE(mpIndexBuffer);
    V_RETURN(pd3dDevice->CreateBuffer(&bd, &InitData, &mpIndexBuffer));

    return S_OK;
}

HRESULT OceanSurface::CreateConstBuffer(ID3D11Device* pd3dDevice )
{
    HRESULT  hr;
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bd.ByteWidth = sizeof(CBChangesEveryFrame);
    V_RETURN(pd3dDevice->CreateBuffer(&bd, nullptr, &mpCBChangesEveryFrame));
    bd.ByteWidth = sizeof(CBWireframe);
    V_RETURN(pd3dDevice->CreateBuffer(&bd, nullptr, &mpCBWireframe));
    return S_OK;
}

HRESULT OceanSurface::CreateEffects(ID3D11Device* pd3dDevice)
{
    HRESULT hr = S_OK;
    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;

    // Disable optimizations to further improve shader debugging
    dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    // Compile the vertex shader
    // Create the vertex shader
    ID3DBlob* pVSBlob = nullptr;
    V_RETURN(DXUTCompileFromFile(mEffectsFile, nullptr, "VS", "vs_4_0", dwShaderFlags, 0, &pVSBlob));
    hr = pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &mpVertexShader);
    if (FAILED(hr))
    {
        SAFE_RELEASE(pVSBlob);
        return hr;
    }

    // Create the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
       // { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
       // { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE(layout);
    pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &mpVertexLayout);
    SAFE_RELEASE(pVSBlob);
    if (FAILED(hr)) return hr;

    // Compile the pixel shader
    // Create the pixel shader
    ID3DBlob* pPSBlob = nullptr;
    V_RETURN(DXUTCompileFromFile(mEffectsFile, nullptr, "PS", "ps_4_0", dwShaderFlags, 0, &pPSBlob));
    hr = pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &mpPixelShader);
    SAFE_RELEASE(pPSBlob);
    return hr;
}

HRESULT OceanSurface::CreateRasterState(ID3D11Device* pd3dDevice)
{
    HRESULT hr = S_OK;

    // Create solid and wireframe rasterizer state objects
    D3D11_RASTERIZER_DESC RasterDesc;
    ZeroMemory(&RasterDesc, sizeof(D3D11_RASTERIZER_DESC));
    RasterDesc.CullMode = D3D11_CULL_NONE;
    RasterDesc.DepthClipEnable = TRUE;
    RasterDesc.FillMode = D3D11_FILL_WIREFRAME;
    V_RETURN( pd3dDevice->CreateRasterizerState(&RasterDesc, &mpRSWireframe) );
    RasterDesc.FillMode = D3D11_FILL_SOLID;
    V_RETURN( pd3dDevice->CreateRasterizerState(&RasterDesc, &mpRSSolid) );

    return hr;
}

HRESULT OceanSurface::BindBuffers(ID3D11DeviceContext* pd3dImmediateContext, int numBuffers, ID3D11Buffer* pVB[], UINT Strides[], UINT Offsets[], ID3D11Buffer* pIB)
{

    pd3dImmediateContext->IASetVertexBuffers(0, 1, pVB, Strides, Offsets);
    pd3dImmediateContext->IASetIndexBuffer(pIB, DXGI_FORMAT_R32_UINT, 0);
    return 0;
}

void OceanSurface::Render(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, const Camera &renderCamera)
{
    if (!IntersectionTest(renderCamera)) return;
    GetSurfaceRange(renderCamera);
    TessellateSurfaceMesh(renderCamera);
    CreateAndUpdateSurfaceMeshBuffer(pd3dDevice);
    mvMeshColor = cvGreen;
    UpdateParameters(pd3dImmediateContext, renderCamera);

    // Render the mesh
    UINT Strides[1] = { sizeof(SurfaceVertex) };
    UINT Offsets[1] = { 0 };
    ID3D11Buffer* pVB[1] = { mpVertexBuffer };
    pd3dImmediateContext->IASetVertexBuffers(0, 1, pVB, Strides, Offsets);
    pd3dImmediateContext->IASetIndexBuffer(mpIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    // Set the Vertex Layout
    pd3dImmediateContext->IASetInputLayout(mpVertexLayout);
    switch (iparameters["primitive_topology"])
    {
    case 0:
        pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
        break;
    case 1:
        pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        break;
    case 2:
        pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        break;
    default:
        break;
    }
    if (iparameters["wireframe"])
    {
        pd3dImmediateContext->RSSetState(mpRSWireframe);
    }
    else
    {
        pd3dImmediateContext->RSSetState(mpRSSolid);
    }

    pd3dImmediateContext->VSSetShader(mpVertexShader, nullptr, 0);
    pd3dImmediateContext->PSSetShader(mpPixelShader, nullptr, 0);
    pd3dImmediateContext->PSSetConstantBuffers(0, 1, &mpCBWireframe);
    pd3dImmediateContext->DrawIndexed(mSurfaceIndex.size(), 0, 0);
}
void OceanSurface::ObserveRenderCameraFrustum(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, const Camera &renderCamera)
{
    CreateFrustumBuffer(pd3dDevice, renderCamera);
    mvMeshColor = cvRed;
    UpdateParameters(pd3dImmediateContext, mObserveCamera);

    pd3dImmediateContext->RSSetState(mpRSWireframe);

    // Render the mesh
    UINT Strides[1] = { 16 };
    UINT Offsets[1] = { 0 };
    ID3D11Buffer* pVB[1] = { mpVertexBuffer };
    // Set the Vertex Layout
    pd3dImmediateContext->IASetInputLayout(mpVertexLayout);
    pd3dImmediateContext->IASetVertexBuffers(0, 1, pVB, Strides, Offsets);
    pd3dImmediateContext->IASetIndexBuffer(mpIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    pd3dImmediateContext->VSSetShader(mpVertexShader, nullptr, 0);
    pd3dImmediateContext->VSSetConstantBuffers(0, 1, &mpCBChangesEveryFrame);

    pd3dImmediateContext->PSSetShader(mpPixelShader, nullptr, 0);
    pd3dImmediateContext->DrawIndexed(24, 0, 0);
}