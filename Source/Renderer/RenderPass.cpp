#include "RenderPass.hpp"

namespace en
{
    RenderPass::RenderPass(const RenderPassAttachment& colorAttachment, const RenderPassAttachment& depthStencilAttachment, const std::vector<VkSubpassDependency>& dependencies)
        : m_UsesDepthAttachment(depthStencilAttachment.format != VK_FORMAT_UNDEFINED),
          m_UsesColorAttachment(colorAttachment.format != VK_FORMAT_UNDEFINED)
    {
        UseContext();

        VkAttachmentDescription colorDescription{
            .format         = colorAttachment.format,
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .loadOp         = colorAttachment.loadOp,
            .storeOp        = colorAttachment.storeOp,
            .stencilLoadOp  = colorAttachment.stencilLoadOp,
            .stencilStoreOp = colorAttachment.stencilStoreOp,
            .initialLayout  = colorAttachment.initialLayout,
            .finalLayout    = colorAttachment.finalLayout,
        };

        VkAttachmentReference colorReference{
            .attachment = 0U,
            .layout     = colorAttachment.refLayout,
        };

        VkAttachmentDescription depthDescription {
            .format         = depthStencilAttachment.format,
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .loadOp         = depthStencilAttachment.loadOp,
            .storeOp        = depthStencilAttachment.storeOp,
            .stencilLoadOp  = depthStencilAttachment.stencilLoadOp,
            .stencilStoreOp = depthStencilAttachment.stencilStoreOp,
            .initialLayout  = depthStencilAttachment.initialLayout,
            .finalLayout    = depthStencilAttachment.finalLayout,
        };

        VkAttachmentReference depthReference {
            .attachment = static_cast<uint32_t>(m_UsesColorAttachment),
            .layout     = depthStencilAttachment.refLayout
        };

        std::vector<VkAttachmentDescription> descriptions{};

        if (m_UsesColorAttachment)
            descriptions.emplace_back(colorDescription);
        if (m_UsesDepthAttachment)
            descriptions.emplace_back(depthDescription);

        VkSubpassDescription subpass {
            .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = static_cast<uint32_t>(m_UsesColorAttachment),
            .pColorAttachments    = m_UsesColorAttachment ? &colorReference : nullptr,
            .pDepthStencilAttachment = m_UsesDepthAttachment ? &depthReference : nullptr,
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
    }

    RenderPass::~RenderPass()
    {
        vkDestroyRenderPass(Context::Get().m_LogicalDevice, m_RenderPass, nullptr);
    }
    void RenderPass::Begin(VkCommandBuffer cmd, VkFramebuffer framebuffer, VkExtent2D extent, glm::vec4 clearColor, float depthClearValue, uint32_t stencilClearValue)
    {
        m_BoundCMD = cmd;

        VkClearValue colorClearValue{};
        colorClearValue.color = VkClearColorValue {
            clearColor.r, clearColor.g, clearColor.b, clearColor.a
        };

        VkClearValue depthStencilClearValue{};
        depthStencilClearValue.depthStencil = VkClearDepthStencilValue{
            depthClearValue, stencilClearValue
        };

        std::vector<VkClearValue> clearValues{};
        if (m_UsesColorAttachment)
            clearValues.emplace_back(colorClearValue);
        if (m_UsesDepthAttachment)
            clearValues.emplace_back(depthStencilClearValue);

        VkRenderPassBeginInfo renderPassInfo{
            .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass  = m_RenderPass,
            .framebuffer = framebuffer,
            .renderArea  = {
                .offset  = { 0U, 0U },
                .extent  = extent
            },
       
            .clearValueCount = static_cast<uint32_t>(clearValues.size()),
            .pClearValues    = clearValues.data(),
        };

        vkCmdBeginRenderPass(m_BoundCMD, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }
    void RenderPass::End()
    {
        vkCmdEndRenderPass(m_BoundCMD);
    }
}