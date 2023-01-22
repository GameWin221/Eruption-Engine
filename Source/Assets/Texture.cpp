
#include "Texture.hpp"

#include <Common/Helpers.hpp>

namespace en
{
	constexpr uint64_t WHITE_PIXEL = 0xffffffffffffffff; // UINT64_MAX;

	Handle<Texture> g_SRGBWhiteTexture;
	Handle<Texture> g_NSRGBTexture;

	Texture::Texture(std::string texturePath, std::string name, VkFormat format, bool flipTexture, bool useMipMaps) : m_Name(name), m_FilePath(texturePath), Asset{ AssetType::Texture }
	{
		bool shouldFreeImage = true;

		stbi_set_flip_vertically_on_load(flipTexture);

		int sizeX, sizeY, channels;

		uint8_t* pixels = stbi_load(m_FilePath.c_str(), &sizeX, &sizeY, &channels, 4);

		if (!pixels)
		{
			pixels = (uint8_t*)&WHITE_PIXEL;

			sizeX = 1;
			sizeY = 1;
			channels = 4;

			shouldFreeImage = false;

			EN_WARN("Texture::Texture() - Failed to load a texture from \"" + m_FilePath + "\"!");
		}

		m_Image = MakeHandle<Image>(VkExtent2D{(uint32_t)sizeX, (uint32_t)sizeY}, format, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, useMipMaps);

		m_Image->SetData(pixels);

		Helpers::CreateSampler(m_ImageSampler, VK_FILTER_LINEAR, ANISOTROPIC_FILTERING, static_cast<float>(m_Image->GetMipLevels()), MIPMAP_BIAS);

		if (shouldFreeImage)
		{
			stbi_image_free(pixels);
			EN_SUCCESS("Successfully loaded a texture from \"" + m_FilePath + "\"");
		}
	}
	Texture::Texture(stbi_uc* pixels, std::string name, VkFormat format, VkExtent2D size, bool useMipMaps) : m_Name(name), Asset{ AssetType::Texture }
	{
		m_Image = MakeHandle<Image>(size, format, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, useMipMaps);
		m_Image->SetData(pixels);

		Helpers::CreateSampler(m_ImageSampler, VK_FILTER_LINEAR, ANISOTROPIC_FILTERING, static_cast<float>(m_Image->GetMipLevels()), MIPMAP_BIAS);
	}
	Texture::~Texture()
	{
		vkDestroySampler(Context::Get().m_LogicalDevice, m_ImageSampler, nullptr);
	}

	Handle<Texture> Texture::GetWhiteSRGBTexture()
	{
		if (!g_SRGBWhiteTexture)
			g_SRGBWhiteTexture = MakeHandle<Texture>((uint8_t*)&WHITE_PIXEL, "_DefaultWhiteSRGB", VK_FORMAT_R8G8B8A8_SRGB, VkExtent2D{ 1U, 1U }, false);

		return g_SRGBWhiteTexture;
	}
	Handle<Texture> Texture::GetWhiteNonSRGBTexture()
	{
		if (!g_NSRGBTexture)
			g_NSRGBTexture = MakeHandle<Texture>((uint8_t*)&WHITE_PIXEL, "_DefaultWhiteNonSRGB", VK_FORMAT_R8G8B8A8_UNORM, VkExtent2D{ 1U, 1U }, false);

		return g_NSRGBTexture;
	}
}