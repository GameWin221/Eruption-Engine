#include "Material.hpp"

namespace en
{
	bool g_LayoutCreated = false;

	Material::Material(const std::string& name, const glm::vec3 color, const float metalnessVal, const float roughnessVal, const float normalStrength, Handle<Texture> albedoTexture, Handle<Texture> roughnessTexture, Handle<Texture> normalTexture, Handle<Texture> metalnessTexture)
		: m_Name(name), m_Color(color), m_MetalnessVal(metalnessVal), m_RoughnessVal(roughnessVal), m_NormalStrength(normalStrength), m_Albedo(albedoTexture), m_Roughness(roughnessTexture), m_Metalness(metalnessTexture), m_Normal(normalTexture), Asset{ AssetType::Material }
	{
		DescriptorInfo info = MakeDescriptorInfo();

		m_DescriptorSet = MakeHandle<DescriptorSet>(info);

		m_UpdateQueued = true;

		Update();
	}
	Material::~Material()
	{

	}

	void Material::SetColor(glm::vec3 color)
	{
		m_Color = color;
		m_UpdateQueued = true;
	}
	void Material::SetNormalStrength(float normalStrength)
	{
		m_NormalStrength = normalStrength;
		m_UpdateQueued = true;
	}
	void Material::SetMetalness(float metalness)
	{
		m_MetalnessVal = metalness;
		m_UpdateQueued = true;
	}
	void Material::SetRoughness(float roughness)
	{
		m_RoughnessVal = roughness;
		m_UpdateQueued = true;
	}

	void Material::SetAlbedoTexture(Handle<Texture> texture)
	{
		m_Albedo = texture;
		m_UpdateQueued = true;
	}
	void Material::SetRoughnessTexture(Handle<Texture> texture)
	{
		m_Roughness = texture;
		m_UpdateQueued = true;
	}
	void Material::SetMetalnessTexture(Handle<Texture> texture)
	{
		m_Metalness = texture;
		m_UpdateQueued = true;
	}
	void Material::SetNormalTexture(Handle<Texture> texture)
	{
		m_Normal = texture;
		m_UpdateQueued = true;
	}

	void Material::Update()
	{
		m_MatBuffer.color = m_Color;
		m_MatBuffer.metalnessVal = m_MetalnessVal;
		m_MatBuffer.roughnessVal = m_RoughnessVal;
		m_MatBuffer.normalStrength = m_NormalStrength;
		/*
		if (m_Roughness.get() != Texture::GetWhiteNonSRGBTexture().get())
			m_MatBuffer.roughnessVal = 1.0f;

		if (m_Metalness.get() != Texture::GetWhiteNonSRGBTexture().get())
			m_MatBuffer.metalnessVal = 1.0f;

		if (m_Normal.get() == Texture::GetWhiteNonSRGBTexture().get())
			m_MatBuffer.normalStrength = 0.0f;
		*/

		if (!m_UpdateQueued) return;

		m_DescriptorSet->Update(MakeDescriptorInfo());

		m_UpdateQueued = false;
	}

	DescriptorInfo Material::MakeDescriptorInfo()
	{
		return DescriptorInfo {
			std::vector<DescriptorInfo::ImageInfo> {
				DescriptorInfo::ImageInfo {
					.index = 0U,
					.imageView = m_Albedo->m_Image->GetViewHandle(),
					.imageSampler = m_Albedo->m_ImageSampler,
				},
				DescriptorInfo::ImageInfo {
					.index = 1U,
					.imageView = m_Roughness->m_Image->GetViewHandle(),
					.imageSampler = m_Roughness->m_ImageSampler,
				},
				DescriptorInfo::ImageInfo {
					.index = 2U,
					.imageView = m_Normal->m_Image->GetViewHandle(),
					.imageSampler = m_Normal->m_ImageSampler,
				},
				DescriptorInfo::ImageInfo {
					.index = 3U,
					.imageView = m_Metalness->m_Image->GetViewHandle(),
					.imageSampler = m_Metalness->m_ImageSampler,
				},
			},
			std::vector<DescriptorInfo::BufferInfo> {}
		};
	}

	VkDescriptorSetLayout Material::GetLayout()
	{
		return Context::Get().m_DescriptorAllocator->MakeLayout(DescriptorInfo{
			std::vector<DescriptorInfo::ImageInfo> {
				DescriptorInfo::ImageInfo { .index = 0U },
				DescriptorInfo::ImageInfo { .index = 1U },
				DescriptorInfo::ImageInfo { .index = 2U },
				DescriptorInfo::ImageInfo { .index = 3U },
			},
			std::vector<DescriptorInfo::BufferInfo> {},
		});
	}
}