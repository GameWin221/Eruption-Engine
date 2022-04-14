#pragma once

#ifndef EN_LIGHT_HPP
#define EN_LIGHT_HPP

class PointLight
{
public:
	PointLight(glm::vec3 position, glm::vec3 color, float radius);
	PointLight() {};
	~PointLight() {};

	bool m_Active = false;

	glm::vec3 m_Position = glm::vec3(0.0);
	glm::vec3 m_Color	 = glm::vec3(1.0);
	float m_Radius = 5.0f;

	struct Buffer
	{
		alignas(16) glm::vec3 position = glm::vec3(0.0);
		alignas(16) glm::vec3 color    = glm::vec3(1.0);
		alignas(4)  float     radius   = 5.0f;
	};
};
/*
class DirectionalLight
{
public:
	struct Buffer
	{
		glm::vec3 direction;
		glm::vec3 color;
	};
};
*/
#endif