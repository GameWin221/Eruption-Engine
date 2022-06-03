#include <Core/EnPch.hpp>
#include "RendererBackend.hpp"

#include <Common/Helpers.hpp>

namespace en
{
	Camera* g_DefaultCamera;

	VulkanRendererBackend* g_CurrentBackend;

	void VulkanRendererBackend::Init()
	{
		m_Ctx		   = &Context::GetContext();
		m_Window	   = &Window::GetMainWindow();

		g_CurrentBackend = this;

		CameraInfo cameraInfo{};
		cameraInfo.dynamicallyScaled = true;

		if (!g_DefaultCamera)
			g_DefaultCamera = new Camera(cameraInfo);

		m_MainCamera = g_DefaultCamera;

		vkDeviceWaitIdle(m_Ctx->m_LogicalDevice);

		glfwSetFramebufferSizeCallback(m_Window->m_GLFWWindow, VulkanRendererBackend::FramebufferResizeCallback);

        EN_SUCCESS("Init began!")

		CreateSwapchain();

		EN_SUCCESS("Created swapchain!")

		CreateSwapchainFramebuffers();

		EN_SUCCESS("Created swapchain framebuffers!")

		CreateFramebuffers();

        EN_SUCCESS("Created the main framebuffers!")

		CreateCommandBuffer();

		EN_SUCCESS("Created command buffer!")

		InitDepthPipeline();

		EN_SUCCESS("Created depth pipeline!")

		InitGeometryPipeline();

		EN_SUCCESS("Created geometry pipeline!")

		InitLightingPipeline();

        EN_SUCCESS("Created lighting pipeline!")

		InitTonemappingPipeline();

        EN_SUCCESS("Created PP pipeline!")

		InitAntialiasingPipeline();

		EN_SUCCESS("Created Antialiasing pipeline!");

		CreateSyncObjects();

        EN_SUCCESS("Created sync objects!")

		CreateCameraMatricesBuffer();

		EN_SUCCESS("Created camera matrices buffer!")

		InitImGui();

		EN_SUCCESS("Created the renderer Vulkan backend");

		vkDeviceWaitIdle(m_Ctx->m_LogicalDevice);
	}
	void VulkanRendererBackend::BindScene(Scene* scene)
	{
		m_Scene = scene;
	}
	void VulkanRendererBackend::UnbindScene()
	{
		m_Scene = nullptr;
	}
	void VulkanRendererBackend::SetVSync(bool& vSync)
	{
		m_VSync = vSync;
		m_FramebufferResized = true;
	}
	VulkanRendererBackend::~VulkanRendererBackend()
	{
		vkDeviceWaitIdle(m_Ctx->m_LogicalDevice);

		m_Swapchain.Destroy(m_Ctx->m_LogicalDevice);

		m_ImGui.Destroy();

		vkDestroyFence(m_Ctx->m_LogicalDevice, m_SubmitFence, nullptr);

		vkFreeCommandBuffers(m_Ctx->m_LogicalDevice, m_Ctx->m_CommandPool, 1U, &m_CommandBuffer);
	}

	void VulkanRendererBackend::WaitForGPUIdle()
	{
		vkWaitForFences(m_Ctx->m_LogicalDevice, 1U, &m_SubmitFence, VK_TRUE, UINT64_MAX);
	}

	void VulkanRendererBackend::UpdateLights()
	{
		m_LightsChanged = false;

		if (m_Lights.LBO.ambientLight != m_Scene->m_AmbientColor || m_Lights.lastPointLightsSize != m_Scene->GetAllPointLights().size() || m_Lights.lastSpotlightsSize != m_Scene->GetAllSpotlights().size() || m_Lights.lastDirLightsSize != m_Scene->GetAllDirectionalLights().size())
			m_LightsChanged = true;

		m_Lights.LBO.ambientLight = m_Scene->m_AmbientColor;

		auto& pointLights = m_Scene->GetAllPointLights();
		auto& spotLights  = m_Scene->GetAllSpotlights();
		auto& dirLights  = m_Scene->GetAllDirectionalLights();
		
		m_Lights.lastPointLightsSize = pointLights.size();
		m_Lights.lastSpotlightsSize  = spotLights.size();
		m_Lights.lastDirLightsSize   = dirLights.size();

		m_Lights.LBO.activePointLights = 0U;
		m_Lights.LBO.activeSpotlights = 0U;
		m_Lights.LBO.activeDirLights = 0U;

		m_Lights.camera.viewPos = m_MainCamera->m_Position;
		m_Lights.camera.debugMode = m_DebugMode;

		for (int i = 0; i < m_Lights.lastPointLightsSize; i++)
		{
			PointLight::Buffer& lightBuffer = m_Lights.LBO.pointLights[m_Lights.LBO.activePointLights];
			PointLight& light = pointLights[i];

			const glm::vec3 lightCol = light.m_Color * (float)light.m_Active * light.m_Intensity;
			const float     lightRad = light.m_Radius * (float)light.m_Active;

			if (lightBuffer.position != light.m_Position || lightBuffer.radius != lightRad || lightBuffer.color != lightCol)
				m_LightsChanged = true;

			if (lightCol == glm::vec3(0.0) || lightRad == 0.0f)
				continue;

			lightBuffer.position = light.m_Position;
			lightBuffer.color = lightCol;
			lightBuffer.radius = lightRad;

			m_Lights.LBO.activePointLights++;
		}
		for (int i = 0; i < m_Lights.lastSpotlightsSize; i++)
		{
			Spotlight::Buffer& lightBuffer = m_Lights.LBO.spotLights[m_Lights.LBO.activeSpotlights];
			Spotlight& light = spotLights[i];

			const glm::vec3 lightColor = light.m_Color * (float)light.m_Active * light.m_Intensity;

			if (lightBuffer.position != light.m_Position || lightBuffer.innerCutoff != light.m_InnerCutoff || lightBuffer.direction != light.m_Direction || lightBuffer.outerCutoff != light.m_OuterCutoff || lightBuffer.color != lightColor || lightBuffer.range != light.m_Range)
				m_LightsChanged = true;

			if (light.m_Range == 0.0f || light.m_Color == glm::vec3(0.0) || light.m_OuterCutoff == 0.0f)
				continue;

			lightBuffer.position = light.m_Position;
			lightBuffer.range = light.m_Range;
			lightBuffer.outerCutoff = light.m_OuterCutoff;
			lightBuffer.innerCutoff = light.m_InnerCutoff;
			lightBuffer.direction = light.m_Direction;
			lightBuffer.color = lightColor;

			m_Lights.LBO.activeSpotlights++;
		}
		for (int i = 0; i < m_Lights.lastDirLightsSize; i++)
		{
			DirectionalLight::Buffer& lightBuffer = m_Lights.LBO.dirLights[m_Lights.LBO.activeDirLights];
			DirectionalLight& light = dirLights[i];

			glm::vec3 lightCol = light.m_Color * (float)light.m_Active * light.m_Intensity;

			if (lightBuffer.direction != light.m_Direction || lightBuffer.color != lightCol)
				m_LightsChanged = true;

			if (lightCol == glm::vec3(0.0))
				continue;

			lightBuffer.direction = light.m_Direction;
			lightBuffer.color = lightCol;

			m_Lights.LBO.activeDirLights++;
		}

		if (m_LightsChanged)
		{
			// Reset unused lights
			memset(m_Lights.LBO.pointLights + m_Lights.LBO.activePointLights, 0, MAX_POINT_LIGHTS - m_Lights.LBO.activePointLights);
			memset(m_Lights.LBO.spotLights  + m_Lights.LBO.activeSpotlights , 0, MAX_SPOT_LIGHTS  - m_Lights.LBO.activeSpotlights );
			memset(m_Lights.LBO.dirLights   + m_Lights.LBO.activeDirLights  , 0, MAX_DIR_LIGHTS   - m_Lights.LBO.activeDirLights  );
			
			m_Lights.buffer->MapMemory(&m_Lights.LBO, m_Lights.buffer->GetSize());
		}
	}

	void VulkanRendererBackend::BeginRender()
	{
		VkResult result = vkAcquireNextImageKHR(m_Ctx->m_LogicalDevice, m_Swapchain.swapchain, UINT64_MAX, m_MainPass->m_PassFinished, VK_NULL_HANDLE, &m_ImageIndex);

		m_SkipFrame = false;

		if (m_ReloadQueued)
		{
			ReloadBackendImpl();
			m_SkipFrame = true;
			return;
		}
		else if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized)
		{
			RecreateFramebuffer();
			m_SkipFrame = true;
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
			EN_ERROR("VulkanRendererBackend::BeginRender() - Failed to acquire swap chain image!");


		vkResetFences(m_Ctx->m_LogicalDevice, 1U, &m_SubmitFence);

		vkResetCommandBuffer(m_CommandBuffer, 0U);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(m_CommandBuffer, &beginInfo) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::BeginRender() - Failed to begin recording command buffer!");
	}
	void VulkanRendererBackend::DepthPass()
	{
		if (m_SkipFrame || !m_Scene) return;

		static std::vector<VkClearValue> clearValues(7);
		clearValues[0].color = { m_Scene->m_AmbientColor.r, m_Scene->m_AmbientColor.g, m_Scene->m_AmbientColor.b, 1.0f };
		clearValues[1].color = clearValues[0].color;
		clearValues[2].color = m_BlackClearValue.color;
		clearValues[3].color = m_BlackClearValue.color;
		clearValues[4].color = m_BlackClearValue.color;
		clearValues[5].color = m_BlackClearValue.color;
		clearValues[6].depthStencil = { 1.0f, 0 };

		m_MainPass->Bind(m_CommandBuffer, m_MainFramebuffers[m_ImageIndex]->m_Framebuffer, m_Swapchain.extent, clearValues);
		m_DepthPipeline->Bind(m_CommandBuffer);

		// Bind the m_MainCamera once per geometry pass
		m_MainCamera->Bind(m_CommandBuffer, m_DepthPipeline->m_Layout, m_CameraMatrices.get());

		for (const auto& sceneObjectPair : m_Scene->m_SceneObjects)
		{
			SceneObject* object = sceneObjectPair.second.get();

			if (!object->m_Active || !object->m_Mesh->m_Active) continue;

			// Bind object data (model matrix) once per SceneObject in the m_RenderQueue
			object->GetObjectData().Bind(m_CommandBuffer, m_DepthPipeline->m_Layout);

			for (const auto& subMesh : object->m_Mesh->m_SubMeshes)
			{
				if (!subMesh.m_Active) continue;

				// Bind SubMesh buffers and material once for each SubMesh in the sceneObject->m_Mesh->m_SubMeshes vector
				subMesh.m_VertexBuffer->Bind(m_CommandBuffer);
				subMesh.m_IndexBuffer->Bind(m_CommandBuffer);

				vkCmdDrawIndexed(m_CommandBuffer, subMesh.m_IndexBuffer->GetIndicesCount(), 1, 0, 0, 0);
			}
		}
	}
	void VulkanRendererBackend::GeometryPass()
	{
		if (m_SkipFrame || !m_Scene) return;

		m_MainPass->NextSubpass(m_CommandBuffer);

		m_GeometryPipeline->Bind(m_CommandBuffer);

		// Bind the m_MainCamera once per geometry pass
		m_MainCamera->Bind(m_CommandBuffer, m_GeometryPipeline->m_Layout, m_CameraMatrices.get());

		for (const auto& sceneObjectPair : m_Scene->m_SceneObjects)
		{
			auto& object = sceneObjectPair.second;

			if (!object->m_Active || !object->m_Mesh->m_Active) continue;

			// Bind object data (model matrix) once per SceneObject in the m_RenderQueue
			object->GetObjectData().Bind(m_CommandBuffer, m_GeometryPipeline->m_Layout);

			for (auto& subMesh : object->m_Mesh->m_SubMeshes)
			{
				if (!subMesh.m_Active) continue;

				// Bind SubMesh buffers and material once for each SubMesh in the sceneObject->m_Mesh->m_SubMeshes vector
				subMesh.m_VertexBuffer->Bind(m_CommandBuffer);
				subMesh.m_IndexBuffer ->Bind(m_CommandBuffer);
				subMesh.m_Material    ->Bind(m_CommandBuffer, m_GeometryPipeline->m_Layout);

				subMesh.Draw(m_CommandBuffer);
			}
		}
	}
	void VulkanRendererBackend::LightingPass()
	{
		if (m_SkipFrame || !m_Scene) return;

		m_MainPass->NextSubpass(m_CommandBuffer);

		Helpers::TransitionImageLayout(m_MainFramebuffers[m_ImageIndex]->m_Attachments[1].image, m_MainFramebuffers[m_ImageIndex]->m_Attachments[0].format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1U, m_CommandBuffer);
		Helpers::TransitionImageLayout(m_MainFramebuffers[m_ImageIndex]->m_Attachments[2].image, m_MainFramebuffers[m_ImageIndex]->m_Attachments[1].format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1U, m_CommandBuffer);
		Helpers::TransitionImageLayout(m_MainFramebuffers[m_ImageIndex]->m_Attachments[3].image, m_MainFramebuffers[m_ImageIndex]->m_Attachments[2].format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1U, m_CommandBuffer);
		
		Helpers::TransitionImageLayout(m_MainFramebuffers[m_ImageIndex]->m_Attachments[4].image, m_MainFramebuffers[m_ImageIndex]->m_Attachments[4].format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1U, m_CommandBuffer);

		m_LightingPipeline->Bind(m_CommandBuffer);

		vkCmdPushConstants(m_CommandBuffer, m_LightingPipeline->m_Layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0U, sizeof(Lights::LightsCameraInfo), &m_Lights.camera);

		m_LightingInput->Bind(m_CommandBuffer, m_LightingPipeline->m_Layout);

		vkCmdDraw(m_CommandBuffer, 3U, 1U, 0U, 0U);
	}
	void VulkanRendererBackend::PostProcessPass()
	{
		if (m_SkipFrame || !m_Scene) return;
		
		m_MainPass->NextSubpass(m_CommandBuffer);

		Helpers::TransitionImageLayout(m_MainFramebuffers[m_ImageIndex]->m_Attachments[5].image, m_MainFramebuffers[m_ImageIndex]->m_Attachments[5].format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1U, m_CommandBuffer);
		
		m_TonemappingPipeline->Bind(m_CommandBuffer);

		m_PostProcessParams.exposure.value = m_MainCamera->m_Exposure;

		vkCmdPushConstants(m_CommandBuffer, m_TonemappingPipeline->m_Layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0U, sizeof(PostProcessingParams::Exposure), &m_PostProcessParams.exposure);

		vkCmdDraw(m_CommandBuffer, 3U, 1U, 0U, 0U);
	}
	void VulkanRendererBackend::AntialiasPass()
	{
		m_MainPass->NextSubpass(m_CommandBuffer);

		if (m_SkipFrame || !m_Scene || m_PostProcessParams.antialiasingMode == AntialiasingMode::None) return;

		m_AntialiasingPipeline->Bind(m_CommandBuffer);

		m_PostProcessParams.antialiasing.texelSizeX = 1.0f / static_cast<float>(m_Swapchain.extent.width );
		m_PostProcessParams.antialiasing.texelSizeY = 1.0f / static_cast<float>(m_Swapchain.extent.height);

		vkCmdPushConstants(m_CommandBuffer, m_AntialiasingPipeline->m_Layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0U, sizeof(PostProcessingParams::Antialiasing), &m_PostProcessParams.antialiasing);
		
		vkCmdDraw(m_CommandBuffer, 3U, 1U, 0U, 0U);
	}
	void VulkanRendererBackend::ImGuiPass()
	{
		if (m_SkipFrame || m_ImGuiRenderCallback == nullptr) return;

		m_ImGuiRenderCallback();

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType			 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass		 = m_ImGui.renderPass;
		renderPassInfo.framebuffer		 = m_MainFramebuffers[m_ImageIndex]->m_Framebuffer;
		renderPassInfo.renderArea.offset = { 0U, 0U };
		renderPassInfo.renderArea.extent = m_Swapchain.extent;
		renderPassInfo.clearValueCount   = 1U;
		renderPassInfo.pClearValues		 = &m_BlackClearValue;

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		if (vkBeginCommandBuffer(m_ImGui.commandBuffers[m_ImageIndex], &beginInfo) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::BeginRender() - Failed to begin recording ImGui command buffer!");

		vkCmdBeginRenderPass(m_ImGui.commandBuffers[m_ImageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_ImGui.commandBuffers[m_ImageIndex]);

		vkCmdEndRenderPass(m_ImGui.commandBuffers[m_ImageIndex]);

		if (vkEndCommandBuffer(m_ImGui.commandBuffers[m_ImageIndex]) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::EndRender() - Failed to record ImGui command buffer!");
	}
	void VulkanRendererBackend::EndRender()
	{
		if (m_SkipFrame) return;
		
		if (vkEndCommandBuffer(m_CommandBuffer) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::EndRender() - Failed to record command buffer!");

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		submitInfo.waitSemaphoreCount = 1U;
		submitInfo.pWaitSemaphores    = &m_MainPass->m_PassFinished;
		submitInfo.pWaitDstStageMask  = &waitStage;

		VkCommandBuffer commandBuffers[] = {m_CommandBuffer, m_ImGui.commandBuffers[m_ImageIndex]};
		submitInfo.commandBufferCount    = 2U;
		submitInfo.pCommandBuffers       = commandBuffers;

		if(m_ImGuiRenderCallback == nullptr)
		{
			submitInfo.commandBufferCount = 1U;
			submitInfo.pCommandBuffers = &m_CommandBuffer;
		}

		submitInfo.signalSemaphoreCount = 1U;
		submitInfo.pSignalSemaphores    = &m_PresentSemaphore;

		if (vkQueueSubmit(m_Ctx->m_GraphicsQueue, 1U, &submitInfo, m_SubmitFence) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::EndRender() - Failed to submit command buffer!");

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1U;
		presentInfo.pWaitSemaphores	   = &m_PresentSemaphore;

		presentInfo.swapchainCount = 1U;
		presentInfo.pSwapchains    = &m_Swapchain.swapchain;
		presentInfo.pImageIndices  = &m_ImageIndex;
		presentInfo.pResults	   = nullptr;

		VkResult result = vkQueuePresentKHR(m_Ctx->m_PresentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized)
			RecreateFramebuffer();
		else if (result != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::EndRender() - Failed to present swap chain image!");
	}

	void VulkanRendererBackend::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		g_CurrentBackend->m_FramebufferResized = true;
	}
	void VulkanRendererBackend::RecreateFramebuffer()
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(m_Window->m_GLFWWindow, &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(m_Window->m_GLFWWindow, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(m_Ctx->m_LogicalDevice);

		m_Swapchain.Destroy(m_Ctx->m_LogicalDevice);

		CreateSwapchain();

		m_MainPass->Resize();

		m_DepthPipeline->Resize(m_Swapchain.extent);

		m_GeometryPipeline->Resize(m_Swapchain.extent);

		UpdateLightingInput();

		m_LightingPipeline->Resize(m_Swapchain.extent);

		m_TonemappingPipeline->Resize(m_Swapchain.extent);

		if (m_PostProcessParams.antialiasingMode != AntialiasingMode::None)
			m_AntialiasingPipeline->Resize(m_Swapchain.extent);

		ImGui_ImplVulkan_SetMinImageCount(m_Swapchain.imageViews.size());

		m_FramebufferResized = false;

		vkDeviceWaitIdle(m_Ctx->m_LogicalDevice);
	}
	void VulkanRendererBackend::ReloadBackend()
	{
		m_ReloadQueued = true;
	}
	void VulkanRendererBackend::ReloadBackendImpl()
	{
		EN_LOG("Reloading the renderer backend...");
		EN_LOG("RELOADING IS DISABLED FOR NOW!");

		/*
		int width = 0, height = 0;
		glfwGetFramebufferSize(m_Window->m_GLFWWindow, &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(m_Window->m_GLFWWindow, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(m_Ctx->m_LogicalDevice);

		m_Swapchain.Destroy(m_Ctx->m_LogicalDevice);

		m_GBuffer->m_Attachments[3] = Framebuffer::Attachment{};

		m_LightingInput.reset();
		m_TonemappingInput.reset();
		m_AntialiasingInput.reset();

		m_DepthPipeline.reset();

		m_GeometryPipeline.reset();

		m_LightingPipeline.reset();
		m_LightingInput.reset();

		m_TonemappingPipeline.reset();
		m_TonemappingInput.reset();

		m_AntialiasingPipeline.reset();
		m_AntialiasingInput.reset();

		m_CameraMatrices.reset();

		m_DepthBuffer.reset();
		m_GBuffer.reset();
		m_HDRFramebuffer.reset();
		m_AliasedFramebuffer.reset();

		vkResetCommandBuffer(m_CommandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
		vkResetFences(m_Ctx->m_LogicalDevice, 1U, &m_SubmitFence);

		glfwSetFramebufferSizeCallback(m_Window->m_GLFWWindow, VulkanRendererBackend::FramebufferResizeCallback);

		CreateSwapchain();

		CreateDepthBufferAttachments();

		CreateGBufferAttachments();

		CreateCommandBuffer();

		InitDepthPipeline();

		InitGeometryPipeline();

		CreateDepthBufferHandle();

		CreateGBufferHandle();

		InitLightingPipeline();

		InitTonemappingPipeline();

		InitAntialiasingPipeline();

		CreateSwapchainFramebuffers();

		CreateSyncObjects();

		CreateCameraMatricesBuffer();

		ImGui_ImplVulkan_SetMinImageCount(m_Swapchain.imageViews.size());

		m_ReloadQueued = false;

		vkDeviceWaitIdle(m_Ctx->m_LogicalDevice);

		EN_SUCCESS("Succesfully reloaded the backend");
		*/
	}

	void VulkanRendererBackend::SetMainCamera(Camera* camera)
	{
		if (camera)
			m_MainCamera = camera;
		else
			m_MainCamera = g_DefaultCamera;
	}

	void VulkanRendererBackend::CreateSwapchain()
	{
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_Ctx->m_PhysicalDevice);

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR   presentMode   = m_VSync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR;
		VkExtent2D		   extent		 = ChooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
			imageCount = swapChainSupport.capabilities.maxImageCount;


		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType		    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface		    = m_Ctx->m_WindowSurface;
		createInfo.minImageCount    = imageCount;
		createInfo.imageFormat	    = surfaceFormat.format;
		createInfo.imageColorSpace  = surfaceFormat.colorSpace;
		createInfo.imageExtent	    = extent;
		createInfo.imageArrayLayers = 1U;
		createInfo.imageUsage		= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		uint32_t queueFamilyIndices[] = { m_Ctx->m_QueueFamilies.graphicsFamily.value(), m_Ctx->m_QueueFamilies.presentFamily.value() };

		if (m_Ctx->m_QueueFamilies.graphicsFamily != m_Ctx->m_QueueFamilies.presentFamily)
		{
			createInfo.imageSharingMode		 = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2U;
			createInfo.pQueueFamilyIndices	 = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode		 = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices   = nullptr;
		}


		createInfo.preTransform	  = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode	  = presentMode;
		createInfo.clipped		  = VK_TRUE;
		createInfo.oldSwapchain	  = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(m_Ctx->m_LogicalDevice, &createInfo, nullptr, &m_Swapchain.swapchain) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::CreateSwapChain() - Failed to create swap chain!");

		vkGetSwapchainImagesKHR(m_Ctx->m_LogicalDevice, m_Swapchain.swapchain, &imageCount, nullptr);
		m_Swapchain.images.resize(imageCount);
		vkGetSwapchainImagesKHR(m_Ctx->m_LogicalDevice, m_Swapchain.swapchain, &imageCount, m_Swapchain.images.data());

		m_Swapchain.imageFormat = surfaceFormat.format;
		m_Swapchain.extent = extent;

		m_Swapchain.imageViews.resize(m_Swapchain.images.size());

		for (int i = 0; i < m_Swapchain.imageViews.size(); i++)
			Helpers::CreateImageView(m_Swapchain.images[i], m_Swapchain.imageViews[i], m_Swapchain.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	void VulkanRendererBackend::InitDepthPipeline()
	{
		m_DepthPipeline = std::make_unique<Pipeline>();

		Shader vShader("Shaders/DepthVertex.spv", ShaderType::Vertex);
		Shader fShader("Shaders/DepthFragment.spv", ShaderType::Fragment);

		VkPushConstantRange objectPushConstant{};
		objectPushConstant.offset	  = 0U;
		objectPushConstant.size		  = sizeof(PerObjectData);
		objectPushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		Pipeline::PipelineInfo pipelineInfo{};
		pipelineInfo.vShader			= &vShader;
		pipelineInfo.fShader			= &fShader;
		pipelineInfo.extent				= m_Swapchain.extent;
		pipelineInfo.descriptorLayouts  = { CameraMatricesBuffer::GetLayout() };
		pipelineInfo.pushConstantRanges = { objectPushConstant };
		pipelineInfo.enableDepthTest    = true;
		pipelineInfo.useVertexBindings  = true;
		pipelineInfo.renderPass		    = m_MainPass.get();
		pipelineInfo.subpassIndex	    = 0U;

		m_DepthPipeline->CreatePipeline(pipelineInfo);
	}

	void VulkanRendererBackend::CreateFramebuffers()
	{
		m_MainFramebuffers.resize(m_Swapchain.images.size());

		for (int i = 0; auto& framebuffer : m_MainFramebuffers)
		{
			framebuffer = std::make_unique<Framebuffer>();

			framebuffer->CreateFramebuffer(m_MainPass->m_RenderPass);

			Framebuffer::AttachmentInfo swapchain{};
			swapchain.format = m_Swapchain.imageFormat;

			Framebuffer::AttachmentInfo albedo{};
			albedo.format = m_Swapchain.imageFormat;

			Framebuffer::AttachmentInfo position{};
			position.format = VK_FORMAT_R16G16B16A16_SFLOAT;

			Framebuffer::AttachmentInfo normal{};
			normal.format = VK_FORMAT_R16G16B16A16_SFLOAT;

			Framebuffer::AttachmentInfo hdr{};
			hdr.format = VK_FORMAT_R16G16B16A16_SFLOAT;
			hdr.imageFinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			Framebuffer::AttachmentInfo postProcess{};
			postProcess.format = m_Swapchain.imageFormat;
			postProcess.imageFinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			Framebuffer::AttachmentInfo depth{};
			depth.format = FindDepthFormat();
			depth.imageFinalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depth.imageAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
			depth.imageUsageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

			framebuffer->CreateSampler();
			framebuffer->CreateAttachments({ swapchain, albedo, position, normal, hdr, postProcess, depth }, m_Swapchain.extent.width, m_Swapchain.extent.height);

			framebuffer->m_Attachments[0].Destroy();
			framebuffer->m_Attachments[0].format	  = m_Swapchain.imageFormat;
			framebuffer->m_Attachments[0].image		  = m_Swapchain.images[i];
			framebuffer->m_Attachments[0].imageMemory = VK_NULL_HANDLE;
			framebuffer->m_Attachments[0].imageView	  = m_Swapchain.imageViews[i++];
		}
	}

	void VulkanRendererBackend::CreateMainPass()
	{
		m_MainPass = std::make_unique<RenderPass>();

		RenderPass::Attachment swapchainAttachment
		{
			m_Swapchain.imageFormat, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0U
		};

		std::vector<RenderPass::Attachment> GBufferAttachments
		{
			{m_Swapchain.imageFormat, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1U},
			{VK_FORMAT_R16G16B16A16_SFLOAT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 2U},
			{VK_FORMAT_R16G16B16A16_SFLOAT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 3U}
		};

		RenderPass::Attachment lightingAttachment
		{
			VK_FORMAT_R16G16B16A16_SFLOAT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 4U
		};

		RenderPass::Attachment postProcessAttachment
		{
			m_Swapchain.imageFormat, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 4U
		};

		RenderPass::Attachment depthAttachment
		{
			FindDepthFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 5U
		};

		RenderPass::Subpass depthPass{};
		depthPass.srcSubpass = VK_SUBPASS_EXTERNAL;
		depthPass.dstSubpass = 0U;
		depthPass.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		depthPass.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		depthPass.srcAccessMask = 0U;
		depthPass.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		depthPass.depthAttachment = depthAttachment;

		RenderPass::Subpass GBufferPass{};
		GBufferPass.srcSubpass = 0U;
		GBufferPass.dstSubpass = 1U;
		GBufferPass.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		GBufferPass.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		GBufferPass.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		GBufferPass.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		GBufferPass.colorAttachments = GBufferAttachments;
		GBufferPass.depthAttachment = depthAttachment;

		RenderPass::Subpass lightingPass{};
		lightingPass.srcSubpass = 1U;
		lightingPass.dstSubpass = 2U;
		lightingPass.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		lightingPass.dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		lightingPass.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		lightingPass.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		lightingPass.colorAttachments = { lightingAttachment };

		RenderPass::Subpass postProcessPass{};
		postProcessPass.srcSubpass = 2U;
		postProcessPass.dstSubpass = VK_SUBPASS_EXTERNAL;
		postProcessPass.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		postProcessPass.dstStageMask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		postProcessPass.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		postProcessPass.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		postProcessPass.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		postProcessPass.colorAttachments = { postProcessAttachment };

		std::vector<RenderPass::Subpass> subpasses
		{ 
			depthPass, GBufferPass, lightingPass, postProcessPass 
		};

		m_MainPass = std::make_unique<RenderPass>();
		m_MainPass->CreateRenderPass(subpasses);
		m_MainPass->CreateSyncSemaphore();
	}

	void VulkanRendererBackend::InitGeometryPipeline()
	{
		m_GeometryPipeline = std::make_unique<Pipeline>();

		Shader vShader("Shaders/GeometryVertex.spv", ShaderType::Vertex);
		Shader fShader("Shaders/GeometryFragment.spv", ShaderType::Fragment);

		VkPushConstantRange objectPushConstant{};
		objectPushConstant.offset	  = 0U;
		objectPushConstant.size		  = sizeof(PerObjectData);
		objectPushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkPushConstantRange materialPushConstant{};
		materialPushConstant.offset = sizeof(PerObjectData);
		materialPushConstant.size = 24U;
		materialPushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		Pipeline::PipelineInfo pipelineInfo{};
		pipelineInfo.vShader			= &vShader;
		pipelineInfo.fShader			= &fShader;
		pipelineInfo.extent				= m_Swapchain.extent;
		pipelineInfo.descriptorLayouts  = { CameraMatricesBuffer::GetLayout(), Material::GetLayout() };
		pipelineInfo.pushConstantRanges = { objectPushConstant, materialPushConstant };
		pipelineInfo.enableDepthTest    = true;
		pipelineInfo.enableDepthWrite   = false;
		pipelineInfo.compareOp			= VK_COMPARE_OP_LESS_OR_EQUAL;
		pipelineInfo.useVertexBindings  = true;
		pipelineInfo.renderPass			= m_MainPass.get();
		pipelineInfo.subpassIndex		= 1U;
		
		m_GeometryPipeline->CreatePipeline(pipelineInfo); 
	}

	void VulkanRendererBackend::UpdateLightingInput()
	{
		PipelineInput::BufferInfo buffer{};
		buffer.buffer = m_Lights.buffer->GetHandle();
		buffer.index = 0U;
		buffer.size = sizeof(m_Lights.LBO);

		std::vector<PipelineInput::ImageInfo> empty{};

		if (!m_LightingInput)
			m_LightingInput = std::make_unique<PipelineInput>(empty, buffer);
		else
			m_LightingInput->UpdateDescriptorSet(empty, buffer);
	}
	void VulkanRendererBackend::InitLightingPipeline()
	{
		m_LightingPipeline = std::make_unique<Pipeline>();

		if(!m_Lights.buffer)
			m_Lights.buffer = std::make_unique<MemoryBuffer>(sizeof(m_Lights.LBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		UpdateLightingInput();

		VkPushConstantRange cameraPushConstant{};
		cameraPushConstant.offset	  = 0U;
		cameraPushConstant.size		  = sizeof(Lights::LightsCameraInfo);
		cameraPushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		Shader vShader("Shaders/FullscreenTriVert.spv", ShaderType::Vertex  );
		Shader fShader("Shaders/LightingFrag.spv"     , ShaderType::Fragment);

		Pipeline::PipelineInfo pipelineInfo{};
		pipelineInfo.vShader			= &vShader;
		pipelineInfo.fShader			= &fShader;
		pipelineInfo.extent				= m_Swapchain.extent;
		pipelineInfo.descriptorLayouts  = { m_LightingInput->m_DescriptorLayout };
		pipelineInfo.pushConstantRanges = { cameraPushConstant };
		pipelineInfo.cullMode			= VK_CULL_MODE_FRONT_BIT;
		pipelineInfo.renderPass = m_MainPass.get();

		m_LightingPipeline->CreatePipeline(pipelineInfo);
	}

	void VulkanRendererBackend::InitTonemappingPipeline()
	{
		m_TonemappingPipeline = std::make_unique<Pipeline>();

		VkPushConstantRange exposurePushConstant{};
		exposurePushConstant.offset		= 0U;
		exposurePushConstant.size		= sizeof(PostProcessingParams::Exposure);
		exposurePushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		Shader vShader("Shaders/FullscreenTriVert.spv", ShaderType::Vertex);
		Shader fShader("Shaders/TonemapFrag.spv", ShaderType::Fragment);

		Pipeline::PipelineInfo pipelineInfo{};
		pipelineInfo.vShader			= &vShader;
		pipelineInfo.fShader			= &fShader;
		pipelineInfo.extent				= m_Swapchain.extent;
		pipelineInfo.pushConstantRanges = { exposurePushConstant };
		pipelineInfo.cullMode		    = VK_CULL_MODE_FRONT_BIT;
		pipelineInfo.renderPass			= m_MainPass.get();

		m_TonemappingPipeline->CreatePipeline(pipelineInfo);
	}

	void VulkanRendererBackend::InitAntialiasingPipeline()
	{
		if (m_PostProcessParams.antialiasingMode == AntialiasingMode::None) return;

		m_AntialiasingPipeline = std::make_unique<Pipeline>();

		VkPushConstantRange antialiasingPushConstant{};
		antialiasingPushConstant.offset = 0U;
		antialiasingPushConstant.size = sizeof(PostProcessingParams::Antialiasing);
		antialiasingPushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::string fragmentSource = "Shaders/";

		if (m_PostProcessParams.antialiasingMode == AntialiasingMode::FXAA)
			fragmentSource.append("FXAA.spv");
		//else if(m_PostProcessParams.antialiasingMode == AntialiasingMode::SMAA)
			//fragmentSource.append("SMAA.spv");

		Shader vShader("Shaders/FullscreenTriVert.spv", ShaderType::Vertex);
		Shader fShader(fragmentSource, ShaderType::Fragment);

		Pipeline::PipelineInfo pipelineInfo{};
		pipelineInfo.vShader = &vShader;
		pipelineInfo.fShader = &fShader;
		pipelineInfo.extent  = m_Swapchain.extent;
		pipelineInfo.pushConstantRanges = { antialiasingPushConstant };
		pipelineInfo.cullMode			= VK_CULL_MODE_FRONT_BIT;
		pipelineInfo.renderPass		    = m_MainPass.get();

		m_AntialiasingPipeline->CreatePipeline(pipelineInfo);
	}

	void VulkanRendererBackend::CreateCommandBuffer()
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType				 = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool		 = m_Ctx->m_CommandPool;
		allocInfo.level				 = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;
		
		if (vkAllocateCommandBuffers(m_Ctx->m_LogicalDevice, &allocInfo, &m_CommandBuffer) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::GCreateCommandBuffer() - Failed to allocate command buffer!");
	}

	void VulkanRendererBackend::CreateSyncObjects()
	{
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		if (vkCreateFence(m_Ctx->m_LogicalDevice, &fenceInfo	, nullptr, &m_SubmitFence) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::CreateSyncObjects() - Failed to create a fence!");

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if(vkCreateSemaphore(m_Ctx->m_LogicalDevice, &semaphoreInfo, nullptr, &m_PresentSemaphore) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::CreateSyncObjects() - Failed to create a semaphore!");
	}
	
	void VulkanRendererBackend::CreateCameraMatricesBuffer()
	{
		m_CameraMatrices = std::make_unique<CameraMatricesBuffer>();
	}

	void ImGuiCheckResult(VkResult err)
	{
		if (err == 0) return;

		std::cerr << err << '\n';
		EN_ERROR("ImGui has caused a problem!");
	}
	void VulkanRendererBackend::InitImGui()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		ImGui::StyleColorsDark();

		VkAttachmentDescription colorAttachment{};
		colorAttachment.format		   = m_Swapchain.imageFormat;
		colorAttachment.samples		   = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp		   = VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp		   = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		colorAttachment.finalLayout	   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout	  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint	 = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments	 = &colorAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass	 = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass	 = 0;
		dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType		   = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments	   = &colorAttachment;
		renderPassInfo.subpassCount	   = 1;
		renderPassInfo.pSubpasses	   = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies   = &dependency;

		if (vkCreateRenderPass(m_Ctx->m_LogicalDevice, &renderPassInfo, nullptr, &m_ImGui.renderPass) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::InitImGui() - Failed to create ImGui's render pass!");

		VkDescriptorPoolSize poolSizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};
		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType		   = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags		   = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.maxSets	   = 1000 * IM_ARRAYSIZE(poolSizes);
		poolInfo.poolSizeCount = (uint32_t)IM_ARRAYSIZE(poolSizes);
		poolInfo.pPoolSizes    = poolSizes;

		if (vkCreateDescriptorPool(m_Ctx->m_LogicalDevice, &poolInfo, nullptr, &m_ImGui.descriptorPool) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::InitImGui() - Failed to create ImGui's descriptor pool!");

		ImGui_ImplGlfw_InitForVulkan(m_Window->m_GLFWWindow, true);
		ImGui_ImplVulkan_InitInfo initInfo = {};
		initInfo.Instance		 = m_Ctx->m_Instance;
		initInfo.PhysicalDevice  = m_Ctx->m_PhysicalDevice;
		initInfo.Device			 = m_Ctx->m_LogicalDevice;
		initInfo.QueueFamily	 = m_Ctx->m_QueueFamilies.graphicsFamily.value();
		initInfo.Queue			 = m_Ctx->m_GraphicsQueue;
		initInfo.PipelineCache   = VK_NULL_HANDLE;
		initInfo.DescriptorPool  = m_ImGui.descriptorPool;
		initInfo.Allocator		 = VK_NULL_HANDLE;
		initInfo.MinImageCount   = m_Swapchain.imageViews.size();
		initInfo.ImageCount		 = m_Swapchain.imageViews.size();
		initInfo.CheckVkResultFn = ImGuiCheckResult;
		ImGui_ImplVulkan_Init(&initInfo, m_ImGui.renderPass);

		VkCommandBuffer cmd = Helpers::BeginSingleTimeGraphicsCommands();
		ImGui_ImplVulkan_CreateFontsTexture(cmd);
		Helpers::EndSingleTimeGraphicsCommands(cmd);

		m_ImGui.commandBuffers.resize(m_Swapchain.imageViews.size());

		Helpers::CreateCommandPool(m_ImGui.commandPool, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		Helpers::CreateCommandBuffers(m_ImGui.commandBuffers.data(), static_cast<uint32_t>(m_ImGui.commandBuffers.size()), m_ImGui.commandPool);

		ImGui_ImplVulkanH_SelectSurfaceFormat(m_Ctx->m_PhysicalDevice, m_Ctx->m_WindowSurface, &m_Swapchain.imageFormat, 1, VK_COLORSPACE_SRGB_NONLINEAR_KHR);
	}
	
	SwapChainSupportDetails VulkanRendererBackend::QuerySwapChainSupport(VkPhysicalDevice& device)
	{
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Ctx->m_WindowSurface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Ctx->m_WindowSurface, &formatCount, nullptr);

		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Ctx->m_WindowSurface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Ctx->m_WindowSurface, &presentModeCount, nullptr);

		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Ctx->m_WindowSurface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}
	VkSurfaceFormatKHR VulkanRendererBackend::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return availableFormat;
		}

		return availableFormats[0];
	}
	VkExtent2D VulkanRendererBackend::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			return capabilities.currentExtent;
		else
		{
			int width, height;
			glfwGetFramebufferSize(m_Window->m_GLFWWindow, &width, &height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width  = std::clamp(actualExtent.width , capabilities.minImageExtent.width , capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}
	
	VkFormat VulkanRendererBackend::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(m_Ctx->m_PhysicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
				return format;

			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
				return format;
		}

		EN_ERROR("VulkanRendererBackend::FindSupportedFormat - Failed to find a supported format!");
	}
	VkFormat VulkanRendererBackend::FindDepthFormat()
	{
		return FindSupportedFormat
		(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	bool VulkanRendererBackend::HasStencilComponent(const VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

}