#pragma once

#ifndef EN_DIRECTIONALLIGHT_HPP
#define EN_DIRECTIONALLIGHT_HPP

#include <Scene/SceneMember.hpp>

namespace en
{
	class DirectionalLight : public SceneMember
	{
	public:
		constexpr DirectionalLight(const glm::vec3& direction, const glm::vec3& color, const float& intensity, const bool& active)
			: m_Direction(direction), m_Color(color), m_Intensity(intensity), m_Active(active), SceneMember{ SceneMemberType::DirLight } {};

		bool m_Active = true;

		glm::vec3 m_Direction = glm::vec3(1.0, 0.0, 0.0);
		glm::vec3 m_Color	  = glm::vec3(1.0);

		float m_Intensity = 1.0f;

		void operator=(const DirectionalLight& other)
		{
			m_Active = other.m_Active;
			m_Direction = other.m_Direction;
			m_Color = other.m_Color;

			m_Intensity = other.m_Intensity;
		}

		struct Buffer
		{
			alignas(16) glm::vec3 direction = glm::vec3(1.0, 0.0, 0.0);
			alignas(16) glm::vec3 color = glm::vec3(1.0);
		};
	};
}
#endif