#pragma once

#ifndef EN_MATERIAL_HPP
#define EN_MATERIAL_HPP

#include <Assets/Texture.hpp>

namespace en
{
	class Material
	{
	public:
		Material(glm::vec3 color, float shininess, Texture* albedoTexture, Texture* specularTexture);

		glm::vec3 m_Color;

		float m_Shininess;

		Texture* m_Albedo;
		Texture* m_Specular;

		static Material* GetDefaultMaterial();
	};
}

#endif