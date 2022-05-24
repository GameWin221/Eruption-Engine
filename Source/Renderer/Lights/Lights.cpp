#include <Core/EnPch.hpp>
#include "PointLight.hpp"
#include "Spotlight.hpp"
#include "DirectionalLight.hpp"

namespace en
{
	PointLight::PointLight(glm::vec3 position, glm::vec3 color, float intensity, float radius) : m_Position(position), m_Color(color), m_Intensity(intensity), m_Radius(radius) {}
	
	DirectionalLight::DirectionalLight(glm::vec3 direction, glm::vec3 color, float intensity) : m_Direction(direction), m_Color(color), m_Intensity(intensity) {}
	
	Spotlight::Spotlight(glm::vec3 position, glm::vec3 direction, glm::vec3 color, float innerCutoff, float outerCutoff, float range, float intensity) : m_Position(position), m_Direction(direction), m_Color(color), m_InnerCutoff(innerCutoff), m_OuterCutoff(outerCutoff), m_Range(range), m_Intensity(intensity) {}
}