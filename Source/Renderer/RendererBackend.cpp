#include <Core/EnPch.hpp>
#include "RendererBackend.hpp"

namespace en
{
	Camera* g_DefaultCamera;

	VulkanRendererBackend* g_CurrentBackend;

	void VulkanRendererBackend::Init(RendererInfo& rendererInfo)
	{
		m_Ctx		   = &Context::GetContext();
		m_Window	   = &Window::GetMainWindow();
		m_RendererInfo = m_RendererInfo;

		g_CurrentBackend = this;

		CameraInfo cameraInfo{};
		cameraInfo.dynamicallyScaled = true;

		if (!g_DefaultCamera)
			g_DefaultCamera = new Camera(cameraInfo);

		m_MainCamera = g_DefaultCamera;

		glfwSetFramebufferSizeCallback(m_Window->m_GLFWWindow, VulkanRendererBackend::FramebufferResizeCallback);

        EN_SUCCESS("Init began!")

		CreateSwapchain();

        EN_SUCCESS("Created swapchain!")

		CreateGBufferAttachments();

        EN_SUCCESS("Created GBuffer attachments!")

		InitGeometryPipeline();

        EN_SUCCESS("Created geometry pipeline!")

		CreateGBuffer();

        EN_SUCCESS("Created GBuffer!")

		InitLightingPipeline();

        EN_SUCCESS("Created lighting pipeline!")

		InitPostProcessPipeline();

        EN_SUCCESS("Created PP pipeline!")

		CreateSwapchainFramebuffers();

        EN_SUCCESS("Created swapchain framebuffers!")

		CreateSyncObjects();

        EN_SUCCESS("Created sync objects!")

		CreateCameraMatricesBuffer();

        EN_SUCCESS("Created camera matrices buffer!")

		InitImGui();

		EN_SUCCESS("Successfully created the renderer Vulkan backend");

		m_Lights.pointLights[0].m_Position = glm::vec3(3, 2, 0.5);
		m_Lights.pointLights[0].m_Color    = glm::vec3(0.4, 1.0, 0.4);
		m_Lights.pointLights[0].m_Active   = true;

		m_Lights.pointLights[1].m_Position = glm::vec3(-2, 2, 2);
		m_Lights.pointLights[1].m_Color    = glm::vec3(1, 0.4, 0.4);
		m_Lights.pointLights[1].m_Active   = true;

		m_Lights.pointLights[2].m_Position = glm::vec3(0.4, 1.7, -2);
		m_Lights.pointLights[2].m_Color    = glm::vec3(0.2, 0.2, 1.0);
		m_Lights.pointLights[2].m_Active   = true;
	}
	VulkanRendererBackend::~VulkanRendererBackend()
	{
		vkDeviceWaitIdle(m_Ctx->m_LogicalDevice);

		m_GBuffer.Destroy(m_Ctx->m_LogicalDevice);

		m_Swapchain.Destroy(m_Ctx->m_LogicalDevice);

		m_GeometryPipeline.Destroy(m_Ctx->m_LogicalDevice);
		m_LightingPipeline.Destroy(m_Ctx->m_LogicalDevice);
		m_TonemappingPipeline.Destroy(m_Ctx->m_LogicalDevice);

		vkDestroyFence(m_Ctx->m_LogicalDevice, m_SubmitFence, nullptr);

		vkFreeDescriptorSets(m_Ctx->m_LogicalDevice, m_LightingPipeline.descriptorPool, 1, &m_LightingPipeline.descriptorSet);

		vkFreeCommandBuffers(m_Ctx->m_LogicalDevice, m_Ctx->m_CommandPool, 1, &m_CommandBuffer);

		en::Helpers::DestroyBuffer(m_Lights.buffer, m_Lights.bufferMemory);

		vkDestroyRenderPass(m_Ctx->m_LogicalDevice, m_ImGui.renderPass, nullptr);

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		vkDestroyDescriptorPool(m_Ctx->m_LogicalDevice, m_ImGui.descriptorPool, nullptr);
	}

	void VulkanRendererBackend::WaitForGPUIdle()
	{
		vkWaitForFences(m_Ctx->m_LogicalDevice, 1U, &m_SubmitFence, VK_TRUE, UINT64_MAX);
	}
	void VulkanRendererBackend::BeginRender()
	{
		VkResult result = vkAcquireNextImageKHR(m_Ctx->m_LogicalDevice, m_Swapchain.swapchain, UINT64_MAX, m_GeometryPipeline.passFinished, VK_NULL_HANDLE, &m_ImageIndex);

		m_SkipFrame = false;

		if (result == VK_ERROR_OUT_OF_DATE_KHR || m_FramebufferResized)
		{
			RecreateFramebuffer();
			m_SkipFrame = true;
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
			EN_ERROR("VulkanRendererBackend::BeginRender() - Failed to acquire swap chain image!");

		
		vkResetFences(m_Ctx->m_LogicalDevice, 1, &m_SubmitFence);

		vkResetCommandBuffer(m_CommandBuffer, 0);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(m_CommandBuffer, &beginInfo) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::BeginRender() - Failed to begin recording command buffer!");
	}
	void VulkanRendererBackend::GeometryPass()
	{
		if (m_SkipFrame) return;
		
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType	   = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass  = m_GeometryPipeline.renderPass;
		renderPassInfo.framebuffer = m_GBuffer.framebuffer;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_Swapchain.extent;

		std::array<VkClearValue, 4> clearValues;
		clearValues[0].color = m_RendererInfo.clearColor;
		clearValues[1].color = m_BlackClearValue.color;
		clearValues[2].color = m_BlackClearValue.color;

		clearValues[3].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(m_CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GeometryPipeline.pipeline);

		// Bind the m_MainCamera once per geometry pass
		m_MainCamera->Bind(m_CommandBuffer, m_GeometryPipeline.layout, m_CameraMatrices);

		for (const auto& object : m_RenderQueue)
		{
			// Bind object data (model matrix) once per SceneObject in the m_RenderQueue
			object->GetObjectData().Bind(m_CommandBuffer, m_GeometryPipeline.layout);

			for (const auto& subMesh : object->m_Mesh->m_SubMeshes)
			{
				// Bind SubMesh buffers and material once for each SubMesh in the sceneObject->m_Mesh->m_SubMeshes vector
				subMesh.m_VertexBuffer->Bind(m_CommandBuffer);
				subMesh.m_IndexBuffer ->Bind(m_CommandBuffer);
				subMesh.m_Material    ->Bind(m_CommandBuffer, m_GeometryPipeline.layout);

				vkCmdDrawIndexed(m_CommandBuffer, subMesh.m_IndexBuffer->GetSize(), 1, 0, 0, 0);
			}
		}
		m_RenderQueue.clear();

		vkCmdEndRenderPass(m_CommandBuffer);
	}
	void VulkanRendererBackend::LightingPass()
	{
		if (m_SkipFrame) return;
		
		Helpers::TransitionImageLayout(m_GBuffer.albedo.image   , m_GBuffer.albedo.format   , VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_CommandBuffer);
		Helpers::TransitionImageLayout(m_GBuffer.position.image , m_GBuffer.position.format , VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_CommandBuffer);
		Helpers::TransitionImageLayout(m_GBuffer.normal.image   , m_GBuffer.normal.format   , VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_CommandBuffer);
		Helpers::TransitionImageLayout(m_LightingHDRColorBuffer.image, m_LightingHDRColorBuffer.format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_CommandBuffer);
		
		bool lightsChanged = false;

		// Prepare lights and check if at least one value was changed. If so, update the buffer on the GPU
		for (int i = 0; i < MAX_LIGHTS; i++)
		{
			PointLight::Buffer& lightBuffer = m_Lights.LBO.lights[i];
			PointLight& light = m_Lights.pointLights[i];

			const glm::vec3& lightPos = light.m_Position;
			const glm::vec3  lightCol = light.m_Color * (float)light.m_Active * light.m_Intensity;
			const float      lightRad = light.m_Radius * (float)light.m_Active;

			if (lightBuffer.position != lightPos || lightBuffer.color != lightCol || lightBuffer.radius != lightRad)
				lightsChanged = true;

			lightBuffer.position = lightPos;
			lightBuffer.color    = lightCol;
			lightBuffer.radius   = lightRad;
		}

		m_Lights.camera.viewPos   = m_MainCamera->m_Position;
		m_Lights.camera.debugMode = m_DebugMode;
		
		if (lightsChanged)
			en::Helpers::MapBuffer(m_Lights.bufferMemory, &m_Lights.LBO, m_Lights.bufferSize);

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType			 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass		 = m_LightingPipeline.renderPass;
		renderPassInfo.framebuffer		 = m_LightingHDRFramebuffer;
		renderPassInfo.renderArea.offset = { 0U, 0U };
		renderPassInfo.renderArea.extent = m_Swapchain.extent;
		renderPassInfo.clearValueCount	 = 1U;
		renderPassInfo.pClearValues		 = &m_BlackClearValue;

		vkCmdBeginRenderPass(m_CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_LightingPipeline.pipeline);

		vkCmdPushConstants(m_CommandBuffer, m_LightingPipeline.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0U, sizeof(Lights::LightsCameraInfo), &m_Lights.camera);

		vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_LightingPipeline.layout, 0U, 1U, &m_LightingPipeline.descriptorSet, 0U, nullptr);

		vkCmdDraw(m_CommandBuffer, 3U, 1U, 0U, 0U);

		vkCmdEndRenderPass(m_CommandBuffer);
	}
	void VulkanRendererBackend::PostProcessPass()
	{
		if (m_SkipFrame) return;

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType			 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass		 = m_TonemappingPipeline.renderPass;
		renderPassInfo.framebuffer		 = m_Swapchain.framebuffers[m_ImageIndex];
		renderPassInfo.renderArea.offset = { 0U, 0U };
		renderPassInfo.renderArea.extent = m_Swapchain.extent;
		renderPassInfo.clearValueCount   = 1U;
		renderPassInfo.pClearValues		 = &m_BlackClearValue;
		
		vkCmdBeginRenderPass(m_CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_TonemappingPipeline.pipeline);

		static PostProcessingParams::Exposure exposure { m_MainCamera->m_Exposure };

		vkCmdPushConstants(m_CommandBuffer, m_TonemappingPipeline.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PostProcessingParams::Exposure), &exposure);

		vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_TonemappingPipeline.layout, 0U, 1U, &m_TonemappingPipeline.descriptorSet, 0U, nullptr);

		vkCmdDraw(m_CommandBuffer, 3U, 1U, 0U, 0U);

		vkCmdEndRenderPass(m_CommandBuffer);
	}
	void VulkanRendererBackend::ImGuiPass()
	{
		if (m_SkipFrame || m_ImGuiRenderCallback == nullptr) return;

		m_ImGuiRenderCallback();

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType			 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass		 = m_ImGui.renderPass;
		renderPassInfo.framebuffer		 = m_Swapchain.framebuffers[m_ImageIndex];
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
		submitInfo.pWaitSemaphores    = &m_GeometryPipeline.passFinished;
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
		submitInfo.pSignalSemaphores    = &m_LightingPipeline.passFinished;

		if (vkQueueSubmit(m_Ctx->m_GraphicsQueue, 1U, &submitInfo, m_SubmitFence) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::EndRender() - Failed to submit command buffer!");

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1U;
		presentInfo.pWaitSemaphores	   = &m_LightingPipeline.passFinished;

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

	void VulkanRendererBackend::EnqueueSceneObject(SceneObject* sceneObject)
	{
		m_RenderQueue.emplace_back(sceneObject);
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

		m_GBuffer.Destroy(m_Ctx->m_LogicalDevice);
		m_Swapchain.Destroy(m_Ctx->m_LogicalDevice);

		vkDestroyPipeline(m_Ctx->m_LogicalDevice, m_TonemappingPipeline.pipeline, nullptr);
		vkDestroyPipelineLayout(m_Ctx->m_LogicalDevice, m_TonemappingPipeline.layout, nullptr);
		vkDestroyRenderPass(m_Ctx->m_LogicalDevice, m_TonemappingPipeline.renderPass, nullptr);

		vkDestroyPipeline(m_Ctx->m_LogicalDevice, m_LightingPipeline.pipeline, nullptr);
		vkDestroyPipelineLayout(m_Ctx->m_LogicalDevice, m_LightingPipeline.layout, nullptr);
		vkDestroyRenderPass(m_Ctx->m_LogicalDevice, m_LightingPipeline.renderPass, nullptr);

		vkDestroyPipeline(m_Ctx->m_LogicalDevice, m_GeometryPipeline.pipeline, nullptr);
		vkDestroyPipelineLayout(m_Ctx->m_LogicalDevice, m_GeometryPipeline.layout, nullptr);
		vkDestroyRenderPass(m_Ctx->m_LogicalDevice, m_GeometryPipeline.renderPass, nullptr);

		m_LightingHDRColorBuffer.Destroy(m_Ctx->m_LogicalDevice);
		vkDestroyFramebuffer(m_Ctx->m_LogicalDevice, m_LightingHDRFramebuffer, nullptr);

		CreateSwapchain();

		CreateGBufferAttachments();
		
		GCreateRenderPass();
		GCreatePipeline();

		CreateGBuffer();

		LCreateRenderPass();
		LCreateHDRFramebuffer();
		LCreatePipeline();

		PPCreateRenderPass();
		PPCreatePipeline();

		CreateSwapchainFramebuffers();

		LCreateDescriptorSet();
		PPCreateDescriptorSet();

		ImGui_ImplVulkan_SetMinImageCount(m_Swapchain.imageViews.size());

		m_FramebufferResized = false;
	}

	void VulkanRendererBackend::ReloadBackend()
	{
		g_CurrentBackend->m_FramebufferResized = true;
	}

	void VulkanRendererBackend::SetMainCamera(Camera* camera)
	{
		if (camera)
			m_MainCamera = camera;
		else
			m_MainCamera = g_DefaultCamera;
	}
	Camera* VulkanRendererBackend::GetMainCamera()
	{
		return m_MainCamera;
	}

	std::array<PointLight, MAX_LIGHTS>& VulkanRendererBackend::GetPointLights()
	{
		return m_Lights.pointLights;
	}

	void VulkanRendererBackend::CreateSwapchain()
	{
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_Ctx->m_PhysicalDevice);

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR   presentMode   = VSYNC ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR;
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
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage		= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		Helpers::QueueFamilyIndices indices = Helpers::FindQueueFamilies(m_Ctx->m_PhysicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily)
		{
			createInfo.imageSharingMode		 = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
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
	void VulkanRendererBackend::CreateSwapchainFramebuffers()
	{
		m_Swapchain.framebuffers.resize(m_Swapchain.imageViews.size());

		for (int i = 0; i < m_Swapchain.framebuffers.size(); i++)
		{
			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType			= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass		= m_TonemappingPipeline.renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments	= &m_Swapchain.imageViews[i];
			framebufferInfo.width			= m_Swapchain.extent.width;
			framebufferInfo.height			= m_Swapchain.extent.height;
			framebufferInfo.layers			= 1;

			if (vkCreateFramebuffer(m_Ctx->m_LogicalDevice, &framebufferInfo, nullptr, &m_Swapchain.framebuffers[i]) != VK_SUCCESS)
				EN_ERROR("VulkanRendererBackend::CreateSwapchainFramebuffers() - Failed to create framebuffers!");
		}
	}

	void VulkanRendererBackend::CreateGBuffer()
	{
		std::array<VkImageView, 4> attachments = 
		{
			m_GBuffer.albedo.imageView,
			m_GBuffer.position.imageView,
			m_GBuffer.normal.imageView,
			m_GBuffer.depth.imageView
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType			= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass	    = m_GeometryPipeline.renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments    = attachments.data();
		framebufferInfo.width  = m_Swapchain.extent.width;
		framebufferInfo.height = m_Swapchain.extent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_Ctx->m_LogicalDevice, &framebufferInfo, nullptr, &m_GBuffer.framebuffer) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::InitGBuffer() - Failed to create the GBuffer!");

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType			 = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter		 = VK_FILTER_LINEAR;
		samplerInfo.minFilter		 = VK_FILTER_LINEAR;
		samplerInfo.addressModeU	 = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV	 = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW	 = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy	 = 0;
		samplerInfo.borderColor		 = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp	  = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode	  = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias	  = 0.0f;
		samplerInfo.minLod		  = 0.0f;
		samplerInfo.maxLod		  = 0.0f;

		if (vkCreateSampler(m_Ctx->m_LogicalDevice, &samplerInfo, nullptr, &m_GBuffer.sampler) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::InitGBuffer() - Failed to create texture sampler!");
	}
	void VulkanRendererBackend::CreateGBufferAttachments()
	{
		CreateAttachment(m_GBuffer.albedo  , m_Swapchain.imageFormat	  , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		CreateAttachment(m_GBuffer.position, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		CreateAttachment(m_GBuffer.normal  , VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		
		CreateAttachment(m_GBuffer.depth, FindDepthFormat(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}
	void VulkanRendererBackend::CreateAttachment(Attachment& attachment, VkFormat format, VkImageLayout imageLayout, VkImageAspectFlags imageAspectFlags, VkImageUsageFlags imageUsageFlags)
	{
		Helpers::CreateImage(m_Swapchain.extent.width, m_Swapchain.extent.height, format, VK_IMAGE_TILING_OPTIMAL, imageUsageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, attachment.image, attachment.imageMemory);
		Helpers::CreateImageView(attachment.image, attachment.imageView, format, imageAspectFlags);
		Helpers::TransitionImageLayout(attachment.image, format, imageAspectFlags, VK_IMAGE_LAYOUT_UNDEFINED, imageLayout);
		attachment.format = format;
	}

	void VulkanRendererBackend::InitGeometryPipeline()
	{
		GCreateCommandBuffer();
		GCreateRenderPass();
		GCreatePipeline();
	}
	void VulkanRendererBackend::InitLightingPipeline()
	{
		LCreateRenderPass();
		LCreateHDRFramebuffer();
		LCreateLightsBuffer();
		LCreateDescriptorPool();
		LCreateDescriptorSetLayout();
		LCreateDescriptorSet();
		LCreatePipeline();
	}
	void VulkanRendererBackend::InitPostProcessPipeline()
	{
		PPCreateRenderPass();
		PPCreateDescriptorPool();
		PPCreateDescriptorSetLayout();
		PPCreateDescriptorSet();
		PPCreatePipeline();
	}

	void VulkanRendererBackend::GCreateCommandBuffer()
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType				 = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool		 = m_Ctx->m_CommandPool;
		allocInfo.level				 = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;
		
		if (vkAllocateCommandBuffers(m_Ctx->m_LogicalDevice, &allocInfo, &m_CommandBuffer) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::GCreateCommandBuffer() - Failed to allocate command buffer!");
	}
	void VulkanRendererBackend::GCreateRenderPass()
	{
		VkAttachmentDescription albedoAttachment{};
		albedoAttachment.format		    = m_GBuffer.albedo.format;
		albedoAttachment.samples	    = VK_SAMPLE_COUNT_1_BIT;
		albedoAttachment.loadOp		    = VK_ATTACHMENT_LOAD_OP_CLEAR;
		albedoAttachment.storeOp	    = VK_ATTACHMENT_STORE_OP_STORE;
		albedoAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		albedoAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		albedoAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		albedoAttachment.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference albedoAttachmentRef{};
		albedoAttachmentRef.attachment = 0;
		albedoAttachmentRef.layout	   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription positionAttachment{};
		positionAttachment.format		  = m_GBuffer.position.format;
		positionAttachment.samples		  = VK_SAMPLE_COUNT_1_BIT;
		positionAttachment.loadOp		  = VK_ATTACHMENT_LOAD_OP_CLEAR;
		positionAttachment.storeOp		  = VK_ATTACHMENT_STORE_OP_STORE;
		positionAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		positionAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		positionAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		positionAttachment.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference positionAttachmentRef{};
		positionAttachmentRef.attachment = 1;
		positionAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription normalAttachment{};
		normalAttachment.format		    = m_GBuffer.normal.format;
		normalAttachment.samples	    = VK_SAMPLE_COUNT_1_BIT;
		normalAttachment.loadOp		    = VK_ATTACHMENT_LOAD_OP_CLEAR;
		normalAttachment.storeOp	    = VK_ATTACHMENT_STORE_OP_STORE;
		normalAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		normalAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		normalAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		normalAttachment.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference normalAttachmentRef{};
		normalAttachmentRef.attachment = 2;
		normalAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format		   = m_GBuffer.depth.format;
		depthAttachment.samples		   = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp		   = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp		   = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 3;
		depthAttachmentRef.layout	  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		std::array<VkAttachmentReference, 3> colorAttachments = {
			albedoAttachmentRef,
			positionAttachmentRef,
			normalAttachmentRef
		};

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount	= static_cast<uint32_t>(colorAttachments.size());
		subpass.pColorAttachments		= colorAttachments.data();
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass	 = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass	 = 0;
		dependency.srcAccessMask = 0;
		dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 4> attachments = {
			albedoAttachment,
			positionAttachment,
			normalAttachment,
			depthAttachment
		};

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments    = attachments.data();
		renderPassInfo.subpassCount    = 1;
		renderPassInfo.pSubpasses      = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies   = &dependency;

		if (vkCreateRenderPass(m_Ctx->m_LogicalDevice, &renderPassInfo, nullptr, &m_GeometryPipeline.renderPass) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::GCreateRenderPass() - Failed to create render pass!");
	}
	void VulkanRendererBackend::GCreatePipeline()
	{
		auto bindingDescription    = Vertex::GetBindingDescription();
		auto attributeDescriptions = Vertex::GetAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType						    = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount   = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions	    = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType					 = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology				 = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x		  = 0.0f;
		viewport.y		  = 0.0f;
		viewport.width    = (float)m_Swapchain.extent.width;
		viewport.height   = (float)m_Swapchain.extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = m_Swapchain.extent;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType			= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports	= &viewport;
		viewportState.scissorCount  = 1;
		viewportState.pScissors		= &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType				   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable		   = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode			   = m_RendererInfo.polygonMode;
		rasterizer.lineWidth			   = 1.0f;
		rasterizer.cullMode				   = m_RendererInfo.cullMode;
		rasterizer.frontFace			   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable		   = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType				   = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable  = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask		 = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable		 = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp		 = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp		 = VK_BLEND_OP_ADD;

		std::array<VkPipelineColorBlendAttachmentState, 3> colorBlendAttachments = {
			colorBlendAttachment,
			colorBlendAttachment,
			colorBlendAttachment
		};

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType			    = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable     = VK_FALSE;
		colorBlending.logicOp		    = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount   = static_cast<uint32_t>(colorBlendAttachments.size());
		colorBlending.pAttachments		= colorBlendAttachments.data();
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType				   = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable	   = VK_TRUE;
		depthStencil.depthWriteEnable	   = VK_TRUE;
		depthStencil.depthCompareOp		   = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds		   = 0.0f;
		depthStencil.maxDepthBounds		   = 1.0f;
		depthStencil.stencilTestEnable	   = VK_FALSE;
		depthStencil.front = {};
		depthStencil.back  = {};

		std::array<VkDescriptorSetLayout, 2> descriptorLayouts = { CameraMatricesBuffer::GetLayout(), Material::GetLayout() };
		
		VkPushConstantRange objectPushConstant;
		objectPushConstant.offset = 0U;
		objectPushConstant.size   = sizeof(PerObjectData);
		objectPushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType		  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorLayouts.size());
		pipelineLayoutInfo.pSetLayouts	  = descriptorLayouts.data();
		pipelineLayoutInfo.pPushConstantRanges = &objectPushConstant;
		pipelineLayoutInfo.pushConstantRangeCount = 1U;

		if (vkCreatePipelineLayout(m_Ctx->m_LogicalDevice, &pipelineLayoutInfo, nullptr, &m_GeometryPipeline.layout) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::GCreatePipeline() - Failed to create pipeline layout!");

		Shader vShader("Shaders/GeometryVertex.spv", ShaderType::Vertex);
		Shader fShader("Shaders/GeometryFragment.spv", ShaderType::Fragment);

		VkPipelineShaderStageCreateInfo shaderStages[] = { vShader.m_ShaderInfo, fShader.m_ShaderInfo };

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType		= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2U;
		pipelineInfo.pStages	= shaderStages;

		pipelineInfo.pVertexInputState   = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState		 = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState   = &multisampling;
		pipelineInfo.pDepthStencilState  = &depthStencil;
		pipelineInfo.pColorBlendState    = &colorBlending;
		pipelineInfo.pDynamicState		 = nullptr;

		pipelineInfo.layout		= m_GeometryPipeline.layout;
		pipelineInfo.renderPass = m_GeometryPipeline.renderPass;
		pipelineInfo.subpass	= 0;

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex  = -1;

		if (vkCreateGraphicsPipelines(m_Ctx->m_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GeometryPipeline.pipeline) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::GCreatePipeline() - Failed to create graphics pipeline!");
	}

	void VulkanRendererBackend::LCreateLightsBuffer()
	{
		m_Lights.bufferSize = sizeof(m_Lights.LBO);
		en::Helpers::CreateBuffer(m_Lights.bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_Lights.buffer, m_Lights.bufferMemory);
	}
	void VulkanRendererBackend::LCreateDescriptorSetLayout()
	{
		std::array<VkDescriptorSetLayoutBinding, 4> bindings;
		//0: Color Framebuffer
		//1: Position Framebuffer (Alpha channel is specularity)
		//2: Normal Framebuffer
		//3: Lights Buffer
		
		for (uint32_t i = 0U; auto & binding : bindings)
		{
			binding.binding			   = i;
			binding.descriptorCount	   = 1U;
			binding.descriptorType	   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			binding.pImmutableSamplers = nullptr;
			binding.stageFlags		   = VK_SHADER_STAGE_FRAGMENT_BIT;

			i++;
		}

		// Change the light buffer's type to VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
		bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType		= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings    = bindings.data();

		if (vkCreateDescriptorSetLayout(m_Ctx->m_LogicalDevice, &layoutInfo, nullptr, &m_LightingPipeline.descriptorSetLayout) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::LCreateDescriptorSetLayout() - Failed to create descriptor set layout!");

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType				 = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool	 = m_LightingPipeline.descriptorPool;
		allocInfo.descriptorSetCount = 1U;
		allocInfo.pSetLayouts		 = &m_LightingPipeline.descriptorSetLayout;

		if (vkAllocateDescriptorSets(m_Ctx->m_LogicalDevice, &allocInfo, &m_LightingPipeline.descriptorSet) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::LCreateDescriptorSetLayout() - Failed to allocate descriptor set!");
	}
	void VulkanRendererBackend::LCreateDescriptorPool()
	{
		std::array<VkDescriptorPoolSize, 4> poolSizes{};

		for (auto& poolSize : poolSizes)
		{
			poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSize.descriptorCount = 1U;
		}

		// Lights buffer
		poolSizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType		   = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes	   = poolSizes.data();
		poolInfo.maxSets	   = 1U;

		if (vkCreateDescriptorPool(m_Ctx->m_LogicalDevice, &poolInfo, nullptr, &m_LightingPipeline.descriptorPool) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::LCreateDescriptorPool() - Failed to create descriptor pool!");
	}
	void VulkanRendererBackend::LCreateDescriptorSet()
	{
		std::array<VkDescriptorImageInfo, 3> imageInfos{};

		// Albedo
		imageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfos[0].imageView	  = m_GBuffer.albedo.imageView;
		imageInfos[0].sampler	  = m_GBuffer.sampler;

		// Position
		imageInfos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfos[1].imageView   = m_GBuffer.position.imageView;
		imageInfos[1].sampler	  = m_GBuffer.sampler;

		// Normal
		imageInfos[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfos[2].imageView   = m_GBuffer.normal.imageView;
		imageInfos[2].sampler	  = m_GBuffer.sampler;

		// Light Buffer
		VkDescriptorBufferInfo lightBufferInfo{};
		lightBufferInfo.buffer = m_Lights.buffer;
		lightBufferInfo.offset = 0U;
		lightBufferInfo.range  = sizeof(m_Lights.LBO);

		std::array<VkWriteDescriptorSet, 4> descriptorWrites{};

		for (uint32_t i = 0U; auto & descriptorWrite : descriptorWrites)
		{
			descriptorWrite.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet			= m_LightingPipeline.descriptorSet;
			descriptorWrite.dstBinding		= i;
			descriptorWrite.dstArrayElement = 0U;
			descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrite.descriptorCount = 1U;

			if(i!=3U)
				descriptorWrite.pImageInfo  = &imageInfos[i];

			i++;
		}

		// Light Buffer
		descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[3].pBufferInfo    = &lightBufferInfo;

		vkUpdateDescriptorSets(m_Ctx->m_LogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
	void VulkanRendererBackend::LCreateRenderPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format		   = VK_FORMAT_R16G16B16A16_SFLOAT;
		colorAttachment.samples		   = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp		   = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp		   = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0U;
		colorAttachmentRef.layout	  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint	    = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount    = 1U;
		subpass.pColorAttachments	    = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = nullptr;

		VkSubpassDependency dependency{};
		dependency.srcSubpass	 = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass	 = 0U;
		dependency.srcAccessMask = 0U;
		dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType		   = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1U;
		renderPassInfo.pAttachments	   = &colorAttachment;
		renderPassInfo.subpassCount	   = 1U;
		renderPassInfo.pSubpasses	   = &subpass;
		renderPassInfo.dependencyCount = 1U;
		renderPassInfo.pDependencies   = &dependency;

		if (vkCreateRenderPass(m_Ctx->m_LogicalDevice, &renderPassInfo, nullptr, &m_LightingPipeline.renderPass) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::LCreateRenderPass() - Failed to create render pass!");
	}
	void VulkanRendererBackend::LCreatePipeline()
	{
		Shader vShader("Shaders/FullscreenTriVert.spv", ShaderType::Vertex);
		Shader fShader("Shaders/LightingFrag.spv", ShaderType::Fragment);

		VkPipelineShaderStageCreateInfo shaderStages[] = { vShader.m_ShaderInfo, fShader.m_ShaderInfo };

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType							= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount	= 0U;
		vertexInputInfo.vertexAttributeDescriptionCount = 0U;
		vertexInputInfo.pVertexBindingDescriptions		= nullptr;
		vertexInputInfo.pVertexAttributeDescriptions	= nullptr;

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType					 = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology				 = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x		  = 0.0f;
		viewport.y		  = 0.0f;
		viewport.width	  = (float)m_Swapchain.extent.width;
		viewport.height	  = (float)m_Swapchain.extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = m_Swapchain.extent;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType			= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports	= &viewport;
		viewportState.scissorCount	= 1;
		viewportState.pScissors		= &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType				   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable		   = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode			   = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth			   = 1.0f;
		rasterizer.cullMode				   = VK_CULL_MODE_FRONT_BIT;
		rasterizer.frontFace			   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable		   = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType				   = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable  = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask		 = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable		 = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp		 = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp		 = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType			  = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable	  = VK_FALSE;
		colorBlending.logicOp		  = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments    = &colorBlendAttachment;

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType				   = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable	   = VK_FALSE;
		depthStencil.depthWriteEnable	   = VK_FALSE;
		depthStencil.depthCompareOp		   = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds	       = 0.0f;
		depthStencil.maxDepthBounds	       = 1.0f;
		depthStencil.stencilTestEnable	   = VK_FALSE;
		depthStencil.front = {};
		depthStencil.back  = {};

		VkPushConstantRange lightsCameraInfoRange{};
		lightsCameraInfoRange.offset = 0U;
		lightsCameraInfoRange.size = sizeof(Lights::LightsCameraInfo);
		lightsCameraInfoRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType		  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1U;
		pipelineLayoutInfo.pSetLayouts	  = &m_LightingPipeline.descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1U;
		pipelineLayoutInfo.pPushConstantRanges    = &lightsCameraInfoRange;

		if (vkCreatePipelineLayout(m_Ctx->m_LogicalDevice, &pipelineLayoutInfo, nullptr, &m_LightingPipeline.layout) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::LCreatePipeline() - Failed to create pipeline layout!");

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType		= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2U;
		pipelineInfo.pStages	= shaderStages;

		pipelineInfo.pVertexInputState   = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState		 = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState   = &multisampling;
		pipelineInfo.pDepthStencilState  = &depthStencil;
		pipelineInfo.pColorBlendState    = &colorBlending;
		pipelineInfo.pDynamicState		 = nullptr;

		pipelineInfo.layout		= m_LightingPipeline.layout;
		pipelineInfo.renderPass = m_LightingPipeline.renderPass;
		pipelineInfo.subpass	= 0;

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex  = -1;

		if (vkCreateGraphicsPipelines(m_Ctx->m_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_LightingPipeline.pipeline) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::LCreatePipeline() - Failed to create pipeline!");
	}
	void VulkanRendererBackend::LCreateHDRFramebuffer()
	{
		CreateAttachment(m_LightingHDRColorBuffer, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType			= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass		= m_LightingPipeline.renderPass;
		framebufferInfo.attachmentCount = 1U;
		framebufferInfo.pAttachments	= &m_LightingHDRColorBuffer.imageView;
		framebufferInfo.width			= m_Swapchain.extent.width;
		framebufferInfo.height			= m_Swapchain.extent.height;
		framebufferInfo.layers			= 1U;

		if (vkCreateFramebuffer(m_Ctx->m_LogicalDevice, &framebufferInfo, nullptr, &m_LightingHDRFramebuffer) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::LCreateHDRFramebuffer() - Failed to create the GBuffer!");
	}

	void VulkanRendererBackend::PPCreateDescriptorSetLayout()
	{
		VkDescriptorSetLayoutBinding binding;

		binding.binding			   = 0U;
		binding.descriptorCount	   = 1U;
		binding.descriptorType	   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding.pImmutableSamplers = nullptr;
		binding.stageFlags		   = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType		= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1U;
		layoutInfo.pBindings	= &binding;

		if (vkCreateDescriptorSetLayout(m_Ctx->m_LogicalDevice, &layoutInfo, nullptr, &m_TonemappingPipeline.descriptorSetLayout) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::PPCreateDescriptorSetLayout() - Failed to create descriptor set layout!");

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType				 = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool	 = m_TonemappingPipeline.descriptorPool;
		allocInfo.descriptorSetCount = 1U;
		allocInfo.pSetLayouts		 = &m_TonemappingPipeline.descriptorSetLayout;

		if (vkAllocateDescriptorSets(m_Ctx->m_LogicalDevice, &allocInfo, &m_TonemappingPipeline.descriptorSet) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::PPCreateDescriptorSetLayout() - Failed to allocate descriptor set!");
	}
	void VulkanRendererBackend::PPCreateDescriptorPool()
	{
		VkDescriptorPoolSize poolSize{};
		poolSize.type			 = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSize.descriptorCount = 1U;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType		   = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1U;
		poolInfo.pPoolSizes	   = &poolSize;
		poolInfo.maxSets	   = 1U;

		if (vkCreateDescriptorPool(m_Ctx->m_LogicalDevice, &poolInfo, nullptr, &m_TonemappingPipeline.descriptorPool) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::PPCreateDescriptorPool() - Failed to create descriptor pool!");
	}
	void VulkanRendererBackend::PPCreateDescriptorSet()
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView   = m_LightingHDRColorBuffer.imageView;
		imageInfo.sampler	  = m_GBuffer.sampler;

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet			= m_TonemappingPipeline.descriptorSet;
		descriptorWrite.dstBinding		= 0U;
		descriptorWrite.dstArrayElement = 0U;
		descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrite.descriptorCount = 1U;
		descriptorWrite.pImageInfo		= &imageInfo;

		vkUpdateDescriptorSets(m_Ctx->m_LogicalDevice, 1U, &descriptorWrite, 0, nullptr);
	}
	void VulkanRendererBackend::PPCreateRenderPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format		   = m_Swapchain.imageFormat;
		colorAttachment.samples		   = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp		   = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp		   = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0U;
		colorAttachmentRef.layout	  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount	= 1U;
		subpass.pColorAttachments		= &colorAttachmentRef;
		subpass.pDepthStencilAttachment = nullptr;

		VkSubpassDependency dependency{};
		dependency.srcSubpass	 = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass	 = 0U;
		dependency.srcAccessMask = 0U;
		dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType		   = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1U;
		renderPassInfo.pAttachments	   = &colorAttachment;
		renderPassInfo.subpassCount	   = 1U;
		renderPassInfo.pSubpasses	   = &subpass;
		renderPassInfo.dependencyCount = 1U;
		renderPassInfo.pDependencies   = &dependency;

		if (vkCreateRenderPass(m_Ctx->m_LogicalDevice, &renderPassInfo, nullptr, &m_TonemappingPipeline.renderPass) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::PPCreateRenderPass() - Failed to create render pass!");
	}
	void VulkanRendererBackend::PPCreatePipeline()
	{
		Shader vShader("Shaders/FullscreenTriVert.spv", ShaderType::Vertex  );
		Shader fShader("Shaders/TonemapFrag.spv"	  , ShaderType::Fragment);

		VkPipelineShaderStageCreateInfo shaderStages[] = { vShader.m_ShaderInfo, fShader.m_ShaderInfo };

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType							= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount   = 0U;
		vertexInputInfo.vertexAttributeDescriptionCount = 0U;
		vertexInputInfo.pVertexBindingDescriptions		= nullptr;
		vertexInputInfo.pVertexAttributeDescriptions	= nullptr;

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType					 = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology				 = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x		  = 0.0f;
		viewport.y		  = 0.0f;
		viewport.width	  = (float)m_Swapchain.extent.width;
		viewport.height	  = (float)m_Swapchain.extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0U, 0U };
		scissor.extent = m_Swapchain.extent;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType			= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1U;
		viewportState.pViewports	= &viewport;
		viewportState.scissorCount  = 1U;
		viewportState.pScissors		= &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType				   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable		   = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode			   = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth			   = 1.0f;
		rasterizer.cullMode				   = VK_CULL_MODE_FRONT_BIT;
		rasterizer.frontFace			   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable		   = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType				   = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable  = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask		 = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable		 = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp		 = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp		 = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType			  = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable	  = VK_FALSE;
		colorBlending.logicOp		  = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1U;
		colorBlending.pAttachments	  = &colorBlendAttachment;

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType				   = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable	   = VK_FALSE;
		depthStencil.depthWriteEnable	   = VK_FALSE;
		depthStencil.depthCompareOp		   = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds		   = 0.0f;
		depthStencil.maxDepthBounds		   = 1.0f;
		depthStencil.stencilTestEnable	   = VK_FALSE;
		depthStencil.front = {};
		depthStencil.back  = {};

		VkPushConstantRange exposurePushConstant{};
		exposurePushConstant.offset		= 0U;
		exposurePushConstant.size		= sizeof(PostProcessingParams::Exposure);
		exposurePushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType		  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1U;
		pipelineLayoutInfo.pSetLayouts	  = &m_TonemappingPipeline.descriptorSetLayout;

		pipelineLayoutInfo.pPushConstantRanges	  = &exposurePushConstant;
		pipelineLayoutInfo.pushConstantRangeCount = 1U;

		if (vkCreatePipelineLayout(m_Ctx->m_LogicalDevice, &pipelineLayoutInfo, nullptr, &m_TonemappingPipeline.layout) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::PPCreatePipeline() - Failed to create pipeline layout!");

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType		= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2U;
		pipelineInfo.pStages	= shaderStages;

		pipelineInfo.pVertexInputState   = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState		 = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState   = &multisampling;
		pipelineInfo.pDepthStencilState  = &depthStencil;
		pipelineInfo.pColorBlendState    = &colorBlending;
		pipelineInfo.pDynamicState		 = nullptr;

		pipelineInfo.layout		= m_TonemappingPipeline.layout;
		pipelineInfo.renderPass = m_TonemappingPipeline.renderPass;
		pipelineInfo.subpass	= 0U;

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(m_Ctx->m_LogicalDevice, VK_NULL_HANDLE, 1U, &pipelineInfo, nullptr, &m_TonemappingPipeline.pipeline) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::PPCreatePipeline() - Failed to create pipeline!");
	}
	
	void VulkanRendererBackend::CreateSyncObjects()
	{
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		if (vkCreateSemaphore(m_Ctx->m_LogicalDevice, &semaphoreInfo, nullptr, &m_GeometryPipeline.passFinished) != VK_SUCCESS ||
			vkCreateSemaphore(m_Ctx->m_LogicalDevice, &semaphoreInfo, nullptr, &m_LightingPipeline.passFinished) != VK_SUCCESS ||
			vkCreateFence	 (m_Ctx->m_LogicalDevice, &fenceInfo	, nullptr, &m_SubmitFence				   ) != VK_SUCCESS)
		{
			EN_ERROR("VulkanRendererBackend::CreateSyncObjects() - Failed to create sync objects!");
		}
	}

	void VulkanRendererBackend::CreateCameraMatricesBuffer()
	{
		m_CameraMatrices = new CameraMatricesBuffer;
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
		initInfo.QueueFamily	 = Helpers::FindQueueFamilies(m_Ctx->m_PhysicalDevice).graphicsFamily.value();
		initInfo.Queue			 = m_Ctx->m_GraphicsQueue;
		initInfo.PipelineCache   = VK_NULL_HANDLE;
		initInfo.DescriptorPool  = m_ImGui.descriptorPool;
		initInfo.Allocator		 = VK_NULL_HANDLE;
		initInfo.MinImageCount   = m_Swapchain.imageViews.size();
		initInfo.ImageCount		 = m_Swapchain.imageViews.size();
		initInfo.CheckVkResultFn = ImGuiCheckResult;
		ImGui_ImplVulkan_Init(&initInfo, m_ImGui.renderPass);

		VkCommandBuffer cmd = Helpers::BeginSingleTimeCommands();
		ImGui_ImplVulkan_CreateFontsTexture(cmd);
		Helpers::EndSingleTimeCommands(cmd);

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