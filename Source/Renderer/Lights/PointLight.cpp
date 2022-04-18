#include <Core/EnPch.hpp>
#include "PointLight.hpp"

PointLight::PointLight(glm::vec3 position, glm::vec3 color, float intensity, float radius) 
	: m_Position(position),  m_Color(color), m_Intensity(intensity), m_Radius(radius) {}