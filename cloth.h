#pragma once
#include <vector>
#include "mass.h"
#include "spring.h"
#include "define.h"
class CCloth
{
public:
	CCloth(float height, int num_height_node, int num_width_node, float spring_k, float mass, std::vector<XMFLOAT4> pinned_nodes);
	~CCloth() = default;


	void SimulateVerlet(float delta_t, DirectX::XMFLOAT4 gravity);
	void SimulateEuler(float delta_t, DirectX::XMFLOAT4 gravity);


	void SimulateImplicitEuler(float delta_t, DirectX::XMFLOAT4 gravity);
	void SimulatePBDJacobi(float delta_t, DirectX::XMFLOAT4 gravity);
	void SimulatePBDJacobiGames(float delta_t, DirectX::XMFLOAT4 gravity);

	void GetRenderResource(std::vector<RHI_VERTEX>& point_vertices, std::vector<RHI_VERTEX>& line_vertices, XMFLOAT4 color);

	std::vector<SMass> m_mass{};
	std::vector<SSpring> m_springs{};

	int m_width_vertices{ 0 };
	int m_height_vertices{ 0 };

};

class PhysicCloth
{
public:
	PhysicCloth();

	void UpdatePhysic(std::vector<RHI_VERTEX>& point_vertices, std::vector<RHI_VERTEX>& line_vertices) const;
private:
	CCloth* m_cloth{ nullptr };
	CCloth* m_cloth_euler{ nullptr };
};
