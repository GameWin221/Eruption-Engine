#include <Core/EnPch.hpp>
#include "PointLight.hpp"
#include "Spotlight.hpp"
#include "DirectionalLight.hpp"

namespace en
{
	PointLight::PointLight(glm::vec3 position, glm::vec3 color, float intensity, float radius, bool active) : m_Position(position), m_Color(color), m_Intensity(intensity), m_Radius(radius), m_Active(active) {}
	
	DirectionalLight::DirectionalLight(glm::vec3 direction, glm::vec3 color, float intensity, bool active) : m_Direction(direction), m_Color(color), m_Intensity(intensity), m_Active(active) {}
	
	Spotlight::Spotlight(glm::vec3 position, glm::vec3 direction, glm::vec3 color, float innerCutoff, float outerCutoff, float range, float intensity, bool active) : m_Position(position), m_Direction(direction), m_Color(color), m_InnerCutoff(innerCutoff), m_OuterCutoff(outerCutoff), m_Range(range), m_Intensity(intensity), m_Active(active) {}
}