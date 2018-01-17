#ifndef _c_camera_
#define _c_camera_

#include <DirectXMath.h>

using namespace DirectX;

class Camera {
private:

    float fov, aspect, znear, zfar;
    float rotx, roty, rotz;
    XMFLOAT3 position;
    XMFLOAT3 forward, up, right;
    XMMATRIX view, invview, proj, invproj, viewproj, invviewproj;

public:
    Camera()
        : fov(XM_PI * 0.25), aspect(1.78), znear(0.01), zfar(1000)
        , rotx(0), roty(0), rotz(0)
        , position(0, 0, 0) 
        , forward(0, 0, 1)
        , right(1, 0, 0)
        , up(0, 1, 0)
    {

    }

    Camera(XMFLOAT3 pos, float rotx, float roty, float rotz, float fov, float aspect, float nearz, float farz)
    {
        this->position = pos;
        this->rotx = rotx; this->roty = roty; this->rotz = rotz;
        this->fov = fov; this->aspect = aspect; this->znear = nearz; this->zfar = farz;
    }

    Camera(XMFLOAT3 pos, XMFLOAT3 target, float fov, float aspect, float nearz, float farz)
    {
        this->position = pos;
        this->rotx = rotx; this->roty = roty; this->rotz = rotz;
        this->fov = fov; this->aspect = aspect; this->znear = nearz; this->zfar = farz;
        view = XMMatrixLookAtLH( XMLoadFloat3(&position), XMLoadFloat3(&target), XMLoadFloat3(&XMFLOAT3(0, 1, 0)) );
        proj = XMMatrixPerspectiveFovLH(this->fov, this->aspect, this->znear, this->zfar);

        viewproj = view*proj;
        invproj = XMMatrixInverse(nullptr, proj);
        invview = XMMatrixInverse(nullptr, view);
        invviewproj = XMMatrixInverse(nullptr, viewproj);

        // update the direction vectors
        XMStoreFloat3(&forward, XMVector3TransformNormal(XMLoadFloat3(&XMFLOAT3(0, 0, 1)), invview));
        XMStoreFloat3(&up, XMVector3TransformNormal(XMLoadFloat3(&XMFLOAT3(0, 1, 0)), invview));
        XMStoreFloat3(&right, XMVector3TransformNormal(XMLoadFloat3(&XMFLOAT3(1, 0, 0)), invview));
    }

    Camera(const Camera &src)
    { }

    ~Camera()
    { }

    XMMATRIX GetInvViewMatrix()
    { return this->invview; }
    XMMATRIX GetInvProjMatrix()
    { return this->invproj; }
    XMMATRIX GetViewMatrix()
    { return this->view; }
    XMMATRIX GetProjMatrix()
    { return this->proj; }
    XMMATRIX GetViewProjMatrix()
    { return this->viewproj; }
    XMMATRIX GetInvViewProjMatrix()
    { return this->invviewproj; }

    void UpdateAspect(float aspect)
    {
        this->aspect = aspect;
        proj = XMMatrixPerspectiveFovLH(this->fov, this->aspect, this->znear, this->zfar);

        viewproj = view * proj;
        invproj = XMMatrixInverse(nullptr, proj);
        invviewproj = XMMatrixInverse(nullptr, viewproj);
    }

    void Update()
    {
        XMMATRIX rotatex, rotatey, rotatez, translation;
        // perspective matrix
        proj = XMMatrixPerspectiveFovLH(fov, aspect, znear, zfar);

        // view matrix
        translation = XMMatrixTranslation(-position.x, -position.y, -position.z);
        rotatex = XMMatrixRotationX(rotx);
        rotatey = XMMatrixRotationY(roty);
        rotatez = XMMatrixRotationZ(rotz);

        view = translation * rotatey * rotatex * rotatez;
        viewproj = view*proj;

        invproj = XMMatrixInverse( nullptr, proj);
        invview = XMMatrixInverse( nullptr, view);
        invviewproj = XMMatrixInverse( nullptr, viewproj);

        // and the direction vectors
        XMStoreFloat3( &forward, XMVector3TransformNormal( XMLoadFloat3(&XMFLOAT3(0, 0, 1)), invview ) );
        XMStoreFloat3( &up, XMVector3TransformNormal( XMLoadFloat3(&XMFLOAT3(0, 1, 0)), invview ) );
        XMStoreFloat3( &right, XMVector3TransformNormal( XMLoadFloat3(&XMFLOAT3(1, 0, 0)), invview ) );
    }
};

#endif