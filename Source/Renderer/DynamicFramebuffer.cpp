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
	void DynamicFramebuffer::CreateAttachments(std::vector<AttachmentInfo> attachmentInfos, VkExtent2D size)
	{
		m_Size = size;

		m_Attachments.reserve(attachmentInfos.size());

		for (const auto& info : attachmentInfos)
			m_Attachments.emplace_back(m_Size, info.format, info.imageUsageFlags, info.imageAspectFlags, info.initialLayout, false);
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
		if (m_Sampler != VK_NULL_HANDLE)
			vkDestroySampler(Context::Get().m_LogicalDevice, m_Sampler, nullptr);

		m_Attachments.clear();
	}
}