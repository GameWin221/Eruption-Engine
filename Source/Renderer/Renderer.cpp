#include "Renderer.hpp"

#include <Common/Helpers.hpp>

namespace en
{
	constexpr uint32_t MAX_INSTANCES = 16U * 1024U;
	constexpr uint32_t MAX_INDIRECT_DRAWS = 1024U;

	Renderer* g_CurrentBackend{};
	Context*  g_Ctx{};

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
		DestroyPerFrameData();
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

	void Renderer::WaitForGPUIdle()
	{
		vkWaitForFences(g_Ctx->m_LogicalDevice, 1U, &m_Frames[m_FrameIndex].submitFence, VK_TRUE, UINT64_MAX);
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
		VkResult result = vkAcquireNextImageKHR(g_Ctx->m_LogicalDevice, m_Swapchain->m_Swapchain, UINT64_MAX, m_Frames[m_FrameIndex].mainSemaphore, VK_NULL_HANDLE, &m_Swapchain->m_ImageIndex);

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

		vkResetFences(g_Ctx->m_LogicalDevice, 1U, &m_Frames[m_FrameIndex].submitFence);

		vkResetCommandBuffer(m_Frames[m_FrameIndex].commandBuffer, 0U);

		constexpr VkCommandBufferBeginInfo beginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
		};

		if (vkBeginCommandBuffer(m_Frames[m_FrameIndex].commandBuffer, &beginInfo) != VK_SUCCESS)
			EN_ERROR("Renderer::BeginRender() - Failed to begin recording command buffer!");
	}
	void Renderer::ForwardPass()
	{
		if (m_SkipFrame) return;

		m_RenderPass->Begin(m_Frames[m_FrameIndex].commandBuffer, m_Swapchain->m_ImageIndex);

			m_Pipeline->Bind(m_Frames[m_FrameIndex].commandBuffer, m_Swapchain->GetExtent());

			m_Pipeline->BindDescriptorSet(m_CameraBuffer->GetDescriptorHandle(m_FrameIndex), 0U);
			m_Pipeline->BindDescriptorSet(m_Scene->m_GlobalDescriptorSet, 1U);

			for (const auto& [name, sceneObject] : m_Scene->m_SceneObjects)
			{
				m_Pipeline->PushConstants(&sceneObject->GetMatrixIndex(), sizeof(uint32_t), 0U, VK_SHADER_STAGE_VERTEX_BIT);

				for (const auto& subMesh : sceneObject->m_Mesh->m_SubMeshes)
				{
					m_Pipeline->PushConstants(&subMesh.GetMaterialIndex(), sizeof(uint32_t), sizeof(uint32_t), VK_SHADER_STAGE_FRAGMENT_BIT);

					m_Pipeline->BindVertexBuffer(subMesh.m_VertexBuffer);
					m_Pipeline->BindIndexBuffer(subMesh.m_IndexBuffer);

					m_Pipeline->DrawIndexed(subMesh.m_IndexCount);
				}
			}

		m_RenderPass->End();
	}
	void Renderer::ImGuiPass()
	{
		if (m_SkipFrame || m_ImGuiRenderCallback == nullptr) return;

		m_ImGuiRenderCallback();

		m_ImGuiRenderPass->Begin(m_Frames[m_FrameIndex].commandBuffer, m_Swapchain->m_ImageIndex);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_Frames[m_FrameIndex].commandBuffer);

		m_ImGuiRenderPass->End();
	}
	void Renderer::EndRender()
	{
		if (m_SkipFrame) return;

		if (vkEndCommandBuffer(m_Frames[m_FrameIndex].commandBuffer) != VK_SUCCESS)
			EN_ERROR("Renderer::EndRender() - Failed to record command buffer!");

		const VkPipelineStageFlags waitStages[]     = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		const VkSemaphore		   waitSemaphores[] = { m_Frames[m_FrameIndex].mainSemaphore };
		
		const VkCommandBuffer commandBuffers[] = { m_Frames[m_FrameIndex].commandBuffer };

		const VkSemaphore signalSemaphores[] = { m_Frames[m_FrameIndex].presentSemaphore };

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


		if (vkQueueSubmit(g_Ctx->m_GraphicsQueue, 1U, &submitInfo, m_Frames[m_FrameIndex].submitFence) != VK_SUCCESS)
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

	void Renderer::SetVSync(bool vSync)
	{
		m_VSync = vSync;
		m_FramebufferResized = true;
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

		CreateDepthBuffer();

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
		DestroyPerFrameData();

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

			CreatePerFrameData();

		EN_SUCCESS("Created per frame data!")

			CreateDepthBuffer();

		EN_SUCCESS("Created depth buffer!")

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
		std::vector<VkSubpassDependency> dependencies{
			VkSubpassDependency {
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0U,
				.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
				.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
				.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
			}
		};

		std::vector<RenderPass::Attachment> attachments{
			RenderPass::Attachment {
				.imageViews = m_Swapchain->m_ImageViews,

				.format = m_Swapchain->GetFormat(),

				.refLayout	   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,

				.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,

				.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			}
		};

		RenderPass::Attachment depthAttachment{
			.imageViews = {m_DepthBuffer->GetViewHandle()},

			.format = m_DepthBuffer->m_Format,

			.refLayout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.finalLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,

			.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,

			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		};

		m_RenderPass = MakeHandle<RenderPass>(m_Swapchain->GetExtent(), attachments, depthAttachment, dependencies);
	}
	void Renderer::CreateForwardPipeline()
	{
		constexpr VkPushConstantRange model {
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset		= 0U,
			.size		= sizeof(uint32_t),
		};
		constexpr VkPushConstantRange material {
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.offset		= sizeof(uint32_t),
			.size		= sizeof(uint32_t),
		};

		GraphicsPipeline::CreateInfo info{
			.renderPass = m_RenderPass,

			.vShader = "Shaders/vert.spv",
			.fShader = "Shaders/frag.spv",

			.descriptorLayouts {m_CameraBuffer->GetLayout(), Scene::GetGlobalDescriptorLayout()},
			.pushConstantRanges {model, material},

			.useVertexBindings = true,
			.enableDepthTest   = true,
			.enableDepthWrite  = true,
			.blendEnable	   = false,

			.compareOp	 = VK_COMPARE_OP_LESS,
			.polygonMode = VK_POLYGON_MODE_FILL,
		};

		m_Pipeline = MakeHandle<GraphicsPipeline>(info);
	}

	void Renderer::CreateDepthBuffer()
	{
		m_DepthBuffer = MakeHandle<Image>(
			m_Swapchain->GetExtent(),
			VK_FORMAT_D32_SFLOAT,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
		);
	}

	void Renderer::CreatePerFrameData()
	{
		constexpr VkFenceCreateInfo fenceInfo{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT
		};

		constexpr VkSemaphoreCreateInfo semaphoreInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
		};

		for (auto& frame : m_Frames)
		{
			if (vkCreateFence(g_Ctx->m_LogicalDevice, &fenceInfo, nullptr, &frame.submitFence) != VK_SUCCESS)
				EN_ERROR("Renderer::CreatePerFrameData() - Failed to create a submit fence!");

			if (vkCreateSemaphore(g_Ctx->m_LogicalDevice, &semaphoreInfo, nullptr, &frame.mainSemaphore) != VK_SUCCESS)
				EN_ERROR("GraphicsPipeline::CreatePerFrameData - Failed to create a main semaphore!");

			if (vkCreateSemaphore(g_Ctx->m_LogicalDevice, &semaphoreInfo, nullptr, &frame.presentSemaphore) != VK_SUCCESS)
				EN_ERROR("GraphicsPipeline::CreatePerFrameData - Failed to create a present semaphore!");
		
			VkCommandBufferAllocateInfo allocInfo{
				.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.commandPool		= g_Ctx->m_GraphicsCommandPool,
				.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				.commandBufferCount = 1U
			};

			if (vkAllocateCommandBuffers(g_Ctx->m_LogicalDevice, &allocInfo, &frame.commandBuffer) != VK_SUCCESS)
				EN_ERROR("Renderer::CreatePerFrameData() - Failed to allocate a command buffer!");
		}	
	}
	void Renderer::DestroyPerFrameData()
	{
		for (auto& frame : m_Frames)
		{
			vkDestroyFence(g_Ctx->m_LogicalDevice, frame.submitFence, nullptr);
			vkDestroySemaphore(g_Ctx->m_LogicalDevice, frame.mainSemaphore, nullptr);
			vkDestroySemaphore(g_Ctx->m_LogicalDevice, frame.presentSemaphore, nullptr);

			vkFreeCommandBuffers(g_Ctx->m_LogicalDevice, g_Ctx->m_GraphicsCommandPool, 1U, &frame.commandBuffer);
		}
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

		m_ImGuiRenderPass = MakeHandle<RenderPass>(m_Swapchain->GetExtent(), attachments, RenderPass::Attachment{}, dependencies);

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