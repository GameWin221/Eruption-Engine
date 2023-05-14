#include "Framebuffer.hpp"

namespace en
{
	Framebuffer::Framebuffer(Handle<RenderPass> renderPass, const VkExtent2D extent, const RenderPassAttachment& colorAttachment, const RenderPassAttachment& depthAttachment)
        : m_Extent(extent), m_ColorAttachment(colorAttachment), m_DepthAttachment(depthAttachment)
	{
        std::vector<VkImageView> imageViews{ };
        if (colorAttachment.imageView != VK_NULL_HANDLE)
            imageViews.push_back(colorAttachment.imageView);
        if (depthAttachment.imageView != VK_NULL_HANDLE)
            imageViews.push_back(depthAttachment.imageView);


        VkFramebufferCreateInfo framebufferInfo {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = renderPass->GetHandle(),
            .attachmentCount = static_cast<uint32_t>(imageViews.size()),
            .pAttachments    = imageViews.data(),
      
            .width  = extent.width,
            .height = extent.height,
            .layers = 1U,
        };

        if (vkCreateFramebuffer(Context::Get().m_LogicalDevice, &framebufferInfo, nullptr, &m_Handle) != VK_SUCCESS)
            EN_ERROR("Framebuffer::Framebuffer() - Failed to create a Framebuffer!");
	}

	Framebuffer::~Framebuffer()
	{
        vkDestroyFramebuffer(Context::Get().m_LogicalDevice, m_Handle, nullptr);
	}
}