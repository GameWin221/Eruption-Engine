#pragma once

#ifndef EN_POINTLIGHT_HPP
#define EN_POINTLIGHT_HPP

#include <Scene/SceneMember.hpp>

namespace en
{
	class PointLight : public SceneMember
	{
	friend class Scene;
	friend class Renderer;

	public:
		constexpr PointLight(glm::vec3 position, glm::vec3 color, float intensity, float radius, bool active)
			: m_Position(position), m_Color(color), m_Intensity(intensity), m_Radius(radius), m_Active(active), SceneMember{ SceneMemberType::PointLight } {};

		bool m_Active = true;

		glm::vec3 m_Position = glm::vec3(0.0);
		glm::vec3 m_Color	 = glm::vec3(1.0);

		float m_Intensity = 1.0f;
		float m_Radius	  = 5.0f;

		float m_ShadowBias = 0.02f;

		bool m_CastShadows = false;
		float m_ShadowSoftness = 1.0f;
		int m_PCFSampleRate = 1;

		void operator=(const PointLight& other)
		{
			m_Active   = other.m_Active;
			m_Position = other.m_Position;
			m_Color	   = other.m_Color;

			m_Intensity = other.m_Intensity;

			m_Radius = other.m_Radius;

			m_CastShadows = other.m_CastShadows;
			m_ShadowmapIndex = other.m_ShadowmapIndex;
			m_ShadowSoftness = other.m_ShadowSoftness;
			m_PCFSampleRate = other.m_PCFSampleRate;

			m_ShadowBias = other.m_ShadowBias;
		}

		struct Buffer
		{
			glm::vec3 position = glm::vec3(0.0f);
			float radius = 5.0f;

			glm::vec3 color = glm::vec3(1.0f);
			int shadowmapIndex = -1;

			float shadowSoftness = 1.0f;
			int pcfSampleRate = 1;
			float bias = 0.0f;
			float _padding0{};

			std::array<glm::mat4, 6> viewProj{};
		};

	private:
		int m_ShadowmapIndex = -1;
	};
}
#endif