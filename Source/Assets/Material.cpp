#include "Material.hpp"

namespace en
{
	void Material::SetColor(glm::vec3 color)
	{
		m_Color = color;
		m_Changed = true;
	}
	void Material::SetNormalStrength(float normalStrength)
	{
		m_NormalStrength = normalStrength;
		m_Changed = true;
	}
	void Material::SetMetalness(float metalness)
	{
		m_MetalnessVal = metalness;
		m_Changed = true;
	}
	void Material::SetRoughness(float roughness)
	{
		m_RoughnessVal = roughness;
		m_Changed = true;
	}

	void Material::SetAlbedoTexture(Handle<Texture> texture)
	{
		m_Albedo = texture;
		m_Changed = true;
	}
	void Material::SetRoughnessTexture(Handle<Texture> texture)
	{
		m_Roughness = texture;
		m_Changed = true;
	}
	void Material::SetMetalnessTexture(Handle<Texture> texture)
	{
		m_Metalness = texture;
		m_Changed = true;
	}
	void Material::SetNormalTexture(Handle<Texture> texture)
	{
		m_Normal = texture;
		m_Changed = true;
	}
}