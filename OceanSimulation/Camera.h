#ifndef _c_camera_
#define _c_camera_

#include <DirectXMath.h>
#include <windows.h>

using namespace DirectX;

class Camera {
private:

    float fov, aspect, znear, zfar;
    float rotx, roty, rotz;
    XMFLOAT3 position;
    XMFLOAT3 target;
    XMFLOAT3 forward, up, right;
    XMMATRIX view, invview, proj, invproj, viewproj, invviewproj;

    float movespeed;
public:
    Camera()
        : fov(XM_PI * 0.25), aspect(1.78), znear(0.01), zfar(1000)
        , rotx(0), roty(0), rotz(0)
        , position(0, 0, 0), forward(0, 0, 1)
        , right(1, 0, 0), up(0, 1, 0)
        , movespeed(1.0)
    {

    }

private:
    Camera(XMFLOAT3 pos, float rotx, float roty, float rotz, float fov, float aspect, float nearz, float farz)
    {
        this->position = pos;
        this->rotx = rotx; this->roty = roty; this->rotz = rotz;
        this->fov = fov; this->aspect = aspect; this->znear = nearz; this->zfar = farz;
    }
public:
    Camera(XMFLOAT3 pos, XMFLOAT3 target, float fov = XM_PI * 0.25, float aspect = 1.78, float nearz = 0.1, float farz = 50.0)
        : movespeed(1.0)
    {
        this->position = pos;
        this->target = target;
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

    LRESULT HandleMessages(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);

    XMMATRIX GetInvViewMatrix() const
    { return this->invview; }
    XMMATRIX GetInvProjMatrix() const
    { return this->invproj; }
    XMMATRIX GetViewMatrix() const
    { return this->view; }
    XMMATRIX GetProjMatrix() const
    { return this->proj; }
    XMMATRIX GetViewProjMatrix() const
    { return this->viewproj; }
    XMMATRIX GetInvViewProjMatrix() const
    { return this->invviewproj; }
    XMFLOAT3 GetEyePt() const
    { return  this->position; }
    XMFLOAT3 GetViewDir() const
    { return  this->forward; }

    void UpdateAspect(float aspect)
    {
        this->aspect = aspect;
        UpdateProj();
    }

private:

    void UpdateCameraByPos(XMVECTOR eyePos)
    {
        XMStoreFloat3(&this->position, eyePos);
        UpdateViewByPos();
    }

    void UpdateViewByPos()
    {
        view = XMMatrixLookAtLH(XMLoadFloat3(&this->position), XMLoadFloat3(&this->target), XMLoadFloat3(&XMFLOAT3(0, 1, 0)));
        invview = XMMatrixInverse(nullptr, view);
        viewproj = view*proj;
        invviewproj = XMMatrixInverse(nullptr, viewproj);

        // update the direction vectors
        XMStoreFloat3(&forward, XMVector3TransformNormal(XMLoadFloat3(&XMFLOAT3(0, 0, 1)), invview));
        XMStoreFloat3(&up, XMVector3TransformNormal(XMLoadFloat3(&XMFLOAT3(0, 1, 0)), invview));
        XMStoreFloat3(&right, XMVector3TransformNormal(XMLoadFloat3(&XMFLOAT3(1, 0, 0)), invview));

    }
    
    void UpdateProj()
    {
        proj = XMMatrixPerspectiveFovLH(this->fov, this->aspect, this->znear, this->zfar);
        invproj = XMMatrixInverse(nullptr, proj);
        viewproj = view * proj;
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