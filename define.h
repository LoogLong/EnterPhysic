#pragma once

#include <DirectXMath.h>

using namespace DirectX;

struct RHI_VERTEX
{
	XMFLOAT4 m_vtPos;
	XMFLOAT4 m_vtColor;

	RHI_VERTEX(XMFLOAT4 pos, XMFLOAT4 color) : m_vtPos(pos), m_vtColor(color)
	{
	}
};
#define UINT_UPPER(A,B) ((UINT)(((A)+((B)-1))&~(B - 1)))
namespace Config
{
	constexpr float mass = 1.f;
	constexpr float ks = 80.f;
	constexpr DirectX::XMFLOAT4 gravity = { 0, -9.8f, 0, 0 };
	constexpr uint32_t steps_per_frame = 64;
	constexpr float DUMP_FACTOR = 0.01f;

};
