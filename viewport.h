#pragma once
#include <strsafe.h>
#include <tchar.h>
#include <windows.h>

#define WIND_CLASS_NAME _T("GRS Game Window Class")
#define WINDOW_TITLE	_T("Direct12 Physics Smulate Sample")


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

class CViewport
{
public:
	CViewport(int width, int height):m_width(width), m_height(height) {}
	bool CreateWinViewport(const HINSTANCE& hInstance, int nCmdShow);
	void AddAdapterNameToWindowTitle(TCHAR* title) const;

	HWND& GetWindowHwnd() { return  m_hwnd; }

private:
	HWND m_hwnd { nullptr };
	int m_width{ 1920 };
	int m_height{ 1080 };
};