#include <Core/EnPch.hpp>
#include "Texture.hpp"

namespace en
{
	std::array<unsigned char, 4> g_WhiteTexturePixels = { 255, 255, 255, 255 };

	Texture* g_WhiteTexture;
	Texture* g_NormalTexture;
	Texture* g_GreyTexture;

	Texture::Texture(std::string texturePath, std::string name, VkFormat format, bool flipTexture) : m_Name(name), m_FilePath(texturePath), m_ImageFormat(format)
	{
		bool shouldFreeImage = true;

		stbi_set_flip_vertically_on_load(flipTexture);

		stbi_uc* pixels = stbi_load(m_FilePath.c_str(), &m_Size.x, &m_Size.y, &m_Channels, 4);

		if (!pixels)
		{
			pixels = g_WhiteTexturePixels.data();
			m_Size = glm::uvec2(1);
			m_Channels = 4;

			shouldFreeImage = false;

			EN_WARN("Texture::Texture() - Failed to load a texture from \"" + m_FilePath + "\"!");
		}

		VkDeviceSize imageSize = m_Size.x * m_Size.y * 4;

		MemoryBuffer stagingBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		stagingBuffer.MapMemory(pixels, imageSize);

		en::Helpers::CreateImage(m_Size.x, m_Size.y, m_ImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Image, m_ImageMemory);
		en::Helpers::CreateImageView(m_Image, m_ImageView, m_ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

		en::Helpers::TransitionImageLayout(m_Image, m_ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		stagingBuffer.CopyTo(m_Image, m_Size.x, m_Size.y);
	
		en::Helpers::TransitionImageLayout(m_Image, m_ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		CreateImageSampler();

		if (shouldFreeImage)
		{
			stbi_image_free(pixels);
			EN_SUCCESS("Successfully loaded a texture from \"" + m_FilePath + "\"");
		}
	}
	Texture::Texture(stbi_uc* pixelData, std::string name, VkFormat format, glm::uvec2 size): m_Name(name), m_Size(size), m_ImageFormat(format)
	{
		VkDeviceSize imageSize = m_Size.x * m_Size.y * 4;

		MemoryBuffer stagingBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		stagingBuffer.MapMemory(pixelData, imageSize);

		en::Helpers::CreateImage(m_Size.x, m_Size.y, m_ImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, this->m_Image, this->m_ImageMemory);
		en::Helpers::CreateImageView(m_Image, m_ImageView, m_ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

		en::Helpers::TransitionImageLayout(m_Image, m_ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		stagingBuffer.CopyTo(m_Image, m_Size.x, m_Size.y);

		en::Helpers::TransitionImageLayout(m_Image, m_ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		CreateImageSampler();
	}
	Texture::~Texture()
	{
		UseContext();

		en::Helpers::DestroyImage(m_Image, m_ImageMemory);
		
		vkDestroySampler(ctx.m_LogicalDevice, m_ImageSampler, nullptr);
		vkDestroyImageView(ctx.m_LogicalDevice, m_ImageView, nullptr);
	}

	Texture* Texture::GetWhiteSRGBTexture()
	{
		if (!g_WhiteTexture)
			g_WhiteTexture = new Texture(g_WhiteTexturePixels.data(), "_DefaultWhiteSRGB", VK_FORMAT_R8G8B8A8_SRGB, glm::uvec2(1));

		return g_WhiteTexture;
	}
	Texture* Texture::GetWhiteNonSRGBTexture()
	{
		if (!g_GreyTexture)
			g_GreyTexture = new Texture(g_WhiteTexturePixels.data(), "_DefaultWhiteNonSRGB", VK_FORMAT_R8G8B8A8_UNORM, glm::uvec2(1));

		return g_GreyTexture;
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
		samplerInfo.anisotropyEnable		= (ANISOTROPIC_FILTERING > 0);
		samplerInfo.maxAnisotropy			= std::min((float)ANISOTROPIC_FILTERING, properties.limits.maxSamplerAnisotropy);
		samplerInfo.borderColor				= VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable			= VK_FALSE;
		samplerInfo.compareOp				= VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode				= VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias				= 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(ctx.m_LogicalDevice, &samplerInfo, nullptr, &this->m_ImageSampler) != VK_SUCCESS)
			EN_ERROR("Texture.cpp::Texture::CreateImageSampler() - Failed to create texture sampler!");
	}
}