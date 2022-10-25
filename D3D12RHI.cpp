#include "D3D12RHI.h"
#include "define.h"

D3D12RHI::D3D12RHI(const HWND& hwnd)
{
	// 打开显示子系统的调试支持
	{
#if defined(_DEBUG)
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
			// 打开附加的调试支持
			m_dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
		}
#endif
	}

	// 创建DXGI Factory对象
	{
		CHECK_RESULT(CreateDXGIFactory2(m_dxgi_factory_flags, IID_PPV_ARGS(&m_dxgi_factory)));
		// 关闭ALT+ENTER键切换全屏的功能，因为我们没有实现OnSize处理，所以先关闭
		CHECK_RESULT(m_dxgi_factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
	}

	m_hwnd = hwnd;
}

void D3D12RHI::CreateDevice()
{
	DXGI_ADAPTER_DESC1 stAdapterDesc = {};

	// 枚举高性能显卡
	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != m_dxgi_factory->EnumAdapterByGpuPreference(
		     adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&m_dxgi_adapter))
	     ; ++adapterIndex)
	{
		m_dxgi_adapter->GetDesc1(&stAdapterDesc);
		// 创建D3D12.1的设备
		if (SUCCEEDED(D3D12CreateDevice(m_dxgi_adapter.Get(), m_feature_level, IID_PPV_ARGS(&m_d3d12_device))))
		{
			break;
		}
	}
	// 把显卡名字复制出来
	StringCchPrintf(m_adapter_name, MAX_PATH, _T("%s"), stAdapterDesc.Description);

	//缓存一些常量
	m_srv_cbv_uav_descriptor_size = m_d3d12_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void D3D12RHI::CreateCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC queue_desc = {};
	queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queue_desc.Priority = 0;
	CHECK_RESULT(m_d3d12_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&m_cmd_queue)));


	CHECK_RESULT(
		m_d3d12_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmd_allocator)));

	// 创建图形命令列表
	CHECK_RESULT(
		m_d3d12_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmd_allocator.Get(), nullptr,
			IID_PPV_ARGS(&m_cmd_list)));
	CHECK_RESULT(m_cmd_list->Close());
}

void D3D12RHI::CreateSwapChain(const int width, const int height)
{
	m_view_port = {
		0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH
	};
	m_scissor_rect = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};

	DXGI_SWAP_CHAIN_DESC1 stSwapChainDesc = {};
	stSwapChainDesc.BufferCount = RHID3D12::FRAME_BACK_BUF_COUNT;
	stSwapChainDesc.Width = width;
	stSwapChainDesc.Height = height;
	stSwapChainDesc.Format = m_render_target_format;
	stSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	stSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	stSwapChainDesc.SampleDesc.Count = 1;


	CHECK_RESULT(
		m_dxgi_factory->CreateSwapChainForHwnd(m_cmd_queue.Get(),m_hwnd,&stSwapChainDesc, nullptr,nullptr,&
			m_swap_chain1));

	CHECK_RESULT(m_swap_chain1.As(&m_swap_chain4));

	m_frame_index = m_swap_chain4->GetCurrentBackBufferIndex();

	//创建RTV(渲染目标视图)描述符堆
	D3D12_DESCRIPTOR_HEAP_DESC stRTVHeapDesc = {};
	stRTVHeapDesc.NumDescriptors = RHID3D12::FRAME_BACK_BUF_COUNT;
	stRTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	stRTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	CHECK_RESULT(m_d3d12_device->CreateDescriptorHeap(&stRTVHeapDesc, IID_PPV_ARGS(&m_rtv_heap)));
	//得到每个描述符元素的大小
	m_rtv_descriptor_size = m_d3d12_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//创建RTV的描述符
	D3D12_CPU_DESCRIPTOR_HANDLE stRTVHandle = m_rtv_heap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < RHID3D12::FRAME_BACK_BUF_COUNT; ++i)
	{
		CHECK_RESULT(m_swap_chain4->GetBuffer(i, IID_PPV_ARGS(&m_render_targets[i])));
		m_d3d12_device->CreateRenderTargetView(m_render_targets[i].Get(), nullptr, stRTVHandle);
		stRTVHandle.ptr += m_rtv_descriptor_size;
	}
}

size_t D3D12RHI::CreateRootSignature()
{
	constexpr UINT root_signature_count = 2;
	// 第一个是全局const buffer
	D3D12_ROOT_PARAMETER root_parameter[root_signature_count];
	root_parameter[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	root_parameter[0].Descriptor.ShaderRegister = 0;
	root_parameter[0].Descriptor.RegisterSpace = 0;
	root_parameter[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	//第二个是各自的
	D3D12_ROOT_PARAMETER root_parameter1{};
	root_parameter[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	root_parameter[1].Descriptor.ShaderRegister = 1;
	root_parameter[1].Descriptor.RegisterSpace = 0;
	root_parameter[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;



	D3D12_ROOT_SIGNATURE_DESC root_signature_desc;
	root_signature_desc.NumParameters = root_signature_count;
	root_signature_desc.pParameters = root_parameter;
	root_signature_desc.NumStaticSamplers = 0;
	root_signature_desc.pStaticSamplers = nullptr;
	root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;


	ComPtr<ID3DBlob> signature_blob;
	ComPtr<ID3DBlob> error_blob;

	if (!SUCCEEDED(
		D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature_blob, &error_blob
		)))
	{
		CHAR psz_err_msg[MAX_PATH] = {};
		StringCchPrintfA(psz_err_msg, MAX_PATH, "\n%s\n", static_cast<CHAR*>(error_blob->GetBufferPointer()));
		::OutputDebugStringA(psz_err_msg);
	}
	ComPtr<ID3D12RootSignature> root_signature;
	CHECK_RESULT(
		m_d3d12_device->CreateRootSignature(0, signature_blob->GetBufferPointer(), signature_blob->GetBufferSize(),
			IID_PPV_ARGS(&root_signature)));
	m_root_signatures.emplace_back(root_signature);

	assert(!m_root_signatures.empty(), "");
	return m_root_signatures.size() - 1;
}

void D3D12RHI::CreatePipelineStateObject()
{
	ComPtr<ID3DBlob> blob_vertex_shader;
	ComPtr<ID3DBlob> blob_pixel_shader;
	ComPtr<ID3DBlob> error_blob;
#if defined(_DEBUG)
	//调试状态下，打开Shader编译的调试标志，不优化
	UINT nCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT nCompileFlags = 0;
#endif
	TCHAR psz_shader_file_name[MAX_PATH] = {};
	StringCchPrintf(psz_shader_file_name, MAX_PATH, _T("%s")/*, pszAppPath*/, RHID3D12::SHADER_PATH);

	if (!SUCCEEDED(
		D3DCompileFromFile(psz_shader_file_name, nullptr, nullptr, "VSMain", "vs_5_0", nCompileFlags, 0, &
			blob_vertex_shader, &error_blob)))
	{
		CHAR psz_err_msg[MAX_PATH] = {};
		StringCchPrintfA(psz_err_msg, MAX_PATH, "\n%s\n", static_cast<CHAR*>(error_blob->GetBufferPointer()));
		::OutputDebugStringA(psz_err_msg);
	}
	if (!SUCCEEDED(
		D3DCompileFromFile(psz_shader_file_name, nullptr, nullptr, "PSMain", "ps_5_0", nCompileFlags, 0, &
			blob_pixel_shader, &error_blob)))
	{
		CHAR psz_err_msg[MAX_PATH] = {};
		StringCchPrintfA(psz_err_msg, MAX_PATH, "\n%s\n", static_cast<CHAR*>(error_blob->GetBufferPointer()));
		::OutputDebugStringA(psz_err_msg);
	}

	m_shader_cache.emplace_back(blob_vertex_shader);
	m_shader_cache.emplace_back(blob_pixel_shader);


	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC input_element_descs[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC st_pso_desc = {};
	st_pso_desc.InputLayout = {input_element_descs, _countof(input_element_descs)};
	st_pso_desc.pRootSignature = m_root_signatures[0].Get();
	st_pso_desc.VS.pShaderBytecode = blob_vertex_shader->GetBufferPointer();
	st_pso_desc.VS.BytecodeLength = blob_vertex_shader->GetBufferSize();
	st_pso_desc.PS.pShaderBytecode = blob_pixel_shader->GetBufferPointer();
	st_pso_desc.PS.BytecodeLength = blob_pixel_shader->GetBufferSize();

	st_pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	st_pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	st_pso_desc.RasterizerState.ForcedSampleCount = 16;

	st_pso_desc.BlendState.AlphaToCoverageEnable = FALSE;
	st_pso_desc.BlendState.IndependentBlendEnable = FALSE;
	st_pso_desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	st_pso_desc.DepthStencilState.DepthEnable = FALSE;
	st_pso_desc.DepthStencilState.StencilEnable = FALSE;

	st_pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

	st_pso_desc.NumRenderTargets = 1;
	st_pso_desc.RTVFormats[0] = RHID3D12::RENDER_TARGET_FORMAT;

	st_pso_desc.SampleMask = UINT_MAX;
	st_pso_desc.SampleDesc.Count = 1;

	ComPtr<ID3D12PipelineState> pipeline_state_point;
	CHECK_RESULT(m_d3d12_device->CreateGraphicsPipelineState(&st_pso_desc, IID_PPV_ARGS(&pipeline_state_point)));
	m_pipeline_states.emplace_back(pipeline_state_point);

	ComPtr<ID3D12PipelineState> pipeline_state_line;
	st_pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	CHECK_RESULT(m_d3d12_device->CreateGraphicsPipelineState(&st_pso_desc, IID_PPV_ARGS(&pipeline_state_line)));
	m_pipeline_states.emplace_back(pipeline_state_line);
}

void D3D12RHI::CreateRenderEndFence()
{
	CHECK_RESULT(m_d3d12_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_render_end_fence.m_fence)));
	m_render_end_fence.m_fence_value = 1u;

	// 创建一个Event同步对象，用于等待围栏事件通知
	m_render_end_fence.m_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_render_end_fence.m_fence_event == nullptr)
	{
		CHECK_RESULT(HRESULT_FROM_WIN32(GetLastError()));
	}
	// 第一次循环的入口条件
	SetEvent(m_render_end_fence.m_fence_event);
}

DWORD D3D12RHI::WaitForFence() const
{
	return ::MsgWaitForMultipleObjects(1, &m_render_end_fence.m_fence_event, FALSE, INFINITE, QS_ALLINPUT);
}

void D3D12RHI::BeginFrame()
{
	//获取新的后缓冲序号，因为Present真正完成时后缓冲的序号就更新了
	m_frame_index = m_swap_chain4->GetCurrentBackBufferIndex();

	//命令分配器先Reset一下
	HRESULT h = m_cmd_allocator->Reset();
	if (!SUCCEEDED(h))
	{
		printf("[ERROR] Failed Reset CMD Allocator!\n");
	}
	// CHECK_RESULT(h);
	//Reset命令列表，并重新指定命令分配器和PSO对象
	h = m_cmd_list->Reset(m_cmd_allocator.Get(), nullptr);
	if (!SUCCEEDED(h))
	{
		printf("[ERROR] Failed Reset CMD List!\n");
	}
	// 通过资源屏障判定后缓冲已经切换完毕可以开始渲染了
	D3D12_RESOURCE_BARRIER resource_barrier;
	resource_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	resource_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	resource_barrier.Transition.pResource = m_render_targets[m_frame_index].Get();
	resource_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	resource_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	resource_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_cmd_list->ResourceBarrier(1, &resource_barrier);


	m_cmd_list->RSSetViewports(1, &m_view_port);
	m_cmd_list->RSSetScissorRects(1, &m_scissor_rect);


	auto rtv_handle = m_rtv_heap->GetCPUDescriptorHandleForHeapStart();
	rtv_handle.ptr += static_cast<unsigned long long>(m_frame_index) * m_rtv_descriptor_size;
	//设置渲染目标
	m_cmd_list->OMSetRenderTargets(1, &rtv_handle, FALSE, nullptr);

	m_cmd_list->ClearRenderTargetView(rtv_handle, RHID3D12::CLEAR_COLOR, 0, nullptr);
}

void D3D12RHI::RenderObject() const
{
	for (const SResource& resource : m_render_primitives)
	{
		m_cmd_list->SetGraphicsRootSignature(m_root_signatures[resource.m_root_signature_idx].Get());
		m_cmd_list->SetPipelineState(m_pipeline_states[resource.m_pso_idx].Get());

		m_cmd_list->IASetPrimitiveTopology(resource.m_primitive_topology);
		m_cmd_list->IASetVertexBuffers(0, 1, &resource.m_vertex_buffer_view);

		// ID3D12DescriptorHeap* desc_heaps[]= { m_descriptor_heap_global.Get(), m_descriptor_heap_local.Get() };
		// m_cmd_list->SetDescriptorHeaps(_countof(desc_heaps), desc_heaps);

		m_cmd_list->SetGraphicsRootConstantBufferView(0, m_constant_buffer_global->GetGPUVirtualAddress());//全局buffer

		m_cmd_list->SetGraphicsRootConstantBufferView(1, m_constant_buffer_local[resource.m_const_buffer_idx]->GetGPUVirtualAddress());//每个模型自己的buffer
		//Draw Call
		m_cmd_list->DrawInstanced(resource.m_vertex_count, 1, 0, 0u);
	}
}

void D3D12RHI::EndFrame()
{
	D3D12_RESOURCE_BARRIER stEndResBarrier = {};
	stEndResBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	stEndResBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	stEndResBarrier.Transition.pResource = m_render_targets[m_frame_index].Get();
	stEndResBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	stEndResBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	stEndResBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_cmd_list->ResourceBarrier(1, &stEndResBarrier);
	//关闭命令列表，可以去执行了
	// CHECK_RESULT();
	HRESULT h = m_cmd_list->Close();
	if (!SUCCEEDED(h))
	{
		printf("[ERROR] Failed Close CMD List!\n");
	}


	//执行命令列表
	ID3D12CommandList* ppCommandLists[] = {m_cmd_list.Get()};
	m_cmd_queue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	//提交画面
	CHECK_RESULT(m_swap_chain4->Present(1, 0));

	//开始同步GPU与CPU的执行，先记录围栏标记值
	const UINT64 n64CurrentFenceValue = m_render_end_fence.m_fence_value;
	CHECK_RESULT(m_cmd_queue->Signal(m_render_end_fence.m_fence.Get(), n64CurrentFenceValue));
	m_render_end_fence.m_fence_value++;
	CHECK_RESULT(
		m_render_end_fence.m_fence->SetEventOnCompletion(n64CurrentFenceValue, m_render_end_fence.m_fence_event));
}

TCHAR* D3D12RHI::GetCurrentAdapterName()
{
	return m_adapter_name;
}

void D3D12RHI::CreateResource(const std::vector<RHI_VERTEX>& vertex_vector, uint32_t pso_index, uint32_t root_signature_idx, D3D12_PRIMITIVE_TOPOLOGY type, uint32_t const_buffer_idx)
{
	SResource resource;

	const UINT64 vertex_buffer_size = vertex_vector.size() * sizeof(RHI_VERTEX);
	D3D12_HEAP_PROPERTIES heap_prop = {D3D12_HEAP_TYPE_UPLOAD};
	D3D12_RESOURCE_DESC resource_desc = {};
	resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resource_desc.Format = DXGI_FORMAT_UNKNOWN;
	resource_desc.Width = vertex_buffer_size;
	resource_desc.Height = 1;
	resource_desc.DepthOrArraySize = 1;
	resource_desc.MipLevels = 1;
	resource_desc.SampleDesc.Count = 1;
	resource_desc.SampleDesc.Quality = 0;

	CHECK_RESULT(m_d3d12_device->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&resource_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&resource.m_vertex_buffer)));

	UINT8* pVertexDataBegin = nullptr;
	D3D12_RANGE read_range = {0, 0};
	(resource.m_vertex_buffer->Map(
		0
		, &read_range
		, reinterpret_cast<void**>(&pVertexDataBegin)));

	memcpy(pVertexDataBegin, vertex_vector.data(), vertex_buffer_size);

	resource.m_vertex_buffer->Unmap(0, nullptr);

	resource.m_vertex_buffer_view.BufferLocation = resource.m_vertex_buffer->GetGPUVirtualAddress();
	resource.m_vertex_buffer_view.StrideInBytes = sizeof(RHI_VERTEX);
	resource.m_vertex_buffer_view.SizeInBytes = static_cast<UINT>(vertex_buffer_size);

	resource.m_vertex_count = vertex_vector.size();
	resource.m_pso_idx = pso_index;
	resource.m_root_signature_idx = root_signature_idx;
	resource.m_primitive_topology = type;
	resource.m_const_buffer_idx = const_buffer_idx;

	m_render_primitives.emplace_back(resource);
}

void D3D12RHI::CreateGlobalConstantBuffer()
{
	D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc{};
	descriptor_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptor_heap_desc.NumDescriptors = 1;
	descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	m_d3d12_device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&m_descriptor_heap_global));
	descriptor_heap_desc.NumDescriptors = 2;
	m_d3d12_device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&m_descriptor_heap_local));


	const D3D12_HEAP_PROPERTIES heap_prop = { D3D12_HEAP_TYPE_UPLOAD };
	D3D12_RESOURCE_DESC resource_desc = {};
	constexpr auto size_in_bytes = UINT_UPPER(sizeof(XMFLOAT4X4), 256);
	resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resource_desc.Format = DXGI_FORMAT_UNKNOWN;
	resource_desc.Width = size_in_bytes;
	resource_desc.Height = 1;
	resource_desc.DepthOrArraySize = 1;
	resource_desc.MipLevels = 1;
	resource_desc.SampleDesc.Count = 1;
	resource_desc.SampleDesc.Quality = 0;

	CHECK_RESULT(m_d3d12_device->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&resource_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_constant_buffer_global)));



	// UINT8* pVertexDataBegin = nullptr;
	// D3D12_RANGE read_range = { 0, 0 };
	// CHECK_RESULT(m_constant_buffer[0]->Map(0, &read_range, reinterpret_cast<void**>(&pVertexDataBegin)));

	auto handle = m_descriptor_heap_global->GetCPUDescriptorHandleForHeapStart();
	D3D12_CONSTANT_BUFFER_VIEW_DESC constant_buffer_view_desc{};
	constant_buffer_view_desc.SizeInBytes = size_in_bytes;
	constant_buffer_view_desc.BufferLocation = m_constant_buffer_global->GetGPUVirtualAddress();
	m_d3d12_device->CreateConstantBufferView(&constant_buffer_view_desc, handle);



}
void D3D12RHI::CreateLocalConstantBuffer(uint32_t buffer_count)
{
	const D3D12_HEAP_PROPERTIES heap_prop = { D3D12_HEAP_TYPE_UPLOAD };
	D3D12_RESOURCE_DESC resource_desc = {};
	constexpr auto size_in_bytes = UINT_UPPER(sizeof(XMFLOAT4X4), 256);
	resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resource_desc.Format = DXGI_FORMAT_UNKNOWN;
	resource_desc.Width = size_in_bytes;
	resource_desc.Height = 1;
	resource_desc.DepthOrArraySize = 1;
	resource_desc.MipLevels = 1;
	resource_desc.SampleDesc.Count = 1;
	resource_desc.SampleDesc.Quality = 0;

	auto handle = m_descriptor_heap_local->GetCPUDescriptorHandleForHeapStart();


	for (int i = 0; i < buffer_count; ++i)
	{
		ComPtr<ID3D12Resource2> const_buffer;

		CHECK_RESULT(m_d3d12_device->CreateCommittedResource(
			&heap_prop,
			D3D12_HEAP_FLAG_NONE,
			&resource_desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&const_buffer)));


		D3D12_CONSTANT_BUFFER_VIEW_DESC constant_buffer_view_desc{};
		constant_buffer_view_desc.SizeInBytes = size_in_bytes;
		constant_buffer_view_desc.BufferLocation = const_buffer->GetGPUVirtualAddress();

		m_d3d12_device->CreateConstantBufferView(&constant_buffer_view_desc, handle);
		handle.ptr += m_srv_cbv_uav_descriptor_size;

		m_constant_buffer_local.emplace_back(const_buffer);

	}
}

void D3D12RHI::UpdateLocalConstantBuffer(uint32_t idx, void* src_data, size_t size)
{
	if (!src_data)
	{
		return;
	}
	UINT8* data_begin = nullptr;
	D3D12_RANGE read_range = { 0, 0 };
	m_constant_buffer_local[idx]->Map(
		0
		, &read_range
		, reinterpret_cast<void**>(&data_begin));
	memcpy(data_begin, src_data, size);
	m_constant_buffer_local[idx]->Unmap(0, nullptr);
}
void D3D12RHI::UpdateGlobalConstantBuffer(void* src_data, size_t size)
{
	if (!src_data)
	{
		return;
	}
	UINT8* data_begin = nullptr;
	D3D12_RANGE read_range = { 0, 0 };
	m_constant_buffer_global->Map(
		0
		, &read_range
		, reinterpret_cast<void**>(&data_begin));
	memcpy(data_begin, src_data, size);
	m_constant_buffer_global->Unmap(0, nullptr);
}


void D3D12RHI::ClearTempResource()
{
	m_render_primitives.clear();
}
