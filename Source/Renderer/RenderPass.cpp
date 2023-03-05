#include "RenderPass.hpp"

namespace en
{
    RenderPass::RenderPass(const VkExtent2D extent, const std::vector<RenderPass::Attachment>& attachments, const std::vector<VkSubpassDependency>& dependencies)
        : m_Extent(extent), m_AttachmentCount(attachments.size())
    {
        UseContext();

        std::vector<VkAttachmentDescription> descriptions(attachments.size());
        for (uint32_t i = 0U; i < attachments.size(); i++)
            descriptions[i] = VkAttachmentDescription{
                .format         = attachments[i].format,
                .samples        = VK_SAMPLE_COUNT_1_BIT,
                .loadOp         = attachments[i].loadOp,
                .storeOp        = attachments[i].storeOp,
                .stencilLoadOp  = attachments[i].stencilLoadOp,
                .stencilStoreOp = attachments[i].stencilStoreOp,
                .initialLayout  = attachments[i].initialLayout,
                .finalLayout    = attachments[i].finalLayout,
        };

        std::vector<VkAttachmentReference> references(attachments.size());

        for (uint32_t i = 0U; i < references.size(); i++)
            references[i] = VkAttachmentReference{
                .attachment = i,
                .layout     = attachments[i].refLayout,
            };

        VkSubpassDescription subpass{
            .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = static_cast<uint32_t>(references.size()),
            .pColorAttachments    = references.data(),
        };

        VkRenderPassCreateInfo renderPassInfo{
            .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = static_cast<uint32_t>(descriptions.size()),
            .pAttachments    = descriptions.data(),
            .subpassCount    = 1U,
            .pSubpasses      = &subpass,
            .dependencyCount = static_cast<uint32_t>(dependencies.size()),
            .pDependencies   = dependencies.data()
        };

        if (vkCreateRenderPass(ctx.m_LogicalDevice, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
            EN_ERROR("RenderPass::RenderPass() - Failed to create a RenderPass!");

        uint32_t maxFramebuffers = 0U;
        for (const auto& attachment : attachments)
            if (attachment.imageViews.size() > maxFramebuffers)
                maxFramebuffers = attachment.imageViews.size();

        m_Framebuffers.resize(maxFramebuffers);
        m_ClearColors.resize(attachments.size(), VkClearValue{{0.0f, 0.0f, 0.0f, 1.0f}});

        for (int32_t i = 0U; i < maxFramebuffers; i++)
        {

            std::vector<VkImageView> imageViews{};
            for (const auto& attachment : attachments)
                imageViews.emplace_back(attachment.imageViews[i]);

            VkFramebufferCreateInfo framebufferInfo{
                .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass      = m_RenderPass,
                .attachmentCount = static_cast<uint32_t>(imageViews.size()),
                .pAttachments    = imageViews.data(),
                .width  = m_Extent.width,
                .height = m_Extent.height,
                .layers = 1U,
            };

            if (vkCreateFramebuffer(ctx.m_LogicalDevice, &framebufferInfo, nullptr, &m_Framebuffers[i]) != VK_SUCCESS)
                EN_ERROR("RenderPass::RenderPass() - Failed to create a Framebuffer!");
        }
    }

    RenderPass::~RenderPass()
    {
        UseContext();

        for(const auto& framebuffer : m_Framebuffers)
            vkDestroyFramebuffer(ctx.m_LogicalDevice, framebuffer, nullptr);

        vkDestroyRenderPass(ctx.m_LogicalDevice, m_RenderPass, nullptr);
    }
    void RenderPass::Begin(VkCommandBuffer cmd, uint32_t framebufferIndex)
    {
        m_BoundCMD = cmd;

        VkRenderPassBeginInfo renderPassInfo{
            .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass  = m_RenderPass,
            .framebuffer = m_Framebuffers[framebufferIndex],
            .renderArea  = {
                .offset  = { 0U, 0U },
                .extent  = m_Extent
            },

            .clearValueCount = static_cast<uint32_t>(m_ClearColors.size()),
            .pClearValues    = m_ClearColors.data(),
        };

        vkCmdBeginRenderPass(m_BoundCMD, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }
    void RenderPass::End()
    {
        vkCmdEndRenderPass(m_BoundCMD);
    }
}