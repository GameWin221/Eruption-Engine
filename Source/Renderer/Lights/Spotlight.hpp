#pragma once

#ifndef EN_SPOTLIGHT_HPP
#define EN_SPOTLIGHT_HPP

namespace en
{
	class Spotlight
	{
	public:
		Spotlight(glm::vec3 position, glm::vec3 direction, glm::vec3 color, float innerCutoff, float outerCutoff, float range, float intensity);
		Spotlight() {};
		~Spotlight() {};

		bool m_Active = true;

		glm::vec3 m_Position = glm::vec3(0.0);
		glm::vec3 m_Direction = glm::vec3(1.0, 0.0, 0.0);
		glm::vec3 m_Color = glm::vec3(1.0);
		float m_Intensity = 1.0f;
		float m_Range = 8.0f;

		float m_InnerCutoff = 0.2f;
		float m_OuterCutoff = 0.4f;

		struct Buffer
		{
			glm::vec3 position = glm::vec3(0.0f);
			float innerCutoff = 0.2f;

			glm::vec3 direction = glm::vec3(0.0);
			float outerCutoff = 0.4f;

			glm::vec3 color = glm::vec3(1.0);
			float range = 8.0f;
		};
	};
}

#endif