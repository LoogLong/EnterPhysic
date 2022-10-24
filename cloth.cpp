#include "cloth.h"
#include <random>


CCloth::CCloth(float height, int num_height_node, int num_width_node, float spring_k, float mass, std::vector<XMFLOAT4> pinned_nodes)
{
	// 构建cloth 网格
	assert(pinned_nodes.size() == 2, "");

	const XMVECTOR left_point = XMLoadFloat4(&pinned_nodes[0]);
	const XMVECTOR right_point = XMLoadFloat4(&pinned_nodes[1]);

	const float delta_height = - height / static_cast<float>(num_height_node);

	const XMVECTOR left_to_right = right_point - left_point;
	const XMVECTOR delta_direction = left_to_right / num_width_node;

	std::vector<XMVECTOR> first_line;
	first_line.reserve(num_width_node+1);

	
	for (int i = 0; i <= num_width_node; ++i)
	{
		first_line.emplace_back(left_point+delta_direction*i);
	}

	m_mass.reserve((num_width_node + 1) * (num_height_node + 1));

	//构建质点
	for (int j = 0; j <= num_height_node; ++j)
	{
		for (int i = 0; i <= num_width_node; ++i)
		{
			XMFLOAT4 position{};
			const XMVECTOR cur_pos = first_line[i] + XMVectorSet(0, j * delta_height, 0, 0);
			XMStoreFloat4(&position, cur_pos);
			m_mass.emplace_back(position, mass, false);
		}
	}

	//构建弹簧
	
	for (int j = 0; j <= num_height_node; ++j)
	{
		for (int i = 0; i < num_width_node; ++i)
		{
			int idx = j * (num_width_node + 1) + i;
			m_springs.emplace_back(&m_mass[idx], &m_mass[idx + 1], spring_k);/*1.横向弹簧*/
			if (i != num_width_node-1)
			{
				m_springs.emplace_back(&m_mass[idx], &m_mass[idx + 2], spring_k);/*1.横向跨质点弹簧*/
			}
		}
	}
	
	for (int j = 0; j < num_height_node; ++j)
	{
		for (int i = 0; i <= num_width_node; ++i)
		{
			int idx = j * (num_width_node + 1) + i;
			m_springs.emplace_back(&m_mass[idx], &m_mass[idx + num_width_node + 1], spring_k);/*2.竖向弹簧*/
			if (j != num_height_node - 1)
			{
				m_springs.emplace_back(&m_mass[idx], &m_mass[idx + num_width_node + 1], spring_k);/*1.横向跨质点弹簧*/
			}
		}
	}

	for (int j = 0; j < num_height_node; ++j)
	{
		for (int i = 0; i < num_width_node; ++i)
		{
			int idx = j * (num_width_node + 1) + i;
			m_springs.emplace_back(&m_mass[idx], &m_mass[idx + num_width_node + 2], spring_k);/*3.撇向弹簧*/
			m_springs.emplace_back(&m_mass[idx+1], &m_mass[idx + num_width_node + 1], spring_k);/*4.捺向弹簧*/
		}
	}


	m_width_vertices = num_width_node + 1;
	m_height_vertices = num_height_node + 1;

	//设置被固定的点
	// for (int i = 0; i <= num_width_node; ++i)
	// {
	// 	m_mass[i].pinned = true;
	// }
	m_mass[0].pinned = true;
	m_mass[num_width_node].pinned = true;
}


std::random_device g_rd;
std::default_random_engine g_eng(g_rd());
std::uniform_int_distribution<int> g_distr(0, 100);


void CCloth::SimulateVerlet(float delta_t, DirectX::XMFLOAT4 gravity)
{
	for (auto& m : m_mass)
	{
		if (!m.pinned)
		{
			XMVECTOR last_position = XMLoadFloat4(&m.last_position);
			XMVECTOR this_position = XMLoadFloat4(&m.position);

			const XMVECTOR gravity_vector = XMLoadFloat4(&gravity);

			int random_val = g_distr(g_eng);
			/*Wind Force*/
			const XMVECTOR wind_force = XMVectorSet(random_val, 0, random_val, 0);

			XMVECTOR total_acceleration = wind_force / m.mass + gravity_vector;

			this_position = this_position + (this_position - last_position) * (1.0f - Config::DUMP_FACTOR) + total_acceleration * delta_t * delta_t;


			m.last_position = m.position;
			XMStoreFloat4(&m.position, this_position);
		}
	}

	for (int i = 0; i < 1; ++i)
	{
		for (auto& s : m_springs)
		{
			/*2222*/
			XMVECTOR position_1 = XMLoadFloat4(&s.m1->position);
			XMVECTOR position_2 = XMLoadFloat4(&s.m2->position);
			XMVECTOR direction = position_1 - position_2;
			const float current_length = XMVectorGetX(XMVector3Length(direction));
			direction = XMVector3Normalize(direction);

			const XMVECTOR move = direction * (current_length - s.rest_length) * 0.5f;


			position_1 -= move;
			position_2 += move;

			if (!s.m1->pinned)
			{
				XMStoreFloat4(&s.m1->position, position_1);
			}
			if (!s.m2->pinned)
			{
				XMStoreFloat4(&s.m2->position, position_2);
			}
		}
	}
}

void CCloth::SimulateEuler(float delta_t, DirectX::XMFLOAT4 gravity)
{
	for (auto& s : m_springs)
	{
		const XMVECTOR position_1 = XMLoadFloat4(&s.m1->position);
		const XMVECTOR position_2 = XMLoadFloat4(&s.m2->position);
		XMVECTOR direction = position_1 - position_2;
		const float current_length = XMVectorGetX(XMVector3Length(direction));
		direction = XMVector3Normalize(direction);

		const auto force = s.k * direction * (current_length - s.rest_length);

		XMVECTOR total_force = XMLoadFloat4(&s.m1->forces) - force;
		XMStoreFloat4(&s.m1->forces, total_force);

		total_force = XMLoadFloat4(&s.m2->forces) + force;
		XMStoreFloat4(&s.m2->forces, total_force);
	}

	for (auto& m : m_mass)
	{
		if (!m.pinned)
		{
			// TODO (Part 2): Add the force due to gravity, then compute the new velocity and position
			const XMVECTOR gravity_vector = XMLoadFloat4(&gravity);
			XMVECTOR gravity_force = m.mass * gravity_vector;

			XMVECTOR velocity = XMLoadFloat4(&m.velocity);


			int random_val = g_distr(g_eng);
			/*Wind Force*/
			const XMVECTOR wind_force = XMVectorSet(random_val, 0, random_val, 0);
			/*阻力*/
			XMVECTOR dump_force = -velocity * (1.0f - Config::DUMP_FACTOR);

			XMVECTOR total_force = XMLoadFloat4(&m.forces) + gravity_force + wind_force + dump_force;
			XMStoreFloat4(&m.forces, total_force);

			// TODO (Part 2): Add global damping

			XMVECTOR acceleration = total_force / m.mass;

			velocity = velocity + acceleration * delta_t; //半隐式semi-implicit euler

			XMVECTOR position = XMLoadFloat4(&m.position);
			position = position + velocity * delta_t;
			XMStoreFloat4(&m.position, position);

			//velocity = velocity + acceleration * delta_t;//显示explicit euler
			XMStoreFloat4(&m.velocity, velocity);
		}

		// Reset all forces on each mass
		m.forces = { 0, 0, 0, 0 };
	}
}

void CCloth::GetRenderResource(std::vector<RHI_VERTEX>& point_vertices, std::vector<RHI_VERTEX>& line_vertices)
{
	XMFLOAT4 point_color = { 1.f, 1.f, 1.0f, 1.0f }; //White
	XMFLOAT4 line_color = { 1.f, 1.f, 0.0f, 1.0f }; //White

	point_vertices.reserve(m_mass.size());
	for (const auto & mass : m_mass)
	{
		point_vertices.emplace_back(mass.position, point_color);
	}

	/*1.横向*/
	for (int j = 0; j < m_height_vertices; ++j)
	{
		for (int i = 0; i < m_width_vertices-1; ++i)
		{
			const int idx = j * m_width_vertices + i;
			line_vertices.emplace_back(m_mass[idx].position, line_color);
			line_vertices.emplace_back(m_mass[idx + 1].position, line_color);
		}
	}
	/*2.竖向*/
	for (int j = 0; j < m_height_vertices-1; ++j)
	{
		for (int i = 0; i < m_width_vertices; ++i)
		{
			const int idx = j * m_width_vertices + i;
			line_vertices.emplace_back(m_mass[idx].position, line_color);
			line_vertices.emplace_back(m_mass[idx + m_width_vertices].position, line_color);
		}
	}
}

PhysicCloth::PhysicCloth()
{
	std::vector<XMFLOAT4> pined_node;
	pined_node.emplace_back(-200, 200, 400, 1.0f);
	pined_node.emplace_back(200, 200, 0, 1.0f);
	m_cloth = new CCloth(400.f, 50, 50, Config::ks, Config::mass, pined_node);
}
void PhysicCloth::UpdatePhysic(std::vector<RHI_VERTEX>& point_vertices, std::vector<RHI_VERTEX>& line_vertices)
{
	const float delta_time = static_cast<float>(1) / static_cast<float>(Config::steps_per_frame);
	// for (uint32_t i = 0; i < Config::steps_per_frame; i++)
	// {
	// 	m_cloth->SimulateEuler(delta_time, Config::gravity);
	// }
	// m_cloth->SimulateEuler(delta_time, Config::gravity);
	m_cloth->SimulateVerlet(delta_time, Config::gravity);
	// Rendering ropes
	CCloth* cloth = m_cloth;


	// std::vector<RHI_VERTEX> point_vertices;
	// std::vector<RHI_VERTEX> line_vertices;
	m_cloth->GetRenderResource(point_vertices, line_vertices);
	// for (int i = 0; i < 2; i++)
	// {
	// 	XMFLOAT4 vertex_color = { 1.f, 1.f, 1.0f, 1.0f }; //White
	// 	
	//
	// 	for (auto& m : cloth->m_mass)
	// 	{
	// 		point_vertices.emplace_back(m.position, vertex_color);
	// 	}
	//
	//
	// 	for (auto& s : cloth->m_springs)
	// 	{
	// 		line_vertices.emplace_back(s.m1->position, vertex_color);
	// 		line_vertices.emplace_back(s.m2->position, vertex_color);
	// 	}
	// }
}