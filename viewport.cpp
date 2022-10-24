
#include "viewport.h"


LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
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

void CViewport::AddAdapterNameToWindowTitle(TCHAR* title) const
{
	TCHAR pszWndTitle[MAX_PATH]{};
	::GetWindowText(m_hwnd, pszWndTitle, MAX_PATH);
	StringCchPrintf(pszWndTitle, MAX_PATH, _T("%s (GPU:%s)"), pszWndTitle, title);
	::SetWindowText(m_hwnd, pszWndTitle);
}
