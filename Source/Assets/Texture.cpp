#include <Core/EnPch.hpp>
#include "Texture.hpp"

namespace en
{
	std::array<unsigned char, 4> g_WhiteTexturePixels = { 255, 255, 255, 255 };
	std::array<unsigned char, 4> g_BlackTexturePixels = {   0,   0,   0, 255 };

	Texture* g_WhiteTexture;
	Texture* g_BlackTexture;

	Texture::Texture(std::string texturePath, bool flipTexture)
	{
		bool shouldFreeImage = true;

		m_FilePath = texturePath;
		
		VkBuffer       stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		stbi_set_flip_vertically_on_load(flipTexture);

		stbi_uc* pixels = stbi_load(m_FilePath.c_str(), &m_Size.x, &m_Size.y, &m_Channels, 4);

		if (!pixels)
		{
			pixels = g_WhiteTexturePixels.data();
			m_Size = glm::uvec2(1);
			m_Channels = 4;

			shouldFreeImage = false;

			std::cout << "Failed to load \"" + m_FilePath + "\" image!";
		}

		VkDeviceSize imageSize = m_Size.x * m_Size.y * 4;

		en::Helpers::CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		en::Helpers::MapBuffer(stagingBufferMemory, pixels, imageSize);

		en::Helpers::CreateImage(m_Size.x, m_Size.y, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Image, m_ImageMemory);
		en::Helpers::CreateImageView(m_Image, m_ImageView, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

		en::Helpers::TransitionImageLayout(m_Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		CopyBufferToImage(stagingBuffer);

		en::Helpers::TransitionImageLayout(m_Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		CreateImageSampler();

		if (shouldFreeImage)
			stbi_image_free(pixels);

		en::Helpers::DestroyBuffer(stagingBuffer, stagingBufferMemory);
	}
	Texture::Texture(stbi_uc* pixelData, glm::uvec2 size)
	{
		VkBuffer       stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		m_Size = size;
		VkDeviceSize imageSize = m_Size.x * m_Size.y * 4;

		en::Helpers::CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		en::Helpers::MapBuffer(stagingBufferMemory, pixelData, imageSize);

		en::Helpers::CreateImage(m_Size.x, m_Size.y, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, this->m_Image, this->m_ImageMemory);
		en::Helpers::CreateImageView(m_Image, m_ImageView, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

		en::Helpers::TransitionImageLayout(m_Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		CopyBufferToImage(stagingBuffer);

		en::Helpers::TransitionImageLayout(m_Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		CreateImageSampler();

		en::Helpers::DestroyBuffer(stagingBuffer, stagingBufferMemory);
	}
	Texture::~Texture()
	{
		UseContext();

		en::Helpers::DestroyImage(m_Image, m_ImageMemory);

		vkDestroySampler(ctx.m_LogicalDevice, m_ImageSampler, nullptr);
		vkDestroyImageView(ctx.m_LogicalDevice, m_ImageView, nullptr);
	}

	Texture* Texture::GetWhiteTexture()
	{
		if (!g_WhiteTexture)
			g_WhiteTexture = new Texture(g_WhiteTexturePixels.data(), glm::uvec2(1));

		return g_WhiteTexture;
	}
	Texture* Texture::GetBlackTexture()
	{
		if (!g_BlackTexture)
			g_BlackTexture = new Texture(g_BlackTexturePixels.data(), glm::uvec2(1));

		return g_BlackTexture;
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
		samplerInfo.anisotropyEnable		= VK_TRUE;
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
			throw std::runtime_error("Texture.cpp::Texture::CreateImageSampler() - Failed to create texture sampler!");
	}
	void Texture::CopyBufferToImage(const VkBuffer& srcBuffer)
	{
		UseContext();

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = ctx.m_CommandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(ctx.m_LogicalDevice, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = {
			static_cast<uint32_t>(this->m_Size.x),
			static_cast<uint32_t>(this->m_Size.y),
			1
		};

		vkCmdCopyBufferToImage(commandBuffer, srcBuffer, this->m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(ctx.m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(ctx.m_GraphicsQueue);

		vkFreeCommandBuffers(ctx.m_LogicalDevice, ctx.m_CommandPool, 1, &commandBuffer);
	}
}