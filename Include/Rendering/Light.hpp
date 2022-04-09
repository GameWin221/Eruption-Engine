#pragma once

#ifndef EN_LIGHT_HPP
#define EN_LIGHT_HPP

class PointLight
{
public:
	PointLight(glm::vec3 position, glm::vec3 color);
	PointLight() {};
	~PointLight() {};

	bool m_Active = false;

	glm::vec3 m_Position = glm::vec3(0.0);
	glm::vec3 m_Color	 = glm::vec3(1.0);

	struct Buffer
	{
		alignas(16) glm::vec3 position = glm::vec3(0.0);
		alignas(16) glm::vec3 color = glm::vec3(1.0);
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