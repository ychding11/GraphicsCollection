#include "Camera.h"
#include "Logger.h"

//using namespace DirectX;

static bool	keys[256] = { false };
LRESULT Camera::HandleMessages(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_KEYDOWN:							// Is A Key Being Held Down?
        {
            XMVECTOR detPosition = XMLoadFloat3(&this->position);
            keys[wParam] = true;					// If So, Mark It As TRUE
            if (keys['1'])
            {
                return 0;
            }
#if 0
            else if (keys['2'])
            {
                return 0;
            }
            else if (keys['3'])
            {
                return 0;
            }
            else if (keys['T'])
            {
                return 0;
            }
            else if (keys[VK_TAB])
            {
                return 0;
            }
            else if (keys['C'])
            {
                return 0;
            }
            else if (keys['E'])
            {
                return 0;
            }
            else if (keys['0'])
            {
                return 0;
            }
            else if (keys[VK_PRIOR])
            {
                return 0;
            }
            else if (keys[VK_NEXT])
            {
                return 0;
            }
            else if (keys[VK_HOME])
            {
                return 0;
            }
            else if (keys[VK_LEFT])
            {
                return 0;
            }
            else if (keys[VK_RIGHT])
            {
                return 0;
            }
            else if (keys[VK_UP])
            {
                return 0;
            }
            else if (keys[VK_DOWN])
            {
                return 0;
            }
#endif
            else if (keys['A'])
            {
                detPosition -= this->movespeed * XMLoadFloat3( &this->right);
            }
            else if (keys['D'])
            {
                detPosition  += this->movespeed * XMLoadFloat3( &this->right);
            }
            else if (keys['W'])
            {
                detPosition += this->movespeed * XMLoadFloat3( &this->forward);
            }
            else if (keys['S'])
            {
                detPosition -= this->movespeed * XMLoadFloat3( &this->forward);
            }
            else if (keys['Q'])
            {
                detPosition += this->movespeed * XMLoadFloat3( &this->up);
            }
            else if (keys['Z'])
            {
                detPosition -= this->movespeed * XMLoadFloat3( &this->up);
            }

            if (keys['Z'] || keys['Q'] || keys['S'] || keys['W'] || keys['D'] || keys['A'])
            {
                Logger::GetLogger() << "Camera Position: [" << this->position.x << " " << this->position.y << " " <<this->position.z << "]" << std::endl;
                this->UpdateCameraByPos(detPosition);
            }

            return 0;								// Jump Back
        }

        case WM_KEYUP:								// Has A Key Been Released?
        {
            keys[wParam] = false;					// If So, Mark It As FALSE
            return 0;								// Jump Back
        }

    }
}