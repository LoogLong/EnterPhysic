#ifndef SPRING_H
#define SPRING_H

#include "mass.h"

using namespace std;

struct SSpring
{
	SSpring(SMass* a, SMass* b, float k)
		: k(k), m1(a), m2(b)
	{
		DirectX::XMVECTOR a_pos = DirectX::XMLoadFloat4(&a->position);
		DirectX::XMVECTOR b_pos = DirectX::XMLoadFloat4(&b->position);

		DirectX::XMVECTOR delta = DirectX::XMVectorSubtract(a_pos, b_pos);
		auto result = DirectX::XMVector3Length(delta);
		rest_length = DirectX::XMVectorGetX(result);
	}

	float k;
	double rest_length;

	SMass* m1;
	SMass* m2;
}; // struct SSpring
#endif /* SPRING_H */
