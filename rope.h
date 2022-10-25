#ifndef ROPE_H
#define ROPE_H
#include <DirectXMath.h>
#include "mass.h"
#include "spring.h"
#include <vector>

struct RHI_VERTEX;
using namespace std;

using namespace DirectX;



class Rope
{
public:
	Rope(vector<SMass*>& masses, vector<SSpring*>& springs);

	Rope(DirectX::XMFLOAT4 start, DirectX::XMFLOAT4 end, int num_nodes, float node_mass, float k,
	     vector<int> pinned_nodes);

	void SimulateVerlet(float delta_t, DirectX::XMFLOAT4 gravity);
	void SimulateEuler(float delta_t, DirectX::XMFLOAT4 gravity);

	vector<SMass*> masses;
	vector<SSpring*> springs;
}; // struct Rope


class PhysicRope
{
public:

	PhysicRope();

	void UpdatePhysic(std::vector<RHI_VERTEX>& point_vertices, std::vector<RHI_VERTEX>& line_vertices);
private:
	Rope* rope_euler;
	Rope* rope_verlet;


};



#endif /* ROPE_H */
