#include "OceanSurface.h"
#include "SDKmisc.h"
//#include "DDSTextureLoader.h"

bool OceanSurface::IntersectionTest(const CBaseCamera &renderCamera)
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
    XMMATRIX viewMat = renderCamera.GetViewMatrix();
    XMMATRIX projMat = renderCamera.GetProjMatrix();
    XMMATRIX viewprojMat = viewMat * projMat;
    XMMATRIX invViewprojMat = XMMatrixInverse(nullptr, viewprojMat);

    //for (int i = 0; i < 8; ++i) XMStoreFloat3( &frustum[i], XMLoadFloat3(&frustum[i]) * renderCamera.GetFarClip() );
    for (int i = 0; i < 8; ++i) worldFrustum[i] = XMVector3TransformCoord(XMLoadFloat3(&frustum[i]), invViewprojMat);

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

void OceanSurface::GetSurfaceRange(const CBaseCamera &renderCamera)
{
    XMVECTOR eyePos    = renderCamera.GetEyePt();
    XMVECTOR eyeTarget = renderCamera.GetLookAtPt();
    XMVECTOR projectorPos = eyePos;
    XMVECTOR aimPoint1, aimPoint2;
    XMVECTOR viewDir = XMVector3Normalize(eyeTarget - eyePos);
    float eyeHeiht2Low  = XMPlaneDotCoord(mLow, eyePos).m128_f32[0];
    float eyeHeiht2Base = XMPlaneDotCoord(mBase, eyePos).m128_f32[0];
    float cosTheta      = XMPlaneDotNormal(mBase, viewDir).m128_f32[0];

    assert( eyeTarget.m128_f32[3] == 0.0);
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
        XMVECTOR mirror = viewDir - 2.0 * cosTheta * mNormal;
        aimPoint1 = XMPlaneIntersectLine(mBase, eyePos, eyePos + mirror *  1000);
    }

    aimPoint2 = eyePos + viewDir * 10.0f;
    aimPoint2 = aimPoint2 - mNormal * XMPlaneDotCoord(mBase, aimPoint2).m128_f32[0];
    XMVECTOR aimPoint = cosTheta * aimPoint1 + (1.0 - cosTheta) * aimPoint2;

    XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.f };
    XMMATRIX viewMat = XMMatrixLookAtLH(projectorPos, aimPoint, up);
    XMMATRIX projMat = renderCamera.GetProjMatrix();

	float minX = 0.0f, maxX = 0.0f, minY = 0.0f, maxY = 0.0f;
	// projector space
    int nPoints = mIntersectionPoints.size();
    for (int i = 0; i < nPoints; ++i)
    {
        XMVECTOR point = mIntersectionPoints[i];
		float height2Base = XMPlaneDotCoord(mBase, point).m128_f32[0];
        point = point - mNormal * height2Base; 
        point = XMVector3TransformCoord(point, viewMat);
        point = XMVector3TransformCoord(point, projMat);
		float x = point.m128_f32[0];
		float y = point.m128_f32[1];
		if (x < minX) minX = x;
		if (x > maxX) maxX = x;
		if (y < minY) minY = y;
		if (y > maxY) maxY = y;
    }

	XMFLOAT2 gridConer[4] = {
		XMFLOAT2(minX, minY),
		XMFLOAT2(maxX, minY),
		XMFLOAT2(minX, maxY),
		XMFLOAT2(maxX, maxY),
	};

	XMMATRIX viewprojMat = viewMat * projMat;
	XMMATRIX invViewprojMat = XMMatrixInverse(nullptr, viewprojMat);
    mGridConer[0] = getWorldGridConer(gridConer[0], renderCamera, invViewprojMat );
    mGridConer[1] = getWorldGridConer(gridConer[1], renderCamera, invViewprojMat );
    mGridConer[2] = getWorldGridConer(gridConer[2], renderCamera, invViewprojMat );
    mGridConer[3] = getWorldGridConer(gridConer[3], renderCamera, invViewprojMat );

}

void OceanSurface::TessellateSurfaceMesh(void)
{

    int sizeX = mXSize;
    int sizeY = mYSize;
    float u = 0.0f, v = 0.0f;
    float du = 1.0 / float(sizeX - 1), dv = 1.0 / float(sizeY - 1);
    XMFLOAT3  grid[4];

    XMStoreFloat3( &grid[0], mGridConer[0] );
    XMStoreFloat3( &grid[1], mGridConer[1] );
    XMStoreFloat3( &grid[2], mGridConer[2] );
    XMStoreFloat3( &grid[3], mGridConer[3] );

    mSurfaceIndex.clear();
    mSurfaceVertex.clear();
    for (int i = 0; i < sizeY; ++i)
    {
        u = 0.0f;
        for (int j = 0; j < sizeX; ++j)
        {
            float x = (1.0f - v) * ( (1.0f - u) * grid[0].x + u * grid[1].x) + v * (u * grid[2].x + (1.0 -u) * grid[3].x );
            float z = (1.0f - v) * ( (1.0f - u) * grid[0].z + u * grid[1].z) + v * (u * grid[2].z + (1.0 -u) * grid[3].z );
            float y = 0.0f;
            SurfaceVertex vertex = {x, y, z};
            mSurfaceVertex.push_back(vertex);
            u += du;
        }
        v += dv;
    }

    for (int i = 0; i < sizeY - 1; ++i)
    {
        for (int j = 0; j < sizeX - 1; ++j)
        {
            int a = sizeX * i + j;
            int b = sizeX * (i + 1) + j;
            int c = a + 1;
            int d = b + 1;
            
            mSurfaceIndex.push_back(a);
            mSurfaceIndex.push_back(b);
            mSurfaceIndex.push_back(c);

            mSurfaceIndex.push_back(a);
            mSurfaceIndex.push_back(c);
            mSurfaceIndex.push_back(d);
        }
    }
}

XMVECTOR OceanSurface::getWorldGridConer(XMFLOAT2 coner, const CBaseCamera &renderCamera, const XMMATRIX &invViewprojMat)
{
    float farPlane = renderCamera.GetFarClip();
    XMVECTORF32 point1 = {coner.x, coner.y, 0, 1.0};
    XMVECTORF32 point2 = {coner.x, coner.y, 1.0, 1.0};
    XMVECTOR start = XMVector3TransformCoord(point1, invViewprojMat);
    XMVECTOR end   = XMVector3TransformCoord(point2, invViewprojMat);
    XMVECTOR ret = XMPlaneIntersectLine(mBase, start, end);
    return ret;
}

void OceanSurface::UpdateParameters(ID3D11DeviceContext* pd3dImmediateContext, const CBaseCamera &renderCamera)
{
    HRESULT hr;
    XMMATRIX mView = renderCamera.GetViewMatrix();
    XMMATRIX mProjection = renderCamera.GetProjMatrix();
    XMMATRIX mWorldViewProjection = mmWorld * mView * mProjection;

    // Update constant buffer that changes once per frame
    D3D11_MAPPED_SUBRESOURCE MappedResource;
    V(pd3dImmediateContext->Map(mpCBChangesEveryFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
    auto pCB = reinterpret_cast<CBChangesEveryFrame*>(MappedResource.pData);
    XMStoreFloat4x4(&pCB->mWorldViewProj, XMMatrixTranspose(mWorldViewProjection));
    XMStoreFloat4x4(&pCB->mWorld, XMMatrixTranspose(mmWorld));
    //pCB->vMeshColor = mvMeshColor;
    pd3dImmediateContext->Unmap(mpCBChangesEveryFrame, 0);
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
    return S_OK;
}

HRESULT OceanSurface::CreateEffects(ID3D11Device* pd3dDevice, void* pUserContext)
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
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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

