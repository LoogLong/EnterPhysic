
#include "viewport.h"

#include <DirectXMath.h>

using namespace DirectX;

//初始的默认摄像机的位置
constexpr XMFLOAT3 EYE_POS = XMFLOAT3(0.0f, 2.0f, -600.0f); //眼睛位置
constexpr XMFLOAT3 LOCK_AT = XMFLOAT3(0.0f, 2.0f, 0.0f);    //眼睛所盯的位置
constexpr XMFLOAT3 HEAP_UP = XMFLOAT3(0.0f, 1.0f, 0.0f);    //头部正上方位置
constexpr float YAW = 0.f;
constexpr float PITCH = 0.f;
constexpr float MOVE_SPEED = 10.0f;
constexpr float TURN_SPEED = XM_PIDIV2 * 0.01f;

XMFLOAT3 g_f3_eye_pos = EYE_POS;
XMFLOAT3 g_f3_lock_at = LOCK_AT;
XMFLOAT3 g_f3_heap_up = HEAP_UP; 

float g_yaw = YAW;			// 绕正Z轴的旋转量.
float g_pitch = PITCH;			// 绕XZ平面的旋转量


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_KEYDOWN:
	{
		const USHORT n16KeyCode = (wParam & 0xFF);
		if (VK_SPACE == n16KeyCode)
		{

		}
		if (VK_ADD == n16KeyCode || VK_OEM_PLUS == n16KeyCode)
		{
			
		}

		if (VK_SUBTRACT == n16KeyCode || VK_OEM_MINUS == n16KeyCode)
		{
			
		}

		XMFLOAT3 move(0, 0, 0);


		if ('w' == n16KeyCode || 'W' == n16KeyCode)
		{
			move.z -= 1.0f;
		}

		if ('s' == n16KeyCode || 'S' == n16KeyCode)
		{
			move.z += 1.0f;
		}

		if ('d' == n16KeyCode || 'D' == n16KeyCode)
		{
			move.x += 1.0f;
		}

		if ('a' == n16KeyCode || 'A' == n16KeyCode)
		{
			move.x -= 1.0f;
		}
		if ('q' == n16KeyCode || 'Q' == n16KeyCode)
		{
			move.y -= 1.0f;
		}
		if ('e' == n16KeyCode || 'E' == n16KeyCode)
		{
			move.y += 1.0f;
		}

		// if (fabs(move.x) > 0.1f && fabs(move.z) > 0.1f)
		// {
		// 	const XMVECTOR vector = XMVector3Normalize(XMLoadFloat3(&move));
		// 	move.x = XMVectorGetX(vector);
		// 	move.z = XMVectorGetZ(vector);
		// }

		if (VK_UP == n16KeyCode)
		{
			g_pitch += TURN_SPEED;
		}

		if (VK_DOWN == n16KeyCode)
		{
			g_pitch -= TURN_SPEED;
		}

		if (VK_RIGHT == n16KeyCode)
		{
			g_yaw -= TURN_SPEED;
		}

		if (VK_LEFT == n16KeyCode)
		{
			g_yaw += TURN_SPEED;
		}

		// Prevent looking too far up or down.
		g_pitch = min(g_pitch, XM_PIDIV4);
		g_pitch = max(-XM_PIDIV4, g_pitch);

		// Move the camera in model space.
		g_f3_eye_pos.x += move.x * MOVE_SPEED;
		g_f3_eye_pos.y += move.y * MOVE_SPEED;
		g_f3_eye_pos.z += move.z * MOVE_SPEED;

		g_f3_lock_at.x += move.x * MOVE_SPEED;
		g_f3_lock_at.y += move.y * MOVE_SPEED;
		g_f3_lock_at.z += move.z * MOVE_SPEED;
		// Determine the look direction.
		float r = cosf(g_pitch);
		g_f3_lock_at.x = r * sinf(g_yaw) + g_f3_eye_pos.x;
		g_f3_lock_at.y = sinf(g_pitch) + g_f3_eye_pos.y;
		g_f3_lock_at.z = r * cosf(g_yaw) + g_f3_eye_pos.z;

		if (VK_TAB == n16KeyCode)
		{//按Tab键还原摄像机位置
			g_f3_eye_pos = EYE_POS;
			g_f3_lock_at = LOCK_AT;
			g_f3_heap_up = HEAP_UP;

			g_yaw = YAW;
			g_pitch = PITCH;
		}
	}

	break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

bool CViewport::CreateWinViewport(const HINSTANCE& hInstance, int nCmdShow)
{
	// 创建窗口
	WNDCLASSEX wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_GLOBALCLASS;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH)); //防止无聊的背景重绘
	wcex.lpszClassName = WIND_CLASS_NAME;
	RegisterClassEx(&wcex);

	constexpr DWORD dw_wnd_style = WS_OVERLAPPED | WS_SYSMENU;
	RECT rt_wnd = { 0, 0, m_width, m_height };
	AdjustWindowRect(&rt_wnd, dw_wnd_style, FALSE);

	// 计算窗口居中的屏幕坐标
	const INT pos_x = (GetSystemMetrics(SM_CXSCREEN) - rt_wnd.right - rt_wnd.left) / 2;
	const INT pos_y = (GetSystemMetrics(SM_CYSCREEN) - rt_wnd.bottom - rt_wnd.top) / 2;

	m_hwnd = CreateWindowW(WIND_CLASS_NAME
		, WINDOW_TITLE
		, dw_wnd_style
		, pos_x
		, pos_y
		, rt_wnd.right - rt_wnd.left
		, rt_wnd.bottom - rt_wnd.top
		, nullptr
		, nullptr
		, hInstance
		, nullptr);

	if (!m_hwnd)
	{
		return false;
	}

	ShowWindow(m_hwnd, nCmdShow);
	UpdateWindow(m_hwnd);
	return true;
}

void CViewport::AddAdapterNameToWindowTitle(TCHAR* title)
{
	TCHAR pszWndTitle[MAX_PATH]{};
	::GetWindowText(m_hwnd, pszWndTitle, MAX_PATH);
	StringCchPrintf(pszWndTitle, MAX_PATH, _T("%s GPU:%s"), pszWndTitle, title);
	::SetWindowText(m_hwnd, pszWndTitle);

	StringCchPrintf(m_pszWndTitle, MAX_PATH, _T("%s"), pszWndTitle);
}

void CViewport::SetFPS(float fps) const
{
	TCHAR pszWndTitle[MAX_PATH]{};
	StringCchPrintf(pszWndTitle, MAX_PATH, _T("%s(FPS:%.2f)"), m_pszWndTitle, fps);
	::SetWindowText(m_hwnd, pszWndTitle);
}

void CViewport::GetViewMatrix(XMMATRIX& out_matrix)
{
	out_matrix = XMMatrixLookAtLH(XMLoadFloat3(&g_f3_eye_pos)
		, XMLoadFloat3(&g_f3_lock_at)
		, XMLoadFloat3(&g_f3_heap_up));
}