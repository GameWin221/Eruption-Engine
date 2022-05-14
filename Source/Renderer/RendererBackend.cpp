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

		CreateCommandBuffer();

		EN_SUCCESS("Created command buffer!")

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
	void VulkanRendererBackend::BindScene(Scene* scene)
	{
		m_Scene = scene;
	}
	void VulkanRendererBackend::UnbindScene()
	{
		m_Scene = nullptr;
	}
	VulkanRendererBackend::~VulkanRendererBackend()
	{
		vkDeviceWaitIdle(m_Ctx->m_LogicalDevice);

		m_GBuffer.Destroy(m_Ctx->m_LogicalDevice);

		m_Swapchain.Destroy(m_Ctx->m_LogicalDevice);

		m_GeometryPipeline.Destroy();
		m_LightingPipeline.Destroy();
		m_TonemappingPipeline.Destroy();

		vkDestroyFence(m_Ctx->m_LogicalDevice, m_SubmitFence, nullptr);

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
		VkResult result = vkAcquireNextImageKHR(m_Ctx->m_LogicalDevice, m_Swapchain.swapchain, UINT64_MAX, m_GeometryPipeline.m_PassFinished, VK_NULL_HANDLE, &m_ImageIndex);

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
		if (m_SkipFrame || !m_Scene) return;

		std::vector<VkClearValue> clearValues(4);
		clearValues[0].color = { m_Scene->m_AmbientColor.r, m_Scene->m_AmbientColor.g, m_Scene->m_AmbientColor.b, 1.0f };
		clearValues[1].color = m_BlackClearValue.color;
		clearValues[2].color = m_BlackClearValue.color;
		clearValues[3].depthStencil = { 1.0f, 0 };

		m_GeometryPipeline.Bind(m_CommandBuffer, m_GBuffer.framebuffer, m_Swapchain.extent, clearValues);

		// Bind the m_MainCamera once per geometry pass
		m_MainCamera->Bind(m_CommandBuffer, m_GeometryPipeline.m_Layout, m_CameraMatrices.get());

		for (const auto& sceneObjectPair : m_Scene->m_SceneObjects)
		{
			SceneObject* object = sceneObjectPair.second.get();

			if (!object->m_Active || !object->m_Mesh->m_Active) continue;

			// Bind object data (model matrix) once per SceneObject in the m_RenderQueue
			object->GetObjectData().Bind(m_CommandBuffer, m_GeometryPipeline.m_Layout);

			for (const auto& subMesh : object->m_Mesh->m_SubMeshes)
			{
				if (!subMesh.m_Active) continue;

				// Bind SubMesh buffers and material once for each SubMesh in the sceneObject->m_Mesh->m_SubMeshes vector
				subMesh.m_VertexBuffer->Bind(m_CommandBuffer);
				subMesh.m_IndexBuffer ->Bind(m_CommandBuffer);
				subMesh.m_Material    ->Bind(m_CommandBuffer, m_GeometryPipeline.m_Layout);

				vkCmdDrawIndexed(m_CommandBuffer, subMesh.m_IndexBuffer->GetSize(), 1, 0, 0, 0);
			}
		}

		m_GeometryPipeline.Unbind(m_CommandBuffer);
	}
	void VulkanRendererBackend::LightingPass()
	{
		if (m_SkipFrame || !m_Scene) return;

		Helpers::TransitionImageLayout(m_GBuffer.albedo.image   , m_GBuffer.albedo.format   , VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_CommandBuffer);
		Helpers::TransitionImageLayout(m_GBuffer.position.image , m_GBuffer.position.format , VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_CommandBuffer);
		Helpers::TransitionImageLayout(m_GBuffer.normal.image   , m_GBuffer.normal.format   , VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_CommandBuffer);
		Helpers::TransitionImageLayout(m_LightingHDRColorBuffer.image, m_LightingHDRColorBuffer.format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_CommandBuffer);
		
		bool lightsChanged = false;

		// Prepare lights and check if at least one value was changed. If so, update the buffer on the GPU
		if (m_Lights.LBO.ambientLight != m_Scene->m_AmbientColor)
			lightsChanged = true;

		m_Lights.LBO.ambientLight = m_Scene->m_AmbientColor;

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

		static std::vector<VkClearValue> clearValues = { m_BlackClearValue };

		m_LightingPipeline.Bind(m_CommandBuffer, m_LightingHDRFramebuffer, m_Swapchain.extent, clearValues);

		vkCmdPushConstants(m_CommandBuffer, m_LightingPipeline.m_Layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0U, sizeof(Lights::LightsCameraInfo), &m_Lights.camera);

		//m_LightingInput->Bind(m_CommandBuffer, m_LightingPipeline.m_Layout);
		vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_LightingPipeline.m_Layout, 0U, 1U, &m_LightingInput.m_DescriptorSet, 0U, nullptr);

		vkCmdDraw(m_CommandBuffer, 3U, 1U, 0U, 0U);

		m_LightingPipeline.Unbind(m_CommandBuffer);
	}
	void VulkanRendererBackend::PostProcessPass()
	{
		if (m_SkipFrame || !m_Scene) return;
		
		static std::vector<VkClearValue> clearValue = { m_BlackClearValue };

		m_TonemappingPipeline.Bind(m_CommandBuffer, m_Swapchain.framebuffers[m_ImageIndex], m_Swapchain.extent, clearValue);

		m_PostProcessParams.exposure.value = m_MainCamera->m_Exposure;

		vkCmdPushConstants(m_CommandBuffer, m_TonemappingPipeline.m_Layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0U, sizeof(PostProcessingParams::Exposure), &m_PostProcessParams.exposure);

		m_TonemappingInput->Bind(m_CommandBuffer, m_TonemappingPipeline.m_Layout);

		vkCmdDraw(m_CommandBuffer, 3U, 1U, 0U, 0U);

		m_TonemappingPipeline.Unbind(m_CommandBuffer);
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
		submitInfo.pWaitSemaphores    = &m_GeometryPipeline.m_PassFinished;
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
		submitInfo.pSignalSemaphores    = &m_LightingPipeline.m_PassFinished;

		if (vkQueueSubmit(m_Ctx->m_GraphicsQueue, 1U, &submitInfo, m_SubmitFence) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::EndRender() - Failed to submit command buffer!");

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1U;
		presentInfo.pWaitSemaphores	   = &m_LightingPipeline.m_PassFinished;

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

		m_GBuffer  .Destroy(m_Ctx->m_LogicalDevice);
		m_Swapchain.Destroy(m_Ctx->m_LogicalDevice);

		m_LightingHDRColorBuffer.Destroy(m_Ctx->m_LogicalDevice);
		vkDestroyFramebuffer(m_Ctx->m_LogicalDevice, m_LightingHDRFramebuffer, nullptr);

		CreateSwapchain();

		CreateGBufferAttachments();

		CreateGBuffer();

		m_GeometryPipeline.Resize(m_Swapchain.extent);

		m_LightingPipeline.Resize(m_Swapchain.extent);

		m_TonemappingPipeline.Resize(m_Swapchain.extent);

		CreateSwapchainFramebuffers();

		//LCreateDescriptorSet();
		//PPCreateDescriptorSet();

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
			framebufferInfo.renderPass		= m_TonemappingPipeline.m_RenderPass;
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
		framebufferInfo.renderPass	    = m_GeometryPipeline.m_RenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments    = attachments.data();
		framebufferInfo.width  = m_Swapchain.extent.width;
		framebufferInfo.height = m_Swapchain.extent.height;
		framebufferInfo.layers = 1U;

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
		samplerInfo.maxAnisotropy	 = 0U;
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
		std::vector<Pipeline::Attachment> colorAttachments = 
		{
			{m_GBuffer.albedo.format  , VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED ,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0U},
			{m_GBuffer.position.format, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED ,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1U},
			{m_GBuffer.normal.format  , VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED ,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 2U}
		};

		Pipeline::Attachment depthAttachment{ m_GBuffer.depth.format  , VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 3U };

		m_GeometryPipeline.CreateRenderPass(colorAttachments, depthAttachment);

		Shader vShader("Shaders/GeometryVertex.spv", ShaderType::Vertex);
		Shader fShader("Shaders/GeometryFragment.spv", ShaderType::Fragment);

		std::vector<VkDescriptorSetLayout> descriptors{ CameraMatricesBuffer::GetLayout(), Material::GetLayout() };

		VkPushConstantRange objectPushConstant{};
		objectPushConstant.offset	  = 0U;
		objectPushConstant.size		  = sizeof(PerObjectData);
		objectPushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		std::vector<VkPushConstantRange> pushConstants = { objectPushConstant };

		m_GeometryPipeline.CreatePipeline(vShader, fShader, m_Swapchain.extent, descriptors, pushConstants, true, true);

		m_GeometryPipeline.CreateSyncSemaphore();
	}
	void VulkanRendererBackend::InitLightingPipeline()
	{
		std::vector<Pipeline::Attachment> colorAttachments = 
		{
			{VK_FORMAT_R16G16B16A16_SFLOAT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0U}
		};

		m_LightingPipeline.CreateRenderPass(colorAttachments);

		LCreateHDRFramebuffer();
		LCreateLightsBuffer();

		UniformBuffer::ImageInfo albedo{};
		albedo.imageView    = m_GBuffer.albedo.imageView;
		albedo.imageSampler = m_GBuffer.sampler;
		albedo.index		= 0U;

		UniformBuffer::ImageInfo position{};
		position.imageView = m_GBuffer.position.imageView;
		position.imageSampler = m_GBuffer.sampler;
		position.index = 1U;

		UniformBuffer::ImageInfo normal{};
		normal.imageView = m_GBuffer.normal.imageView;
		normal.imageSampler = m_GBuffer.sampler;
		normal.index = 2U;

		UniformBuffer::BufferInfo buffer{};
		buffer.buffer = m_Lights.buffer;
		buffer.index = 3U;
		buffer.size = sizeof(m_Lights.LBO);

		std::vector<UniformBuffer::ImageInfo> imageInfos = { albedo, position, normal };

		CreateDescriptorPool();
		CreateDescriptorSet();

		//m_LightingInput = std::make_unique<UniformBuffer>(imageInfos, buffer);

		Shader vShader("Shaders/FullscreenTriVert.spv", ShaderType::Vertex);
		Shader fShader("Shaders/LightingFrag.spv", ShaderType::Fragment);

		VkPushConstantRange lightsCameraInfoRange{};
		lightsCameraInfoRange.offset = 0U;
		lightsCameraInfoRange.size = sizeof(Lights::LightsCameraInfo);
		lightsCameraInfoRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::vector<VkPushConstantRange> pushConstants = { lightsCameraInfoRange };
		//std::vector<VkDescriptorSetLayout> layout = { m_LightingInput->m_DescriptorLayout };
		std::vector<VkDescriptorSetLayout> layout = { m_LightingInput.m_DescriptorLayout };

		m_LightingPipeline.CreatePipeline(vShader, fShader, m_Swapchain.extent, layout, pushConstants);
		m_LightingPipeline.CreateSyncSemaphore();

		system("pause");
	}
	void VulkanRendererBackend::InitPostProcessPipeline()
	{
		std::vector<Pipeline::Attachment> attachments = 
		{
			{m_Swapchain.imageFormat, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0U}
		};

		m_TonemappingPipeline.CreateRenderPass(attachments);

		Shader vShader("Shaders/FullscreenTriVert.spv", ShaderType::Vertex);
		Shader fShader("Shaders/TonemapFrag.spv", ShaderType::Fragment);

		UniformBuffer::ImageInfo HDRColorBuffer{};
		HDRColorBuffer.imageView = m_LightingHDRColorBuffer.imageView;
		HDRColorBuffer.imageSampler = m_GBuffer.sampler;
		HDRColorBuffer.index = 0U;

		std::vector<UniformBuffer::ImageInfo> imageInfos = { HDRColorBuffer };

		m_TonemappingInput = std::make_unique<UniformBuffer>(imageInfos);

		std::vector<VkDescriptorSetLayout> layout = { m_TonemappingInput->m_DescriptorLayout };

		VkPushConstantRange exposurePushConstant{};
		exposurePushConstant.offset		= 0U;
		exposurePushConstant.size		= sizeof(PostProcessingParams::Exposure);
		exposurePushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::vector<VkPushConstantRange> pushConstants = { exposurePushConstant };

		m_TonemappingPipeline.CreatePipeline(vShader, fShader, m_Swapchain.extent, layout, pushConstants);
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

	void VulkanRendererBackend::LCreateLightsBuffer()
	{
		m_Lights.bufferSize = sizeof(m_Lights.LBO);
		en::Helpers::CreateBuffer(m_Lights.bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_Lights.buffer, m_Lights.bufferMemory);
	}
	void VulkanRendererBackend::LCreateHDRFramebuffer()
	{
		CreateAttachment(m_LightingHDRColorBuffer, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType			= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass		= m_LightingPipeline.m_RenderPass;
		framebufferInfo.attachmentCount = 1U;
		framebufferInfo.pAttachments	= &m_LightingHDRColorBuffer.imageView;
		framebufferInfo.width			= m_Swapchain.extent.width;
		framebufferInfo.height			= m_Swapchain.extent.height;
		framebufferInfo.layers			= 1U;

		if (vkCreateFramebuffer(m_Ctx->m_LogicalDevice, &framebufferInfo, nullptr, &m_LightingHDRFramebuffer) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::LCreateHDRFramebuffer() - Failed to create the GBuffer!");
	}
	
	void VulkanRendererBackend::CreateSyncObjects()
	{
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		if (vkCreateFence(m_Ctx->m_LogicalDevice, &fenceInfo	, nullptr, &m_SubmitFence) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::CreateSyncObjects() - Failed to create sync objects!");
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
	void VulkanRendererBackend::CreateDescriptorPool()
	{
		std::array<VkDescriptorSetLayoutBinding, 4> bindings;
		//0: Color Framebuffer
		//1: Position Framebuffer (Alpha channel is specularity)
		//2: Normal Framebuffer
		//3: Lights Buffer

		for (uint32_t i = 0U; auto & binding : bindings)
		{
			binding.binding = i;
			binding.descriptorCount = 1U;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			binding.pImmutableSamplers = nullptr;
			binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			i++;
		}

		// Change the light buffer's type to VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
		bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(m_Ctx->m_LogicalDevice, &layoutInfo, nullptr, &m_LightingInput.m_DescriptorLayout) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::A() - Failed to create descriptor set layout!");

		std::array<VkDescriptorPoolSize, 4> poolSizes{};

		for (auto& poolSize : poolSizes)
		{
			poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSize.descriptorCount = 1U;
		}

		// Lights buffer
		poolSizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = 1U;

		if (vkCreateDescriptorPool(m_Ctx->m_LogicalDevice, &poolInfo, nullptr, &m_LightingInput.m_DescriptorPool) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::A() - Failed to create descriptor pool!");
	}
	void VulkanRendererBackend::CreateDescriptorSet()
	{
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_LightingInput.m_DescriptorPool;
		allocInfo.descriptorSetCount = 1U;
		allocInfo.pSetLayouts = &m_LightingInput.m_DescriptorLayout;

		if (vkAllocateDescriptorSets(m_Ctx->m_LogicalDevice, &allocInfo, &m_LightingInput.m_DescriptorSet) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::A() - Failed to allocate descriptor set!");

		std::array<VkDescriptorImageInfo, 3> imageInfos{};

		// Albedo
		imageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfos[0].imageView = m_GBuffer.albedo.imageView;
		imageInfos[0].sampler = m_GBuffer.sampler;

		// Position
		imageInfos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfos[1].imageView = m_GBuffer.position.imageView;
		imageInfos[1].sampler = m_GBuffer.sampler;

		// Normal
		imageInfos[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfos[2].imageView = m_GBuffer.normal.imageView;
		imageInfos[2].sampler = m_GBuffer.sampler;

		// Light Buffer
		VkDescriptorBufferInfo lightBufferInfo{};
		lightBufferInfo.buffer = m_Lights.buffer;
		lightBufferInfo.offset = 0U;
		lightBufferInfo.range = sizeof(m_Lights.LBO);

		std::array<VkWriteDescriptorSet, 4> descriptorWrites{};

		for (uint32_t i = 0U; auto & descriptorWrite : descriptorWrites)
		{
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = m_LightingInput.m_DescriptorSet;
			descriptorWrite.dstBinding = i;
			descriptorWrite.dstArrayElement = 0U;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrite.descriptorCount = 1U;

			if (i != 3U)
				descriptorWrite.pImageInfo = &imageInfos[i];

			i++;
		}

		// Light Buffer
		descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[3].pBufferInfo = &lightBufferInfo;

		vkUpdateDescriptorSets(m_Ctx->m_LogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}