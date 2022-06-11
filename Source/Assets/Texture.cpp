#include <Core/EnPch.hpp>
#include "Texture.hpp"

#include <Renderer/Context.hpp>

namespace en
{
	std::array<unsigned char, 4> g_WhiteTexturePixels = { 255, 255, 255, 255 };

	Texture* g_SRGBWhiteTexture;
	Texture* g_NSRGBTexture;

	Texture::Texture(std::string texturePath, std::string name, VkFormat format, bool flipTexture, bool useMipMaps) : m_Name(name), m_FilePath(texturePath), Asset{ AssetType::Texture }
	{
		bool shouldFreeImage = true;

		stbi_set_flip_vertically_on_load(flipTexture);

		int sizeX, sizeY, channels;

		stbi_uc* pixels = stbi_load(m_FilePath.c_str(), &sizeX, &sizeY, &channels, 4);

		if (!pixels)
		{
			pixels = g_WhiteTexturePixels.data();

			sizeX = 1;
			sizeY = 1;
			channels = 4;

			shouldFreeImage = false;

			EN_WARN("Texture::Texture() - Failed to load a texture from \"" + m_FilePath + "\"!");
		}

		m_Image = std::make_unique<Image>(VkExtent2D{(uint32_t)sizeX, (uint32_t)sizeY}, format, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, useMipMaps);

		m_Image->SetData(pixels);

		CreateImageSampler();

		if (shouldFreeImage)
		{
			stbi_image_free(pixels);
			EN_SUCCESS("Successfully loaded a texture from \"" + m_FilePath + "\"");
		}
	}
	Texture::Texture(stbi_uc* pixels, std::string name, VkFormat format, VkExtent2D size, bool useMipMaps) : m_Name(name), Asset{ AssetType::Texture }
	{
		m_Image = std::make_unique<Image>(size, format, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, useMipMaps);
		m_Image->SetData(pixels);

		CreateImageSampler();
	}
	Texture::~Texture()
	{
		vkDestroySampler(Context::Get().m_LogicalDevice, m_ImageSampler, nullptr);
	}

	Texture* Texture::GetWhiteSRGBTexture()
	{
		if (!g_SRGBWhiteTexture)
			g_SRGBWhiteTexture = new Texture(g_WhiteTexturePixels.data(), "_DefaultWhiteSRGB", VK_FORMAT_R8G8B8A8_SRGB, VkExtent2D{ 1U, 1U }, false);

		return g_SRGBWhiteTexture;
	}
	Texture* Texture::GetWhiteNonSRGBTexture()
	{
		if (!g_NSRGBTexture)
			g_NSRGBTexture = new Texture(g_WhiteTexturePixels.data(), "_DefaultWhiteNonSRGB", VK_FORMAT_R8G8B8A8_UNORM, VkExtent2D{ 1U, 1U }, false);

		return g_NSRGBTexture;
	}
	void Texture::CreateImageSampler()
	{
		UseContext();

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(ctx.m_PhysicalDevice, &properties);
		
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType		 = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

		samplerInfo.magFilter	 = VK_FILTER_LINEAR;
		samplerInfo.minFilter	 = VK_FILTER_LINEAR;

		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

		samplerInfo.anisotropyEnable = (ANISOTROPIC_FILTERING > 0);
		samplerInfo.maxAnisotropy	 = std::min(float(ANISOTROPIC_FILTERING), properties.limits.maxSamplerAnisotropy);
		samplerInfo.borderColor		 = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

		samplerInfo.unnormalizedCoordinates = VK_FALSE;

		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp	  = VK_COMPARE_OP_ALWAYS;
		
		if (m_Image->GetMipLevels() > 1U)
		{
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.mipLodBias = MIPMAP_BIAS;
			samplerInfo.minLod	   = 0.0f;
			samplerInfo.maxLod	   = static_cast<float>(m_Image->GetMipLevels());
		}

		if (vkCreateSampler(ctx.m_LogicalDevice, &samplerInfo, nullptr, &m_ImageSampler) != VK_SUCCESS)
			EN_ERROR("Texture.cpp::Texture::CreateImageSampler() - Failed to create texture sampler!");
	}
}