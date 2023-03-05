#include "Renderer.hpp"

#include <Common/Helpers.hpp>

namespace en
{
	Renderer* g_CurrentBackend{};
	Context* g_Ctx{};

	Renderer::Renderer()
	{
		g_Ctx = &Context::Get();

		g_CurrentBackend = this;

		Window::Get().SetResizeCallback(Renderer::FramebufferResizeCallback);

		CreateBackend();
	}
	Renderer::~Renderer()
	{
		vkDeviceWaitIdle(g_Ctx->m_LogicalDevice);

		DestroyImGuiContext();
		DestroyCommandBuffer();
		DestroySyncObjects();
	}

	void Renderer::PreRender()
	{
		m_CameraBuffer->UpdateBuffer(m_FrameIndex, m_Scene->m_MainCamera, m_Swapchain->GetExtent(), m_DebugMode);
		m_CameraBuffer->MapBuffer(m_FrameIndex);
	}

	void Renderer::Render()
	{
		MeasureFrameTime();

		BeginRender();

		ForwardPass();

		ImGuiPass();

		EndRender();
	}

	void Renderer::BindScene(en::Handle<Scene> scene)
	{
		m_Scene = scene;
	}
	void Renderer::UnbindScene()
	{
		m_Scene = nullptr;
	}
	void Renderer::SetVSync(bool vSync)
	{
		m_VSync = vSync;
		m_FramebufferResized = true;
	}

	void Renderer::WaitForGPUIdle()
	{
		vkWaitForFences(g_Ctx->m_LogicalDevice, 1U, &m_SubmitFences[m_FrameIndex], VK_TRUE, UINT64_MAX);
	}

	void Renderer::MeasureFrameTime()
	{
		static std::chrono::high_resolution_clock::time_point lastFrame;

		auto now = std::chrono::high_resolution_clock::now();

		m_FrameTime = std::chrono::duration<double>(now - lastFrame).count();

		lastFrame = now;
	}
	void Renderer::BeginRender()
	{
		VkResult result = vkAcquireNextImageKHR(g_Ctx->m_LogicalDevice, m_Swapchain->m_Swapchain, UINT64_MAX, m_MainSemaphores[m_FrameIndex], VK_NULL_HANDLE, &m_Swapchain->m_ImageIndex);

		m_SkipFrame = false;

		if (m_ReloadQueued)
		{
			ReloadBackendImpl();
			m_SkipFrame = true;
			return;
		}
		else if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateFramebuffer();
			m_SkipFrame = true;
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
			EN_ERROR("Renderer::BeginRender() - Failed to acquire swap chain image!");

		vkResetFences(g_Ctx->m_LogicalDevice, 1U, &m_SubmitFences[m_FrameIndex]);

		vkResetCommandBuffer(m_CommandBuffers[m_FrameIndex], 0U);

		constexpr VkCommandBufferBeginInfo beginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
		};

		if (vkBeginCommandBuffer(m_CommandBuffers[m_FrameIndex], &beginInfo) != VK_SUCCESS)
			EN_ERROR("Renderer::BeginRender() - Failed to begin recording command buffer!");
	}
	void Renderer::ForwardPass()
	{
		if (m_SkipFrame) return;

		m_RenderPass->Begin(m_CommandBuffers[m_FrameIndex], m_Swapchain->m_ImageIndex);

			m_Pipeline->Bind(m_CommandBuffers[m_FrameIndex], m_Swapchain->GetExtent());

			m_Pipeline->BindDescriptorSet(m_CameraBuffer->GetDescriptorHandle(m_FrameIndex));

			for (const auto& sceneObject : m_Scene->GetAllSceneObjects())
			{
				m_Pipeline->PushConstants(&sceneObject->GetModelMatrix(), sizeof(glm::mat4), 0U, VK_SHADER_STAGE_VERTEX_BIT);

				for (const auto& subMesh : sceneObject->m_Mesh->m_SubMeshes)
				{
					m_Pipeline->BindVertexBuffer(subMesh.m_VertexBuffer);
					m_Pipeline->BindIndexBuffer(subMesh.m_IndexBuffer);
					m_Pipeline->DrawIndexed(subMesh.m_IndexBuffer->m_IndicesCount);
				}
			}

		m_RenderPass->End();
	}
	void Renderer::ImGuiPass()
	{
		if (m_SkipFrame || m_ImGuiRenderCallback == nullptr) return;

		m_ImGuiRenderCallback();

		m_ImGuiRenderPass->Begin(m_CommandBuffers[m_FrameIndex], m_Swapchain->m_ImageIndex);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_CommandBuffers[m_FrameIndex]);

		m_ImGuiRenderPass->End();
	}
	void Renderer::EndRender()
	{
		if (m_SkipFrame) return;
		
		/*m_Swapchain->ChangeLayout(m_SwapchainImageIndex, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
								  0U, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
								  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
								  m_CommandBuffers[m_FrameIndex]);*/

		if (vkEndCommandBuffer(m_CommandBuffers[m_FrameIndex]) != VK_SUCCESS)
			EN_ERROR("Renderer::EndRender() - Failed to record command buffer!");

		const VkPipelineStageFlags waitStages[]     = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		const VkSemaphore		   waitSemaphores[] = { m_MainSemaphores[m_FrameIndex] };
		
		const VkCommandBuffer commandBuffers[] = { m_CommandBuffers[m_FrameIndex] };

		const VkSemaphore signalSemaphores[] = { m_PresentSemaphores[m_FrameIndex] };

		VkSubmitInfo submitInfo{
			.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1U,
			.pWaitSemaphores	= waitSemaphores,
			.pWaitDstStageMask  = waitStages,

			.commandBufferCount = 1U,
			.pCommandBuffers	= commandBuffers,

			.signalSemaphoreCount = 1U,
			.pSignalSemaphores	  = signalSemaphores,
		};


		if (vkQueueSubmit(g_Ctx->m_GraphicsQueue, 1U, &submitInfo, m_SubmitFences[m_FrameIndex]) != VK_SUCCESS)
			EN_ERROR("Renderer::EndRender() - Failed to submit command buffer!");

		VkPresentInfoKHR presentInfo{
			.sType				= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1U,
			.pWaitSemaphores	= signalSemaphores,

			.swapchainCount = 1U,
			.pSwapchains	= &m_Swapchain->m_Swapchain,
			.pImageIndices  = &m_Swapchain->m_ImageIndex,
			.pResults	    = nullptr,
		};

		VkResult result = vkQueuePresentKHR(g_Ctx->m_PresentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized)
			RecreateFramebuffer();
		else if (result != VK_SUCCESS)
			EN_ERROR("Renderer::EndRender() - Failed to present swap chain image!");

		m_FrameIndex = (m_FrameIndex + 1) % FRAMES_IN_FLIGHT;
	}

	void Renderer::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		g_CurrentBackend->m_FramebufferResized = true;
	}
	void Renderer::RecreateFramebuffer()
	{
		glm::ivec2 size{};
		while (size.x == 0 || size.y == 0)
			size = Window::Get().GetFramebufferSize();

		vkDeviceWaitIdle(g_Ctx->m_LogicalDevice);

		m_Swapchain.reset(); // It is critical to reset before creating a new one

		m_Swapchain = MakeHandle<Swapchain>(m_VSync);

		CreateForwardPass();

		DestroyImGuiContext();

		CreateImGuiContext();

		ImGui_ImplVulkan_SetMinImageCount(m_Swapchain->m_ImageViews.size());

		m_FramebufferResized = false;

		vkDeviceWaitIdle(g_Ctx->m_LogicalDevice);

		EN_LOG("Resized");
	}
	void Renderer::ReloadBackend()
	{
		m_ReloadQueued = true;
	}
	void Renderer::ReloadBackendImpl()
	{
		vkDeviceWaitIdle(g_Ctx->m_LogicalDevice);

		DestroyImGuiContext();
		DestroyCommandBuffer();
		DestroySyncObjects();

		CreateBackend();

		m_ReloadQueued = false;
	}
	void Renderer::CreateBackend()
	{
		vkDeviceWaitIdle(g_Ctx->m_LogicalDevice);

		EN_SUCCESS("Init began!")

			m_Swapchain.reset();
			m_Swapchain = MakeHandle<Swapchain>(m_VSync);

		EN_SUCCESS("Created swapchain!")

			m_CameraBuffer = MakeHandle<CameraBuffer>();

		EN_SUCCESS("Created the camera buffer!")

			CreateSyncObjects();

		EN_SUCCESS("Created sync objects!")

			CreateCommandBuffer();

		EN_SUCCESS("Created command buffers!")

			CreateForwardPass();

		EN_SUCCESS("Created the forward pass!")

			CreateForwardPipeline();

		EN_SUCCESS("Created the forward pipeline!")

			CreateImGuiContext();

		EN_SUCCESS("Created the ImGui context!")

			EN_SUCCESS("Created the renderer Vulkan backend");

		vkDeviceWaitIdle(g_Ctx->m_LogicalDevice);
	}

	void Renderer::CreateForwardPass()
	{
		std::vector<RenderPass::Attachment> attachments{
			RenderPass::Attachment {
				.imageViews = m_Swapchain->m_ImageViews,

				.format = m_Swapchain->GetFormat(),

				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.refLayout	   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,

				.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,

				.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
			}
		};

		m_RenderPass = MakeHandle<RenderPass>(m_Swapchain->GetExtent(), attachments);
	}
	void Renderer::CreateForwardPipeline()
	{
		VkPushConstantRange pc{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 0U,
			.size = sizeof(glm::mat4),
		};

		GraphicsPipeline::CreateInfo info{
			.renderPass = m_RenderPass,

			.vShader = "Shaders/vert.spv",
			.fShader = "Shaders/frag.spv",

			.descriptorLayouts{m_CameraBuffer->GetLayout()},
			.pushConstantRanges{pc},

			.useVertexBindings = true,
			.enableDepthTest = true,
			.enableDepthWrite = false,
			.blendEnable = false,

			.compareOp = VK_COMPARE_OP_LESS,
			.polygonMode = VK_POLYGON_MODE_FILL,
		};

		m_Pipeline = MakeHandle<GraphicsPipeline>(info);
	}

	void Renderer::CreateCommandBuffer()
	{
		const VkCommandBufferAllocateInfo allocInfo{
			.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool		= g_Ctx->m_GraphicsCommandPool,
			.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = FRAMES_IN_FLIGHT
		};

		if (vkAllocateCommandBuffers(g_Ctx->m_LogicalDevice, &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS)
			EN_ERROR("Renderer::GCreateCommandBuffer() - Failed to allocate command buffer!");
	}
	void Renderer::CreateSyncObjects()
	{
		constexpr VkFenceCreateInfo fenceInfo{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT
		};

		for(auto& fence : m_SubmitFences)
			if (vkCreateFence(g_Ctx->m_LogicalDevice, &fenceInfo, nullptr, &fence) != VK_SUCCESS)
				EN_ERROR("Renderer::CreateSyncObjects() - Failed to create submit fences!");
		
		constexpr VkSemaphoreCreateInfo semaphoreInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
		};
		
		for (auto& semaphore : m_MainSemaphores)
			if (vkCreateSemaphore(g_Ctx->m_LogicalDevice, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS)
				EN_ERROR("GraphicsPipeline::CreateSyncSemaphore - Failed to create main semaphores!");

		for (auto& semaphore : m_PresentSemaphores)
			if (vkCreateSemaphore(g_Ctx->m_LogicalDevice, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS)
				EN_ERROR("GraphicsPipeline::CreateSyncSemaphore - Failed to create present semaphores!");
	}

	void Renderer::DestroyCommandBuffer()
	{
		vkFreeCommandBuffers(g_Ctx->m_LogicalDevice, g_Ctx->m_GraphicsCommandPool, FRAMES_IN_FLIGHT, m_CommandBuffers.data());
	}
	void Renderer::DestroySyncObjects()
	{
		for (auto& fence : m_SubmitFences)
			vkDestroyFence(g_Ctx->m_LogicalDevice, fence, nullptr);

		for (auto& semaphore : m_MainSemaphores)
			vkDestroySemaphore(g_Ctx->m_LogicalDevice, semaphore, nullptr);

		for (auto& semaphore : m_PresentSemaphores)
			vkDestroySemaphore(g_Ctx->m_LogicalDevice, semaphore, nullptr);
	}

	void ImGuiCheckResult(VkResult err)
	{
		if (err == 0) return;

		EN_ERROR("ImGui Error:" + err);
	}

	void Renderer::CreateImGuiContext()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		std::vector<VkSubpassDependency> dependencies{
			VkSubpassDependency {
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0U,
				.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
				.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
			}
		};

		std::vector<RenderPass::Attachment> attachments{
			RenderPass::Attachment {
				.imageViews = m_Swapchain->m_ImageViews,

				.format = m_Swapchain->GetFormat(),

				.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				.refLayout	   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,

				.loadOp  = VK_ATTACHMENT_LOAD_OP_LOAD,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,

				.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			}
		};

		m_ImGuiRenderPass = MakeHandle<RenderPass>(m_Swapchain->GetExtent(), attachments, dependencies);

		ImGui_ImplGlfw_InitForVulkan(Window::Get().m_NativeHandle, true);

		ImGui_ImplVulkan_InitInfo initInfo{
			.Instance		 = g_Ctx->m_Instance,
			.PhysicalDevice  = g_Ctx->m_PhysicalDevice,
			.Device			 = g_Ctx->m_LogicalDevice,
			.QueueFamily	 = g_Ctx->m_QueueFamilies.graphics.value(),
			.Queue			 = g_Ctx->m_GraphicsQueue,
			.DescriptorPool  = g_Ctx->m_DescriptorAllocator->GetPool(),
			.MinImageCount   = static_cast<uint32_t>(m_Swapchain->m_ImageViews.size()),
			.ImageCount		 = static_cast<uint32_t>(m_Swapchain->m_ImageViews.size()),
			.CheckVkResultFn = ImGuiCheckResult
		};

		ImGui_ImplVulkan_Init(&initInfo, m_ImGuiRenderPass->m_RenderPass);

		VkCommandBuffer cmd = Helpers::BeginSingleTimeGraphicsCommands();
		ImGui_ImplVulkan_CreateFontsTexture(cmd);
		Helpers::EndSingleTimeGraphicsCommands(cmd);

		VkFormat format = m_Swapchain->GetFormat();

		ImGui_ImplVulkanH_SelectSurfaceFormat(g_Ctx->m_PhysicalDevice, g_Ctx->m_WindowSurface, &format, 1U, VK_COLORSPACE_SRGB_NONLINEAR_KHR);
		ImGui_ImplVulkan_SetMinImageCount(m_Swapchain->m_ImageViews.size());
	}
	void Renderer::DestroyImGuiContext()
	{
		ImGui_ImplVulkan_DestroyFontUploadObjects();
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();

		m_ImGuiRenderPass.reset();
	}
}