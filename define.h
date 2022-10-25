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

namespace Config
{
	constexpr float mass = 1.f;
	constexpr float ks = 1000.f;
	constexpr DirectX::XMFLOAT4 gravity = { 0, -100, 0, 0 };
	constexpr uint32_t steps_per_frame = 64;
	constexpr float DUMP_FACTOR = 0.0001f;

};
