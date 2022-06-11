#pragma once

#ifndef EN_POINTLIGHT_HPP
#define EN_POINTLIGHT_HPP
namespace en
{
	class PointLight
	{
	public:
		PointLight(glm::vec3 position, glm::vec3 color, float intensity, float radius, bool active);
		PointLight() {};
		~PointLight() {};

		bool m_Active = true;

		glm::vec3 m_Position = glm::vec3(0.0);
		glm::vec3 m_Color = glm::vec3(1.0);
		float m_Intensity = 1.0f;
		float m_Radius = 5.0f;

		struct Buffer
		{
			glm::vec3 position = glm::vec3(0.0);
			float radius = 5.0f;

			alignas(16) glm::vec3 color = glm::vec3(1.0);
		};
	};
}
#endif