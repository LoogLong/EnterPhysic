#pragma once
#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <vector>
#include <cassert>
#include <d3dcompiler.h>
#include <strsafe.h>
#include <tchar.h>

struct RHI_VERTEX;
using namespace Microsoft::WRL;

namespace RHID3D12
{
	constexpr DXGI_FORMAT RENDER_TARGET_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
	constexpr float CLEAR_COLOR[] = {0.1f, 0.1f, 0.1f, 1.0f};

	constexpr UINT DXGI_FACTORY_FLAGS = 0U;
	constexpr UINT FRAME_BACK_BUF_COUNT = 3u;

	constexpr TCHAR SHADER_PATH[MAX_PATH] = _T("Shader\\shaders.hlsl");
}

#define CHECK_RESULT(hr) if(!SUCCEEDED(hr)){ printf("[ERROR]:line:%d\n", __LINE__);}

struct SResource
{
	SResource() = default;
	~SResource() = default;
	SResource(SResource&) = default;
	SResource(SResource&&) = default;
	// SResource(SResource&& other){
	// 	m_vertex_buffer_view = other.m_vertex_buffer_view;
	// 	m_vertex_count = other.m_vertex_count;
	// 	m_vertex_buffer = other.m_vertex_buffer;
	// 	m_const_buffer = std::move(other.m_const_buffer);
	// 	m_pso_idx = other.m_pso_idx;
	// 	m_root_signature_idx = other.m_root_signature_idx;
	// 	m_primitive_topology = other.m_primitive_topology;
	// 	m_const_buffer_view = other.m_const_buffer_view;
	// };

	D3D12_VERTEX_BUFFER_VIEW m_vertex_buffer_view;
	uint32_t m_vertex_count;
	ComPtr<ID3D12Resource2> m_vertex_buffer;

	uint32_t m_pso_idx;
	uint32_t m_root_signature_idx;
	D3D12_PRIMITIVE_TOPOLOGY m_primitive_topology;

	uint32_t m_const_buffer_idx;
};

struct EventFence
{
	ComPtr<ID3D12Fence1> m_fence;
	UINT64 m_fence_value{0ui64};
	HANDLE m_fence_event{nullptr};

	EventFence() = default;
	~EventFence() = default;
	EventFence(EventFence&) = default;
	EventFence(EventFence&&) = default;
};


class D3D12RHI
{
public:
	D3D12RHI(const HWND& hwnd);

	// 枚举适配器，并选择合适的适配器来创建3D设备对象
	void CreateDevice();

	void CreateCommandQueue();

	void CreateSwapChain(const int width, const int height);


	size_t CreateRootSignature();

	void CreatePipelineStateObject();

	void CreateRenderEndFence();


	DWORD WaitForFence() const;


	void BeginFrame();

	void RenderObject() const;


	void EndFrame();


	TCHAR* GetCurrentAdapterName();

	void CreateResource(const std::vector<RHI_VERTEX>& vertex_vector, uint32_t pso_index, uint32_t root_signature_idx, D3D12_PRIMITIVE_TOPOLOGY type, uint32_t const_buffer_idx);
	void CreateGlobalConstantBuffer();
	void CreateLocalConstantBuffer(uint32_t buffer_count);
	void UpdateLocalConstantBuffer(uint32_t idx, void* src_data, size_t size);
	void UpdateGlobalConstantBuffer(void* src_data, size_t size);

	void ClearTempResource();
private:
	/*constance value*/
	D3D_FEATURE_LEVEL m_feature_level = D3D_FEATURE_LEVEL_12_1;
	UINT m_rtv_descriptor_size{0u};
	UINT m_dxgi_factory_flags{0u};
	TCHAR m_adapter_name[MAX_PATH]{};
	DXGI_FORMAT m_render_target_format = DXGI_FORMAT_R8G8B8A8_UNORM;

	D3D12_VIEWPORT m_view_port{};
	D3D12_RECT m_scissor_rect{};

	UINT m_frame_index{ 0 };

	HWND m_hwnd{};

	UINT m_srv_cbv_uav_descriptor_size{ 0 };

	ComPtr<IDXGIFactory7> m_dxgi_factory;
	ComPtr<IDXGIAdapter1> m_dxgi_adapter;
	ComPtr<ID3D12Device8> m_d3d12_device;
	ComPtr<ID3D12CommandQueue> m_cmd_queue;
	ComPtr<ID3D12CommandAllocator> m_cmd_allocator;
	ComPtr<ID3D12GraphicsCommandList6> m_cmd_list;
	
	ComPtr<IDXGISwapChain1> m_swap_chain1;
	ComPtr<IDXGISwapChain4> m_swap_chain4;
	ComPtr<ID3D12DescriptorHeap> m_rtv_heap;
	ComPtr<ID3D12DescriptorHeap> m_srv_cbv_heap;
	ComPtr<ID3D12Resource2> m_render_targets[RHID3D12::FRAME_BACK_BUF_COUNT];

	//constant buffer
	ComPtr<ID3D12Resource2> m_constant_buffer_global;
	ComPtr<ID3D12DescriptorHeap> m_descriptor_heap_global;
	std::vector <ComPtr<ID3D12Resource2>> m_constant_buffer_local;
	ComPtr<ID3D12DescriptorHeap> m_descriptor_heap_local;

	/*用于同步信息的fence*/
	EventFence m_render_end_fence;
	// std::vector<EventFence> m_event_fences;


	/*缓存shader和pso*/
	std::vector<ComPtr<ID3DBlob>> m_shader_cache;
	std::vector<ComPtr<ID3D12PipelineState>> m_pipeline_states;
	std::vector<ComPtr<ID3D12RootSignature>> m_root_signatures;

	/*缓存本帧用到的资源*/
	std::vector<SResource> m_render_primitives;


};


