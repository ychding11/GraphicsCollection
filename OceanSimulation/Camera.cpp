#include "Camera.h"

static bool	keys[256] = { false };
LRESULT Camera::HandleMessages(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_KEYDOWN:							// Is A Key Being Held Down?
        {
            keys[wParam] = TRUE;					// If So, Mark It As TRUE
            // preset handling
            if (keys['1'])
            {
                return 0;
            }
            if (keys['2'])
            {
                return 0;
            }
            if (keys['3'])
            {
                return 0;
            }
            if (keys['4'])
            {
                return 0;
            }
            if (keys['5'])
            {
                return 0;
            }
            if (keys['6'])
            {
                return 0;
            }
            if (keys['T'])
            {
                return 0;
            }
            // other keys
            if (keys[VK_TAB])
            {
                return 0;
            }
            if (keys['C'])
            {
                return 0;
            }
            if (keys['E'])
            {
                return 0;
            }
            if (keys['0'])
            {
                return 0;
            }

            if (keys[VK_PRIOR])
            {
                return 0;
            }
            if (keys[VK_NEXT])
            {
                return 0;
            }
            if (keys[VK_HOME])
            {
                return 0;
            }
            if (keys[VK_END])
            {
                return 0;
            }
            if (keys[VK_SUBTRACT])
            {
                return 0;
            }
            if (keys[VK_ADD])
            {
                return 0;
            }

            if (keys[VK_LEFT])
            {
                return 0;
            }
            if (keys[VK_RIGHT])
            {
                return 0;
            }
            if (keys[VK_UP])
            {
                return 0;
            }
            if (keys[VK_DOWN])
            {
                return 0;
            }

            return 0;								// Jump Back
        }

        case WM_KEYUP:								// Has A Key Been Released?
        {
            keys[wParam] = FALSE;					// If So, Mark It As FALSE
            return 0;								// Jump Back
        }

    }
}