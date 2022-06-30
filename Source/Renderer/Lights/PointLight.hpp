#pragma once

#ifndef EN_POINTLIGHT_HPP
#define EN_POINTLIGHT_HPP

#include <Scene/SceneMember.hpp>

namespace en
{
	class PointLight : public SceneMember
	{
	public:
		constexpr PointLight(const glm::vec3& position, const glm::vec3& color, const float& intensity, const float& radius, const bool& active)
			: m_Position(position), m_Color(color), m_Intensity(intensity), m_Radius(radius), m_Active(active), SceneMember{ SceneMemberType::PointLight } {};

		bool m_Active = true;

		glm::vec3 m_Position = glm::vec3(0.0);
		glm::vec3 m_Color	 = glm::vec3(1.0);

		float m_Intensity = 1.0f;
		float m_Radius	  = 5.0f;

		void operator=(const PointLight& other)
		{
			m_Active   = other.m_Active;
			m_Position = other.m_Position;
			m_Color	   = other.m_Color;

			m_Intensity = other.m_Intensity;

			m_Radius = other.m_Radius;
		}

		struct Buffer
		{
			glm::vec3 position = glm::vec3(0.0);

			float radius = 5.0f;

			alignas(16) glm::vec3 color = glm::vec3(1.0);
		};
	};
}
#endif