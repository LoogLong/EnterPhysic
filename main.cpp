#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN // 从 Windows 头中排除极少使用的资料
#include <windows.h>
#include <tchar.h>
#include <wrl.h>  //添加WTL支持 方便使用COM
#include <strsafe.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <iostream>

#include "rope.h"
#include <vector>
#include <string>
#include "define.h"
#include "viewport.h"
#include "D3D12RHI.h"
#include "cloth.h"
#include <chrono>
using namespace Microsoft;
using namespace DirectX;

//linker
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")



PhysicRope g_physic_rope{};
PhysicCloth g_physic_cloth{};

void GameUpdate(D3D12RHI& rhi)
{
	// RenderUpdate
	std::vector<RHI_VERTEX> point_vertices;
	std::vector<RHI_VERTEX> line_vertices;

	//g_physic_rope.UpdatePhysic(point_vertices, line_vertices);

	g_physic_cloth.UpdatePhysic(point_vertices, line_vertices);

	XMFLOAT4X4 local_transform{};
	const XMMATRIX local_matrix = XMMatrixIdentity();

	XMStoreFloat4x4(&local_transform, local_matrix);

	rhi.CreateResource(point_vertices, 0, 0, D3D10_PRIMITIVE_TOPOLOGY_POINTLIST, 0);
	rhi.UpdateLocalConstantBuffer(0, &local_transform, sizeof(XMFLOAT4X4));

	rhi.CreateResource(line_vertices, 1, 0, D3D10_PRIMITIVE_TOPOLOGY_LINELIST, 1);
	rhi.UpdateLocalConstantBuffer(1, &local_transform, sizeof(XMFLOAT4X4));
}





int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	AllocConsole();
	FILE* stream;
	freopen_s(&stream, "CONOUT$", "w+t", stdout);
	freopen_s(&stream, "CONIN$", "r+t", stdin);

	int width = 1024;
	int height = 768;

	CViewport main_viewport(width, height);
	main_viewport.CreateWinViewport(hInstance, nCmdShow);

	D3D12RHI rhi(main_viewport.GetWindowHwnd());

	rhi.CreateDevice();
	const auto adapter_name = rhi.GetCurrentAdapterName();
	main_viewport.AddAdapterNameToWindowTitle(adapter_name);

	rhi.CreateCommandQueue();

	rhi.CreateSwapChain(width, height);
	rhi.CreateRootSignature();
	rhi.CreatePipelineStateObject();
	rhi.CreateRenderEndFence();
	rhi.CreateGlobalConstantBuffer();
	rhi.CreateLocalConstantBuffer(2);
	


	// main loop
	MSG msg = {};
	DWORD dwRet = 0;
	BOOL bExit = FALSE;

	auto last_time = chrono::steady_clock::now();
	
	while (!bExit)
	{
		dwRet = rhi.WaitForFence();
		switch (dwRet - WAIT_OBJECT_0)
		{
		case 0:
			{
				rhi.ClearTempResource();

				auto now_time = chrono::steady_clock::now();
				auto delta_time = now_time - last_time;
				last_time = now_time;

				const auto t = delta_time.count();
				const float fps = static_cast<float>(1000000000) / t;
				main_viewport.SetFPS(fps);
				//printf("fps:%.2f\n", fps);


				GameUpdate(rhi);

				XMFLOAT4X4 local_transform{};
				XMMATRIX xm_view{};
				CViewport::GetViewMatrix(xm_view);

				XMMATRIX xm_proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, static_cast<FLOAT>(width) / static_cast<FLOAT>(height), 1.0f, 2000.0f);
				const XMMATRIX xm_vp = XMMatrixMultiply(xm_view, xm_proj);

				XMStoreFloat4x4(&local_transform, xm_vp);
				rhi.UpdateGlobalConstantBuffer(&local_transform, sizeof(XMFLOAT4X4));


				rhi.BeginFrame();
				rhi.RenderObject();
				rhi.EndFrame();
			}
			break;
		case 1:
			{
				//处理消息
				while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
				{
					if (WM_QUIT != msg.message)
					{
						::TranslateMessage(&msg);
						::DispatchMessage(&msg);
					}
					else
					{
						bExit = TRUE;
					}
				}
			}
			break;
		case WAIT_TIMEOUT:
			{
			}
			break;
		default:
			break;
		}
	}
	FreeConsole();
	return 0;
}

