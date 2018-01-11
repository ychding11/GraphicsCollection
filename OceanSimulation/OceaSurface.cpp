#include "OceanSurface.h"


bool OceanSurface::IntersectionTest(const CBaseCamera &renderCamera)
{
    const XMFLOAT3 frustum[8] = {
            XMFLOAT3(-1, -1, -1),                                   
            XMFLOAT3(+1, -1, -1),                                   
            XMFLOAT3(-1, +1, -1),                                   
            XMFLOAT3(+1, +1, -1),                                   
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

    nPoints = 0;
    for (int i = 0; i < 8; ++i) worldFrustum[i] = XMVector3TransformCoord(XMLoadFloat3(&frustum[i]), invViewprojMat);

    // check intersections with upper_bound and lower_bound	
    for (int i = 0; i < 12; ++i)
    {
        int src = cube[i * 2], dst = cube[i * 2 + 1];
		float distUpper1 = XMPlaneDotCoord(mUpper, worldFrustum[src]).m128_f32[0];
		float distUpper2 = XMPlaneDotCoord(mUpper, worldFrustum[dst]).m128_f32[0];
		float distLow1   = XMPlaneDotCoord(mLow, worldFrustum[src]).m128_f32[0];
		float distLow2   = XMPlaneDotCoord(mLow, worldFrustum[dst]).m128_f32[0];

        if (distUpper1 * distUpper2 < 0.0f)
        {
            points[nPoints++] = XMPlaneIntersectLine(mUpper, worldFrustum[src], worldFrustum[dst]);
        }
        if (distLow1 * distLow2 < 0.0f)
        {
            points[nPoints++] = XMPlaneIntersectLine(mLow, worldFrustum[src], worldFrustum[dst]);
        }
    }
    for (int i = 0; i < 8; i++)
    {
		float distUpper = XMPlaneDotCoord(mUpper, worldFrustum[i]).m128_f32[0];
		float distLow   = XMPlaneDotCoord(mLow, worldFrustum[i]).m128_f32[0];
        if (( distUpper < 0) && ( distLow > 0))
        {
           points[nPoints++] = worldFrustum[i];
        }
    }
    return nPoints > 0;
}

void OceanSurface::GetIntersectionRange(const CBaseCamera &renderCamra)
{
    XMVECTOR eyePos    = renderCamra.GetEyePt();
    XMVECTOR eyeTarget = renderCamra.GetLookAtPt();
    XMVECTOR projectorPos = eyePos;
    XMVECTOR aimPoint1, aimPoint2;
    XMVECTOR viewDir = XMVector3Normalize(eyeTarget - eyePos);
    float eyeHeiht2Low  = XMPlaneDotCoord(mLow, eyePos).m128_f32[0];
    float eyeHeiht2Base = XMPlaneDotCoord(mBase, eyePos).m128_f32[0];
    float cosTheta      = XMPlaneDotNormal(mBase, viewDir).m128_f32[0];

    if (eyeHeiht2Low < 8 + 7)
    {
        if (eyeHeiht2Low < 0.0f)
        {
            projectorPos += mLow * (8 + 7 - 2.0 * eyeHeiht2Low);
        }
        else
        {
            projectorPos += mLow * (8 + 7 - eyeHeiht2Low);
        }
    }

    if ( cosTheta < 0.0f != eyeHeiht2Base < 0.0f)
    {
        aimPoint1 = XMPlaneIntersectLine(mBase, eyePos, eyePos + viewDir *  1000);
    }
    else
    {
        XMVECTOR mirror = viewDir - 2.0 * cosTheta * mBase;
        aimPoint1 = XMPlaneIntersectLine(mBase, eyePos, eyePos + mirror *  1000);
    }

    aimPoint2 = eyePos + viewDir * 10;
    aimPoint2 = aimPoint2 - mBase * XMPlaneDotCoord(mBase, aimPoint2).m128_f32[0];
    XMVECTOR aimPoint = cosTheta * aimPoint1 + (1.0 - cosTheta) * aimPoint2;
    XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.f };
    XMMATRIX viewMat = XMMatrixLookAtLH(projectorPos, aimPoint, up);
    XMMATRIX projMat = XMMatrixPerspectiveFovLH(mFOV, mAspect, renderCamra.GetNearClip(), renderCamra.GetFarClip());

	float minX = 0.0f, maxX = 0.0f, minY = 0.0f, maxY = 0.0f;
	// projector space
    for (int i = 0; i < nPoints; ++i)
    {
		float height2Base = XMPlaneDotCoord(mBase, points[i]).m128_f32[0];
        points[i] = points[i] - mBase * height2Base; 
        points[i] = XMVector3TransformCoord(points[i], viewMat);
        points[i] = XMVector3TransformCoord(points[i], projMat);
		float x = points[i].m128_f32[0];
		float y = points[i].m128_f32[1];
		if (x < minX) minX = x;
		if (x > maxX) maxX = x;
		if (y < minY) minY = y;
		if (y > maxY) maxY = y;
    }
	mXspan = maxX - minX;
	mYspan = maxY - minY;

	XMMATRIX viewprojMat = viewMat * projMat;
	XMMATRIX invViewprojMat = XMMatrixInverse(nullptr, viewprojMat);

	XMFLOAT2 gridConer[4] = {
		XMFLOAT2(minX, minY),
		XMFLOAT2(maxX, minY),
		XMFLOAT2(minX, maxY),
		XMFLOAT2(maxX, maxY),
	}
}