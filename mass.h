#ifndef MASS_H
#define MASS_H

#include <DirectXMath.h>

struct SMass
{
	SMass(DirectX::XMFLOAT4& position, float mass, bool pinned)
		: mass(mass), pinned(pinned), start_position(position),
		  position(position), last_position(position)
	{
	}
	float mass;
	bool pinned;

	DirectX::XMFLOAT4 start_position;
	DirectX::XMFLOAT4 position;

	// explicit Verlet integration

	DirectX::XMFLOAT4 last_position;

	// explicit Euler integration

	DirectX::XMFLOAT4 velocity{ 0,0,0,0 };
	DirectX::XMFLOAT4 forces{ 0,0,0,0 };
};

#endif /* MASS_H */
