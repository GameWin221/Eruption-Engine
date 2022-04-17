#include <Core/EnPch.hpp>
#include "Material.hpp"

namespace en
{
	Material* g_DefaultMaterial;

	Material::Material(glm::vec3 color, Texture* albedoTexture, Texture* specularTexture) : m_Color(color)
	{
		if (!albedoTexture) m_Albedo = Texture::GetWhiteTexture();
		else m_Albedo = albedoTexture;

		if (!specularTexture) m_Specular = Texture::GetWhiteTexture();
		else m_Specular = specularTexture;
	}
	Material* Material::GetDefaultMaterial()
	{
		if (!g_DefaultMaterial)
			g_DefaultMaterial = new Material(glm::vec3(1.0f), nullptr, nullptr);

		return g_DefaultMaterial;
	}
}