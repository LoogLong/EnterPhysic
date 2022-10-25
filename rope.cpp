#include <iostream>
#include <vector>
#include "mass.h"
#include "rope.h"
#include "spring.h"
#include "define.h"

using namespace DirectX;

Rope::Rope(vector<SMass*>& masses, vector<SSpring*>& springs): masses(masses), springs(springs)
{
}

Rope::Rope(DirectX::XMFLOAT4 start, DirectX::XMFLOAT4 end, int num_nodes, float node_mass, float k,
           vector<int> pinned_nodes)
{
	// TODO (Part 1): Create a rope starting at `start`, ending at `end`, and containing `num_nodes` nodes.

	//        Comment-in this part when you implement the constructor
	//        for (auto &i : pinned_nodes) {
	//            masses[i]->pinned = true;
	//        }

	const XMVECTOR start_point = XMLoadFloat4(&start);
	const XMVECTOR end_point = XMLoadFloat4(&end);

	const XMVECTOR line_direction = XMVectorSubtract(end_point, start_point);

	// const XMVECTOR length_vector = XMVector3Length(line_direction);

	// float rope_length = XMVectorGetX(length_vector);
	const float inverse_node_num = 1.f / static_cast<float>(num_nodes);
	for (int i = 0; i <= num_nodes; ++i)
	{
		const float percent = static_cast<float>(i) * inverse_node_num;
		const XMVECTOR particle_position = start_point + line_direction * percent;

		XMFLOAT4 temp;
		XMStoreFloat4(&temp, particle_position);
		masses.emplace_back(new SMass(temp, node_mass * (i + 1), false));
	}
	for (int i = 0; i < num_nodes; ++i)
	{
		springs.emplace_back(new SSpring(masses[i], masses[i + 1], k));
	}

	for (const auto& i : pinned_nodes)
	{
		masses[i]->pinned = true;
	}
}

void Rope::SimulateEuler(float delta_t, DirectX::XMFLOAT4 gravity)
{
	for (auto& s : springs)
	{
		const XMVECTOR position_1 = XMLoadFloat4(&s->m1->position);
		const XMVECTOR position_2 = XMLoadFloat4(&s->m2->position);
		XMVECTOR direction = position_1 - position_2;
		const float current_length = XMVectorGetX(XMVector3Length(direction));
		direction = XMVector3Normalize(direction);

		const auto force = s->k * direction * (current_length - s->rest_length);

		XMVECTOR total_force = XMLoadFloat4(&s->m1->forces) - force;
		XMStoreFloat4(&s->m1->forces, total_force);

		total_force = XMLoadFloat4(&s->m2->forces) + force;
		XMStoreFloat4(&s->m2->forces, total_force);
	}

	for (auto& m : masses)
	{
		if (!m->pinned)
		{
			// TODO (Part 2): Add the force due to gravity, then compute the new velocity and position
			const XMVECTOR gravity_vector = XMLoadFloat4(&gravity);
			XMVECTOR gravity_force = m->mass * gravity_vector;


			// TODO (Part 2): Add global damping

			XMVECTOR velocity = XMLoadFloat4(&m->velocity);

			XMVECTOR dump_force = - velocity * (1.0f - Config::DUMP_FACTOR);

			XMVECTOR total_force = XMLoadFloat4(&m->forces) + gravity_force + dump_force;

			XMVECTOR acceleration = total_force / m->mass;


			XMVECTOR new_velocity = velocity + acceleration * delta_t; //半隐式semi-implicit euler

			XMVECTOR position = XMLoadFloat4(&m->position);
			position = position + new_velocity * delta_t;
			XMStoreFloat4(&m->position, position);

			//velocity = velocity + acceleration * delta_t;//显示explicit euler
			XMStoreFloat4(&m->velocity, new_velocity);
		}

		// Reset all forces on each mass
		m->forces = {0, 0, 0, 0};
	}
}

void Rope::SimulateVerlet(float delta_t, DirectX::XMFLOAT4 gravity)
{
	// for (auto& s : springs)
	// {
	// TODO (Part 3): Simulate one timestep of the rope using explicit Verlet （solving constraints)
	/*1111111111*/
	// const XMVECTOR position_1 = XMLoadFloat4(&s->m1->position);
	// const XMVECTOR position_2 = XMLoadFloat4(&s->m2->position);
	// XMVECTOR direction = position_1 - position_2;
	// const float current_length = XMVectorGetX(XMVector3Length(direction));
	// direction = XMVector3Normalize(direction);
	//
	// const auto force = s->k * direction * (current_length - s->rest_length);
	//
	// XMVECTOR total_force = XMLoadFloat4(&s->m1->forces) - force;
	// XMStoreFloat4(&s->m1->forces, total_force);
	//
	// total_force = XMLoadFloat4(&s->m2->forces) + force;
	// XMStoreFloat4(&s->m2->forces, total_force);
	// }
	// for (auto& m : masses)
	// {
	// 	if (!m->pinned)
	// 	{
	// 		XMFLOAT4 temp_position = m->position;
	// 		// TODO (Part 3.1): Set the new position of the rope mass
	// 		XMVECTOR last_position = XMLoadFloat4(&m->last_position);
	// 		XMVECTOR this_position = XMLoadFloat4(&m->position);
	//
	// 		const XMVECTOR gravity_vector = XMLoadFloat4(&gravity);
	// 		XMVECTOR gravity_force = m->mass * gravity_vector;
	// 		XMVECTOR total_force = XMLoadFloat4(&m->forces) + gravity_force;
	//
	// 		// TODO (Part 4): Add global Verlet damping
	//
	//
	// 		XMVECTOR acceleration = total_force / m->mass;
	// 		this_position = this_position + (this_position - last_position) * 0.5f + acceleration * delta_t * delta_t;
	//
	//
	// 		m->last_position = m->position;
	// 		XMStoreFloat4(&m->position, this_position);
	//
	//
	//
	// 	}
	// 	// Reset all forces on each mass
	// 	m->forces = { 0, 0, 0, 0 };
	// }


	/*2222222222*/

	for (auto& m : masses)
	{
		if (!m->pinned)
		{
			XMVECTOR last_position = XMLoadFloat4(&m->last_position);
			XMVECTOR this_position = XMLoadFloat4(&m->position);

			const XMVECTOR gravity_vector = XMLoadFloat4(&gravity);
			this_position = this_position + (this_position - last_position) * (1.0f - Config::DUMP_FACTOR) + gravity_vector * delta_t * delta_t;


			m->last_position = m->position;
			XMStoreFloat4(&m->position, this_position);
		}
	}

	for (int i = 0; i < 64; ++i)
	{
		for (auto& s : springs)
		{
			// TODO (Part 3): Simulate one timestep of the rope using explicit Verlet （solving constraints)
			/*2222*/
			XMVECTOR position_1 = XMLoadFloat4(&s->m1->position);
			XMVECTOR position_2 = XMLoadFloat4(&s->m2->position);
			XMVECTOR direction = position_1 - position_2;
			const float current_length = XMVectorGetX(XMVector3Length(direction));
			direction = XMVector3Normalize(direction);

			const XMVECTOR move = direction * (current_length - s->rest_length) * 0.5f;


			position_1 -= move;
			position_2 += move;

			if (!s->m1->pinned)
			{
				XMStoreFloat4(&s->m1->position, position_1);
			}
			if (!s->m2->pinned)
			{
				XMStoreFloat4(&s->m2->position, position_2);
			}
		}
	}
}


PhysicRope::PhysicRope()
{
	/*Init Rope*/
	rope_euler = new Rope({0, 200, 0, 1.0f}, {-400, 200, 0, 1.0f}, 8, Config::mass, Config::ks, {0});
	rope_verlet = new Rope({0, 200, 0, 1.0f}, {-400, 200, 0, 1.0f}, 8, Config::mass, Config::ks, {0});
}

void PhysicRope::UpdatePhysic(std::vector<RHI_VERTEX>& point_vertices, std::vector<RHI_VERTEX>& line_vertices)
{
	const float delta_time = static_cast<float>(1) / static_cast<float>(Config::steps_per_frame);
	// for (uint32_t i = 0; i < Config::steps_per_frame; i++)
	// {
	// 	rope_euler->SimulateEuler(delta_time, Config::gravity);
	// 	rope_verlet->SimulateVerlet(delta_time, Config::gravity);
	// }
	rope_euler->SimulateEuler(delta_time, Config::gravity);
	rope_verlet->SimulateVerlet(delta_time, Config::gravity);

	// Rendering ropes
	Rope* rope;


	// std::vector<RHI_VERTEX> point_vertices;
	// std::vector<RHI_VERTEX> line_vertices;

	for (int i = 0; i < 2; i++)
	{
		XMFLOAT4 vertex_color;
		if (i == 0)
		{
			vertex_color = {1.f, 0.1f, 0.1f, 1.f}; //Red
			rope = rope_euler;
		}
		else
		{
			vertex_color = {1.f, 1.f, 1.0f, 1.0f}; //Blue
			rope = rope_verlet;
		}

		for (auto& m : rope->masses)
		{
			point_vertices.emplace_back(m->position, vertex_color);
		}


		for (auto& s : rope->springs)
		{
			line_vertices.emplace_back(s->m1->position, vertex_color);
			line_vertices.emplace_back(s->m2->position, vertex_color);
		}
	}
}
