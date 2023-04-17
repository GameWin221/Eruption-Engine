
#include "Texture.hpp"

#include <Common/Helpers.hpp>

namespace en
{
	Texture::Texture(std::string texturePath, std::string name, VkFormat format, bool flipTexture, bool useMipMaps) : m_Name(name), m_FilePath(texturePath), Asset{ AssetType::Texture }
	{
		bool shouldFreeImage = true;

		stbi_set_flip_vertically_on_load(flipTexture);

		int sizeX, sizeY, channels;

		uint8_t* pixels = stbi_load(m_FilePath.c_str(), &sizeX, &sizeY, &channels, 4);
		uint64_t white = 0xffffffffffffffff;

		if (!pixels)
		{
			pixels = (uint8_t*)&white;

			sizeX = 1;
			sizeY = 1;
			channels = 4;

			shouldFreeImage = false;

			EN_WARN("Texture::Texture() - Failed to load a texture from \"" + m_FilePath + "\"!");
		}

		m_Image = MakeHandle<Image>(VkExtent2D{(uint32_t)sizeX, (uint32_t)sizeY}, format, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, 0U, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1U, useMipMaps);

		m_Image->SetData(pixels);

		m_Sampler = MakeHandle<Sampler>(VK_FILTER_LINEAR, ANISOTROPIC_FILTERING, static_cast<float>(m_Image->GetMipLevels()), MIPMAP_BIAS);

		if (shouldFreeImage)
		{
			stbi_image_free(pixels);
			EN_SUCCESS("Successfully loaded a texture from \"" + m_FilePath + "\"");
		}
	}
	Texture::Texture(stbi_uc* pixels, std::string name, VkFormat format, VkExtent2D size, bool useMipMaps) : m_Name(name), Asset{ AssetType::Texture }
	{
		m_Image = MakeHandle<Image>(size, format, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, 0U, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1U, useMipMaps);
		m_Image->SetData(pixels);

		m_Sampler = MakeHandle<Sampler>(VK_FILTER_LINEAR, ANISOTROPIC_FILTERING, static_cast<float>(m_Image->GetMipLevels()), MIPMAP_BIAS);
	}
}