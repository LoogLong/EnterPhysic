#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN // 从 Windows 头中排除极少使用的资料
#include <windows.h>
#include <tchar.h>
#include <wrl.h>  //添加WTL支持 方便使用COM
#include <strsafe.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>

#include "rope.h"
#include <vector>
#include <string>
#include "define.h"
#include "viewport.h"
#include "D3D12RHI.h"
#include "cloth.h"
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


	rhi.CreateResource(point_vertices, 0, 0, D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
	rhi.CreateResource(line_vertices, 1, 0, D3D10_PRIMITIVE_TOPOLOGY_LINELIST);
}





int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
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

	


	// main loop
	MSG msg = {};
	DWORD dwRet = 0;
	BOOL bExit = FALSE;
	
	while (!bExit)
	{
		dwRet = rhi.WaitForFence();
		switch (dwRet - WAIT_OBJECT_0)
		{
		case 0:
			{
				rhi.ClearTempResource();

				
				GameUpdate(rhi);
				


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
	return 0;
}

