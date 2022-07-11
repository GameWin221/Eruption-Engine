#pragma once

#ifndef EN_SPOTLIGHT_HPP
#define EN_SPOTLIGHT_HPP

#include <Scene/SceneMember.hpp>

namespace en
{
	class SpotLight : public SceneMember
	{
	public:
		constexpr SpotLight(const glm::vec3& position, const glm::vec3& direction, const glm::vec3& color, const float& innerCutoff, const float& outerCutoff, const float& range, const float& intensity, const bool& active)
			: m_Position(position), m_Direction(direction), m_Color(color), m_InnerCutoff(innerCutoff), m_OuterCutoff(outerCutoff), m_Range(range), m_Intensity(intensity), m_Active(active), SceneMember{ SceneMemberType::SpotLight } {};

		bool m_Active = true;

		glm::vec3 m_Position = glm::vec3(0.0);
		glm::vec3 m_Direction = glm::vec3(1.0, 0.0, 0.0);
		glm::vec3 m_Color = glm::vec3(1.0);

		float m_Intensity = 1.0f;
		float m_Range = 8.0f;

		float m_InnerCutoff = 0.2f;
		float m_OuterCutoff = 0.4f;

		bool m_CastShadows = false;
		int m_ShadowmapIndex = -1;

		void operator=(const SpotLight& other)
		{
			m_Active	= other.m_Active;
			m_Position  = other.m_Position;
			m_Direction = other.m_Direction ;
			m_Color		= other.m_Color;

			m_Intensity = other.m_Intensity;
			m_Range		= other.m_Range;

			m_InnerCutoff = other.m_InnerCutoff;
			m_OuterCutoff = other.m_OuterCutoff;

			m_CastShadows = other.m_CastShadows;
			m_ShadowmapIndex = other.m_ShadowmapIndex;
		}

		struct Buffer
		{
			glm::vec3 position = glm::vec3(0.0f);
			float innerCutoff = 0.2f;

			glm::vec3 direction = glm::vec3(0.0);
			float outerCutoff = 0.4f;

			glm::vec3 color = glm::vec3(1.0);
			float range = 8.0f;

			glm::mat4 lightMat = glm::mat4(1.0);

			int shadowmapIndex = -1;
			float dummy0;
			float dummy1;
			float dummy2;
		};
	};
}

#endif