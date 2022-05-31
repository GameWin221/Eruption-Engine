#include <Core/EnPch.hpp>
#include "Texture.hpp"

namespace en
{
	std::array<unsigned char, 4> g_WhiteTexturePixels = { 255, 255, 255, 255 };

	Texture* g_WhiteTexture;
	Texture* g_NormalTexture;
	Texture* g_GreyTexture;

	Texture::Texture(std::string texturePath, std::string name, VkFormat format, bool flipTexture, bool useMipMaps) : m_Name(name), m_FilePath(texturePath), m_ImageFormat(format), m_UsesMipMaps(useMipMaps)
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

		CreateImage(pixels);

		if(m_UsesMipMaps)
			GenerateMipMaps();

		CreateImageSampler();

		if (shouldFreeImage)
		{
			stbi_image_free(pixels);
			EN_SUCCESS("Successfully loaded a texture from \"" + m_FilePath + "\"");
		}
	}
	Texture::Texture(stbi_uc* pixelData, std::string name, VkFormat format, glm::uvec2 size, bool useMipMaps): m_Name(name), m_Size(size), m_ImageFormat(format), m_UsesMipMaps(useMipMaps)
	{
		CreateImage(pixelData);

		if (m_UsesMipMaps)
			GenerateMipMaps();

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

	void Texture::CreateImage(void* data)
	{
		VkDeviceSize imageSize = m_Size.x * m_Size.y * 4U;

		m_MipLevels = m_UsesMipMaps ? GetMipLevels() : 1U;

		MemoryBuffer stagingBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		stagingBuffer.MapMemory(data, imageSize);

		en::Helpers::CreateImage(m_Size.x, m_Size.y, m_ImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Image, m_ImageMemory, m_MipLevels);
		en::Helpers::CreateImageView(m_Image, m_ImageView, m_ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, m_MipLevels);

		en::Helpers::TransitionImageLayout(m_Image, m_ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_MipLevels);

		stagingBuffer.CopyTo(m_Image, m_Size.x, m_Size.y);

		// The image is being transitioned to the right layout (VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) during the mipmap generation
		if (!m_UsesMipMaps)
			en::Helpers::TransitionImageLayout(m_Image, m_ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_MipLevels);
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
		samplerInfo.maxAnisotropy	 = std::min((float)ANISOTROPIC_FILTERING, properties.limits.maxSamplerAnisotropy);

		samplerInfo.borderColor	= VK_BORDER_COLOR_INT_OPAQUE_BLACK;

		samplerInfo.unnormalizedCoordinates = VK_FALSE;

		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp	  = VK_COMPARE_OP_ALWAYS;
		
		if (m_UsesMipMaps)
		{
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.mipLodBias = MIPMAP_BIAS;
			samplerInfo.minLod	   = 0.0f;
			samplerInfo.maxLod	   = static_cast<float>(m_MipLevels);
		}

		if (vkCreateSampler(ctx.m_LogicalDevice, &samplerInfo, nullptr, &m_ImageSampler) != VK_SUCCESS)
			EN_ERROR("Texture.cpp::Texture::CreateImageSampler() - Failed to create texture sampler!");
	}
	void Texture::GenerateMipMaps()
	{
		UseContext();

		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(ctx.m_PhysicalDevice, m_ImageFormat, &formatProperties);

		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
			throw std::runtime_error("Texture::GenerateMipMaps() - The specified image format does not support linear blitting!");

		VkCommandBuffer cmd = Helpers::BeginSingleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType							= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image							= m_Image;
		barrier.srcQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0U;
		barrier.subresourceRange.layerCount		= 1U;
		barrier.subresourceRange.levelCount		= 1U;

		int32_t mipWidth  = static_cast<uint32_t>(m_Size.x);
		int32_t mipHeight = static_cast<uint32_t>(m_Size.y);

		for (uint32_t i = 1U; i < m_MipLevels; i++) 
		{
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout	  = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout	  = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(cmd,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0U,
				0U, nullptr,
				0U, nullptr,
				1U, &barrier);

			VkImageBlit blit{};
			blit.srcOffsets[0] = { 0U, 0U, 0U };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1U };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1U;
			blit.srcSubresource.baseArrayLayer = 0U;
			blit.srcSubresource.layerCount = 1U;
			blit.dstOffsets[0] = { 0U, 0U, 0U };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1U };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0U;
			blit.dstSubresource.layerCount = 1U;

			vkCmdBlitImage(cmd,
				m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1U, &blit,
				VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(cmd,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0U,
				0U, nullptr,
				0U, nullptr,
				1U, &barrier);

			if (mipWidth  > 1U) mipWidth  /= 2U;
			if (mipHeight > 1U) mipHeight /= 2U;
		}

		barrier.subresourceRange.baseMipLevel = m_MipLevels - 1U;
		barrier.oldLayout	  = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout	  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(cmd,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0U,
			0U, nullptr,
			0U, nullptr,
			1U, &barrier);

		Helpers::EndSingleTimeCommands(cmd);
	}
	uint32_t Texture::GetMipLevels()
	{
		return static_cast<uint32_t>(std::floor(std::log2(std::max(m_Size.x, m_Size.y)))) + 1;
	}
}