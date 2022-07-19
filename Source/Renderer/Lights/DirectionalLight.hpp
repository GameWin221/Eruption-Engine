#pragma once

#ifndef EN_DIRECTIONALLIGHT_HPP
#define EN_DIRECTIONALLIGHT_HPP

#include <Scene/SceneMember.hpp>
#include <Renderer/Image.hpp>

namespace en
{
	class DirectionalLight : public SceneMember
	{
	public:
		DirectionalLight(const glm::vec3& direction, const glm::vec3& color, const float& intensity, const bool& active)
			: m_Direction(direction), m_Color(color), m_Intensity(intensity), m_Active(active), SceneMember{ SceneMemberType::DirLight } {};

		bool m_Active = true;

		glm::vec3 m_Direction = glm::vec3(0.0, 1.0, 0.0);
		glm::vec3 m_Color	  = glm::vec3(1.0);

		float m_Intensity = 1.0f;

		bool m_CastShadows = false;
		int m_ShadowmapIndex = -1;
		float m_ShadowSoftness = 1.0f;
		int m_PCFSampleRate = 1;

		void operator=(const DirectionalLight& other)
		{
			m_Active = other.m_Active;
			m_Direction = other.m_Direction;
			m_Color = other.m_Color;

			m_Intensity = other.m_Intensity;

			m_CastShadows = other.m_CastShadows;
			m_ShadowmapIndex = other.m_ShadowmapIndex;
			m_ShadowSoftness = other.m_ShadowSoftness;
			m_PCFSampleRate = other.m_PCFSampleRate;
		}

		struct Buffer
		{
			glm::vec3 direction = glm::vec3(0.0);
			int shadowmapIndex = -1;
			glm::vec3 color = glm::vec3(1.0);

			float shadowSoftness = 1.0;
			int pcfSampleRate = 1;

			float dummy0;
			float dummy1;
			float dummy2;

			glm::mat4 lightMat = glm::mat4(1.0);
		};
	};
}
#endif