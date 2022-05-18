#include "Core/EnPch.hpp"
#include "Framebuffer.hpp"

#include <Renderer/Context.hpp>

namespace en
{
	Framebuffer::~Framebuffer()
	{
		Destroy();
	}
	void Framebuffer::Attachment::Destroy()
	{
		UseContext();

		vkFreeMemory(ctx.m_LogicalDevice, imageMemory, nullptr);
		vkDestroyImageView(ctx.m_LogicalDevice, imageView, nullptr);
		vkDestroyImage(ctx.m_LogicalDevice, image, nullptr);
	}

	void Framebuffer::CreateSampler()
	{
		UseContext();

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;

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
	void Framebuffer::CreateFramebuffer(VkRenderPass& renderPass)
	{
		std::vector<VkImageView> attachments{};

		for(auto& attachment : m_Attachments)
			attachments.emplace_back(attachment.imageView);

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType			= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass		= renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments	= attachments.data();
		framebufferInfo.width  = m_SizeX;
		framebufferInfo.height = m_SizeY;
		framebufferInfo.layers = 1U;

		UseContext();

		if (vkCreateFramebuffer(ctx.m_LogicalDevice, &framebufferInfo, nullptr, &m_Framebuffer) != VK_SUCCESS)
			EN_ERROR("Framebuffer::CreateFramebuffer() - Failed to create a framebuffer!");
	}
	void Framebuffer::CreateAttachments(std::vector<AttachmentInfo> attachmentInfos, uint32_t sizeX, uint32_t sizeY)
	{
		m_SizeX = sizeX;
		m_SizeY = sizeY;

		for (const auto& info : attachmentInfos)
		{
			Attachment& attachment = m_Attachments.emplace_back();

			Helpers::CreateImage(m_SizeX, m_SizeY, info.format, VK_IMAGE_TILING_OPTIMAL, info.imageUsageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, attachment.image, attachment.imageMemory);
			Helpers::CreateImageView(attachment.image, attachment.imageView, info.format, info.imageAspectFlags);
			Helpers::TransitionImageLayout(attachment.image, info.format, info.imageAspectFlags, info.imageInitialLayout, info.imageFinalLayout);
			attachment.format = info.format;
		}
	}

	void Framebuffer::Destroy()
	{
		UseContext();

		if (m_Sampler != VK_NULL_HANDLE)
			vkDestroySampler(ctx.m_LogicalDevice, m_Sampler, nullptr);
		if (m_Framebuffer != VK_NULL_HANDLE)
			vkDestroyFramebuffer(ctx.m_LogicalDevice, m_Framebuffer, nullptr);

		for (auto& attachment : m_Attachments)
			attachment.Destroy();
		
		m_Attachments.clear();
	}
}