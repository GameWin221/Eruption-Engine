#pragma once

#ifndef EN_DIRECTIONALLIGHT_HPP
#define EN_DIRECTIONALLIGHT_HPP

namespace en
{
	class DirectionalLight
	{
	public:
		DirectionalLight(glm::vec3 direction, glm::vec3 color, float intensity, bool active);
		DirectionalLight() {};
		~DirectionalLight() {};

		bool m_Active = true;

		glm::vec3 m_Direction = glm::vec3(1.0, 0.0, 0.0);
		glm::vec3 m_Color = glm::vec3(1.0);
		float m_Intensity = 1.0f;

		struct Buffer
		{
			alignas(16) glm::vec3 direction = glm::vec3(1.0, 0.0, 0.0);
			alignas(16) glm::vec3 color = glm::vec3(1.0);
		};
	};
}
#endif