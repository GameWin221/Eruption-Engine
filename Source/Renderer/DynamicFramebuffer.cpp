#include "Core/EnPch.hpp"
#include "DynamicFramebuffer.hpp"

#include <Renderer/Context.hpp>
#include <Common/Helpers.hpp>

namespace en
{
	DynamicFramebuffer::~DynamicFramebuffer()
	{
		Destroy();
	}
	void DynamicFramebuffer::Attachment::Destroy()
	{
		UseContext();

		if(imageMemory != VK_NULL_HANDLE)
			vkFreeMemory(ctx.m_LogicalDevice, imageMemory, nullptr);

		if (imageView != VK_NULL_HANDLE)
			vkDestroyImageView(ctx.m_LogicalDevice, imageView, nullptr);

		if (image != VK_NULL_HANDLE)
			vkDestroyImage(ctx.m_LogicalDevice, image, nullptr);
	}
	void DynamicFramebuffer::CreateAttachments(std::vector<AttachmentInfo> attachmentInfos, uint32_t sizeX, uint32_t sizeY)
	{
		m_SizeX = sizeX;
		m_SizeY = sizeY;

		for (const auto& info : attachmentInfos)
		{
			Attachment& attachment = m_Attachments.emplace_back();

			Helpers::CreateImage(m_SizeX, m_SizeY, info.format, VK_IMAGE_TILING_OPTIMAL, info.imageUsageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, attachment.image, attachment.imageMemory);
			Helpers::CreateImageView(attachment.image, attachment.imageView, info.format, info.imageAspectFlags);
			Helpers::TransitionImageLayout(attachment.image, info.format, info.imageAspectFlags, VK_IMAGE_LAYOUT_UNDEFINED, info.initialLayout);
			attachment.format = info.format;
		}
	}

	void DynamicFramebuffer::CreateSampler(VkFilter framebufferFiltering)
	{
		UseContext();

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

		samplerInfo.magFilter = framebufferFiltering;
		samplerInfo.minFilter = framebufferFiltering;

		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 0U;

		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

		samplerInfo.unnormalizedCoordinates = VK_FALSE;

		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;

		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(ctx.m_LogicalDevice, &samplerInfo, nullptr, &m_Sampler) != VK_SUCCESS)
			EN_ERROR("Framebuffer::CreateSampler() - Failed to create a sampler!");
	}

	void DynamicFramebuffer::Destroy()
	{
		for (auto& attachment : m_Attachments)
			attachment.Destroy();
		
		if (m_Sampler != VK_NULL_HANDLE)
			vkDestroySampler(Context::Get().m_LogicalDevice, m_Sampler, nullptr);

		m_Attachments.clear();
	}
}