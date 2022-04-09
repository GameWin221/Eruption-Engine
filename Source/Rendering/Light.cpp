#include "EnPch.hpp"
#include "Rendering/Light.hpp"

PointLight::PointLight(glm::vec3 position, glm::vec3 color)
{
	m_Position = position;
	m_Color = color;
}
