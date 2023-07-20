#include "ImGuiContext.hpp"

#include <Common/Helpers.hpp>

namespace en
{
	static void ImGuiCheckResult(VkResult err)
	{
		if (err == 0) return;

		EN_ERROR("[ImGui Error]:" + err);
	}

	ImGuiContext::ImGuiContext(VkFormat imageFormat, VkExtent2D imageExtent, const std::vector<VkImageView>& imageViews)
	{
		UseContext();

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		ImGui_ImplGlfw_InitForVulkan(Window::Get().GetNativeHandle(), true);

		ImGui_ImplVulkan_InitInfo initInfo{
			.Instance = ctx.m_Instance,
			.PhysicalDevice = ctx.m_PhysicalDevice,
			.Device = ctx.m_LogicalDevice,
			.QueueFamily = ctx.m_QueueFamilies.graphics.value(),
			.Queue = ctx.m_GraphicsQueue,
			.DescriptorPool = ctx.m_DescriptorAllocator->GetPool(),
			.MinImageCount = static_cast<uint32_t>(imageViews.size()),
			.ImageCount = static_cast<uint32_t>(imageViews.size()),
			.CheckVkResultFn = ImGuiCheckResult,
		};

		VkAttachmentDescription colorDescription{
			.format = imageFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		};

		VkAttachmentReference colorReference{
			.attachment = 0U,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};

		VkSubpassDescription subpass{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1U,
			.pColorAttachments = &colorReference
		};

		VkSubpassDependency dependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0U,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
		};

		VkRenderPassCreateInfo renderPassInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = 1U,
			.pAttachments = &colorDescription,
			.subpassCount = 1U,
			.pSubpasses = &subpass,
			.dependencyCount = 1U,
			.pDependencies = &dependency
		};

		if (vkCreateRenderPass(Context::Get().m_LogicalDevice, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
			EN_ERROR("ImGuiContext::ImGuiContext() - Failed to create a RenderPass!");

		ImGui_ImplVulkan_Init(&initInfo, m_RenderPass);

		VkCommandBuffer cmd = Helpers::BeginSingleTimeGraphicsCommands();
		ImGui_ImplVulkan_CreateFontsTexture(cmd);
		Helpers::EndSingleTimeGraphicsCommands(cmd);

		ImGui_ImplVulkanH_SelectSurfaceFormat(ctx.m_PhysicalDevice, ctx.m_WindowSurface, &imageFormat, 1U, VK_COLORSPACE_SRGB_NONLINEAR_KHR);
	
		UpdateFramebuffers(imageExtent, imageViews);
	}
	ImGuiContext::~ImGuiContext()
	{
		ImGui_ImplVulkan_DestroyFontUploadObjects();
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();

		vkDestroyRenderPass(Context::Get().m_LogicalDevice, m_RenderPass, nullptr);
		
		for (const auto& framebuffer : m_Framebuffers)
			vkDestroyFramebuffer(Context::Get().m_LogicalDevice, framebuffer, nullptr);
	}
	void ImGuiContext::UpdateFramebuffers(VkExtent2D imageExtent, const std::vector<VkImageView>& imageViews)
	{
		m_Extent = imageExtent;

		for (const auto& framebuffer : m_Framebuffers)
			vkDestroyFramebuffer(Context::Get().m_LogicalDevice, framebuffer, nullptr);

		m_Framebuffers.resize(imageViews.size());

		for (int i{}; i < imageViews.size(); ++i)
		{
			VkFramebufferCreateInfo framebufferInfo{
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.renderPass = m_RenderPass,
				.attachmentCount = 1U,
				.pAttachments = &imageViews[i],

				.width = imageExtent.width,
				.height = imageExtent.height,
				.layers = 1U,
			};

			if (vkCreateFramebuffer(Context::Get().m_LogicalDevice, &framebufferInfo, nullptr, &m_Framebuffers[i]) != VK_SUCCESS)
				EN_ERROR("ImGuiContext::UpdateFramebuffers() - Failed to create a Framebuffer!");
		}

		ImGui_ImplVulkan_SetMinImageCount(imageViews.size());
	}
	void ImGuiContext::Render(VkCommandBuffer cmd, uint32_t imageIndex)
	{
		VkRenderPassBeginInfo renderPassInfo {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = m_RenderPass,
			.framebuffer = m_Framebuffers[imageIndex],
			.renderArea = {
				.offset = { 0U, 0U },
				.extent = m_Extent
			},
		};

		vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

		vkCmdEndRenderPass(cmd);
	}
}