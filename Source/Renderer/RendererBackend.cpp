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

		CreateGBufferHandle();

		EN_SUCCESS("Created GBuffer handle!")

		InitLightingPipeline();

        EN_SUCCESS("Created lighting pipeline!")

		InitTonemappingPipeline();

        EN_SUCCESS("Created PP pipeline!")

		CreateSwapchainFramebuffers();

        EN_SUCCESS("Created swapchain framebuffers!")

		CreateSyncObjects();

        EN_SUCCESS("Created sync objects!")

		CreateCameraMatricesBuffer();

        EN_SUCCESS("Created camera matrices buffer!")

		InitImGui();

		EN_SUCCESS("Created the renderer Vulkan backend");

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

		m_Swapchain.Destroy(m_Ctx->m_LogicalDevice);

		vkDestroyFence(m_Ctx->m_LogicalDevice, m_SubmitFence, nullptr);

		vkDestroyRenderPass(m_Ctx->m_LogicalDevice, m_ImGui.renderPass, nullptr);

		m_ImGui.Destroy();
	}

	void VulkanRendererBackend::WaitForGPUIdle()
	{
		vkWaitForFences(m_Ctx->m_LogicalDevice, 1U, &m_SubmitFence, VK_TRUE, UINT64_MAX);
	}
	void VulkanRendererBackend::BeginRender()
	{
		VkResult result = vkAcquireNextImageKHR(m_Ctx->m_LogicalDevice, m_Swapchain.swapchain, UINT64_MAX, m_GeometryPipeline->m_PassFinished, VK_NULL_HANDLE, &m_ImageIndex);

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

		m_GeometryPipeline->Bind(m_CommandBuffer, m_GBuffer->m_Framebuffer, m_Swapchain.extent, clearValues);

		// Bind the m_MainCamera once per geometry pass
		m_MainCamera->Bind(m_CommandBuffer, m_GeometryPipeline->m_Layout, m_CameraMatrices.get());

		for (const auto& sceneObjectPair : m_Scene->m_SceneObjects)
		{
			SceneObject* object = sceneObjectPair.second.get();

			if (!object->m_Active || !object->m_Mesh->m_Active) continue;

			// Bind object data (model matrix) once per SceneObject in the m_RenderQueue
			object->GetObjectData().Bind(m_CommandBuffer, m_GeometryPipeline->m_Layout);

			for (const auto& subMesh : object->m_Mesh->m_SubMeshes)
			{
				if (!subMesh.m_Active) continue;

				// Bind SubMesh buffers and material once for each SubMesh in the sceneObject->m_Mesh->m_SubMeshes vector
				subMesh.m_VertexBuffer->Bind(m_CommandBuffer);
				subMesh.m_IndexBuffer ->Bind(m_CommandBuffer);
				subMesh.m_Material    ->Bind(m_CommandBuffer, m_GeometryPipeline->m_Layout);

				vkCmdDrawIndexed(m_CommandBuffer, subMesh.m_IndexBuffer->GetIndicesCount(), 1, 0, 0, 0);
			}
		}

		m_GeometryPipeline->Unbind(m_CommandBuffer);
	}
	void VulkanRendererBackend::LightingPass()
	{
		if (m_SkipFrame || !m_Scene) return;

		Helpers::TransitionImageLayout(m_GBuffer->m_Attachments[0].image, m_GBuffer->m_Attachments[0].format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_CommandBuffer);
		Helpers::TransitionImageLayout(m_GBuffer->m_Attachments[1].image, m_GBuffer->m_Attachments[1].format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_CommandBuffer);
		Helpers::TransitionImageLayout(m_GBuffer->m_Attachments[2].image, m_GBuffer->m_Attachments[2].format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_CommandBuffer);
		
		Helpers::TransitionImageLayout(m_HDRFramebuffer->m_Attachments[0].image, m_HDRFramebuffer->m_Attachments[0].format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_CommandBuffer);

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
			m_Lights.buffer->MapMemory( &m_Lights.LBO, m_Lights.buffer->GetSize());

		m_LightingPipeline->Bind(m_CommandBuffer, m_HDRFramebuffer->m_Framebuffer, m_Swapchain.extent);

		vkCmdPushConstants(m_CommandBuffer, m_LightingPipeline->m_Layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0U, sizeof(Lights::LightsCameraInfo), &m_Lights.camera);

		m_LightingInput->Bind(m_CommandBuffer, m_LightingPipeline->m_Layout);

		vkCmdDraw(m_CommandBuffer, 3U, 1U, 0U, 0U);

		m_LightingPipeline->Unbind(m_CommandBuffer);
	}
	void VulkanRendererBackend::PostProcessPass()
	{
		if (m_SkipFrame || !m_Scene) return;
		
		static std::vector<VkClearValue> clearValue = { m_BlackClearValue };

		m_TonemappingPipeline->Bind(m_CommandBuffer, m_Swapchain.framebuffers[m_ImageIndex], m_Swapchain.extent, clearValue);

		m_PostProcessParams.exposure.value = m_MainCamera->m_Exposure;

		vkCmdPushConstants(m_CommandBuffer, m_TonemappingPipeline->m_Layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0U, sizeof(PostProcessingParams::Exposure), &m_PostProcessParams.exposure);

		m_TonemappingInput->Bind(m_CommandBuffer, m_TonemappingPipeline->m_Layout);

		vkCmdDraw(m_CommandBuffer, 3U, 1U, 0U, 0U);

		m_TonemappingPipeline->Unbind(m_CommandBuffer);
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
		submitInfo.pWaitSemaphores    = &m_GeometryPipeline->m_PassFinished;
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
		submitInfo.pSignalSemaphores    = &m_LightingPipeline->m_PassFinished;

		if (vkQueueSubmit(m_Ctx->m_GraphicsQueue, 1U, &submitInfo, m_SubmitFence) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::EndRender() - Failed to submit command buffer!");

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1U;
		presentInfo.pWaitSemaphores	   = &m_LightingPipeline->m_PassFinished;

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

		m_GBuffer->Destroy();
		m_HDRFramebuffer->Destroy();

		CreateSwapchain();

		CreateGBufferAttachments();

		m_GeometryPipeline->Resize(m_Swapchain.extent);

		CreateGBufferHandle();

		CreateLightingHDRFramebuffer();

		UpdateLightingInput();

		m_LightingPipeline->Resize(m_Swapchain.extent);

		UpdateTonemappingInput();

		m_TonemappingPipeline->Resize(m_Swapchain.extent);

		CreateSwapchainFramebuffers();

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
			framebufferInfo.renderPass		= m_TonemappingPipeline->m_RenderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments	= &m_Swapchain.imageViews[i];
			framebufferInfo.width			= m_Swapchain.extent.width;
			framebufferInfo.height			= m_Swapchain.extent.height;
			framebufferInfo.layers			= 1;

			if (vkCreateFramebuffer(m_Ctx->m_LogicalDevice, &framebufferInfo, nullptr, &m_Swapchain.framebuffers[i]) != VK_SUCCESS)
				EN_ERROR("VulkanRendererBackend::CreateSwapchainFramebuffers() - Failed to create framebuffers!");
		}
	}

	void VulkanRendererBackend::InitGeometryPipeline()
	{
		m_GeometryPipeline = std::make_unique<Pipeline>();

		std::vector<Pipeline::Attachment> colorAttachments = 
		{
			{m_GBuffer->m_Attachments[0].format, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0U},
			{m_GBuffer->m_Attachments[1].format, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1U},
			{m_GBuffer->m_Attachments[2].format, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 2U}
		};

		Pipeline::Attachment depthAttachment{ m_GBuffer->m_Attachments[3].format  , VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 3U };

		Pipeline::RenderPassInfo renderPassInfo{};
		renderPassInfo.colorAttachments = colorAttachments;
		renderPassInfo.depthAttachment = depthAttachment;

		m_GeometryPipeline->CreateRenderPass(renderPassInfo);

		Shader vShader("Shaders/GeometryVertex.spv", ShaderType::Vertex);
		Shader fShader("Shaders/GeometryFragment.spv", ShaderType::Fragment);

		VkPushConstantRange objectPushConstant{};
		objectPushConstant.offset = 0U;
		objectPushConstant.size = sizeof(PerObjectData);
		objectPushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		Pipeline::PipelineInfo pipelineInfo{};
		pipelineInfo.vShader			= &vShader;
		pipelineInfo.fShader			= &fShader;
		pipelineInfo.extent				= m_Swapchain.extent;
		pipelineInfo.descriptorLayouts  = { CameraMatricesBuffer::GetLayout(), Material::GetLayout() };
		pipelineInfo.pushConstantRanges = { objectPushConstant };
		pipelineInfo.enableDepthTest    = true;
		pipelineInfo.useVertexBindings  = true;
		
		m_GeometryPipeline->CreatePipeline(pipelineInfo); 
		m_GeometryPipeline->CreateSyncSemaphore(); 
	}

	void VulkanRendererBackend::UpdateLightingInput()
	{
		PipelineInput::ImageInfo albedo{};
		albedo.imageView = m_GBuffer->m_Attachments[0].imageView;
		albedo.imageSampler = m_GBuffer->m_Sampler;
		albedo.index = 0U;

		PipelineInput::ImageInfo position{};
		position.imageView = m_GBuffer->m_Attachments[1].imageView;
		position.imageSampler = m_GBuffer->m_Sampler;
		position.index = 1U;

		PipelineInput::ImageInfo normal{};
		normal.imageView = m_GBuffer->m_Attachments[2].imageView;
		normal.imageSampler = m_GBuffer->m_Sampler;
		normal.index = 2U;

		PipelineInput::BufferInfo buffer{};
		buffer.buffer = m_Lights.buffer->GetHandle();
		buffer.index = 3U;
		buffer.size = sizeof(m_Lights.LBO);

		std::vector<PipelineInput::ImageInfo> imageInfos = { albedo, position, normal };

		if (!m_LightingInput)
			m_LightingInput = std::make_unique<PipelineInput>(imageInfos, buffer);
		else
			m_LightingInput->UpdateDescriptorSet(imageInfos, buffer);
	}
	void VulkanRendererBackend::InitLightingPipeline()
	{
		m_LightingPipeline = std::make_unique<Pipeline>();

		std::vector<Pipeline::Attachment> colorAttachments = 
		{
			{VK_FORMAT_R16G16B16A16_SFLOAT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0U}
		};

		m_Lights.buffer = std::make_unique<MemoryBuffer>(sizeof(m_Lights.LBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		UpdateLightingInput();

		Pipeline::RenderPassInfo renderPassInfo{};
		renderPassInfo.colorAttachments = colorAttachments;

		m_LightingPipeline->CreateRenderPass(renderPassInfo);

		CreateLightingHDRFramebuffer();

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

		m_LightingPipeline->CreatePipeline(pipelineInfo);
		m_LightingPipeline->CreateSyncSemaphore();
	}

	void VulkanRendererBackend::UpdateTonemappingInput()
	{
		PipelineInput::ImageInfo HDRColorBuffer{};
		HDRColorBuffer.imageView	= m_HDRFramebuffer->m_Attachments[0].imageView;
		HDRColorBuffer.imageSampler = m_HDRFramebuffer->m_Sampler;
		HDRColorBuffer.index = 0U;

		std::vector<PipelineInput::ImageInfo> imageInfos = { HDRColorBuffer };

		if (!m_TonemappingInput)
			m_TonemappingInput = std::make_unique<PipelineInput>(imageInfos);
		else
			m_TonemappingInput->UpdateDescriptorSet(imageInfos);
	}
	void VulkanRendererBackend::InitTonemappingPipeline()
	{
		m_TonemappingPipeline = std::make_unique<Pipeline>();

		std::vector<Pipeline::Attachment> attachments = 
		{
			{m_Swapchain.imageFormat, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0U}
		};

		UpdateTonemappingInput();

		Pipeline::RenderPassInfo renderPassInfo{};
		renderPassInfo.colorAttachments = attachments;

		m_TonemappingPipeline->CreateRenderPass(renderPassInfo);

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
		pipelineInfo.descriptorLayouts  = { m_TonemappingInput->m_DescriptorLayout };
		pipelineInfo.pushConstantRanges = { exposurePushConstant };
		pipelineInfo.cullMode		    = VK_CULL_MODE_FRONT_BIT;

		m_TonemappingPipeline->CreatePipeline(pipelineInfo);
		m_TonemappingPipeline->CreateSyncSemaphore();
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

	void VulkanRendererBackend::CreateGBufferHandle()
	{
		m_GBuffer->CreateFramebuffer(m_GeometryPipeline->m_RenderPass);
	}
	void VulkanRendererBackend::CreateGBufferAttachments()
	{
		if(!m_GBuffer)
			m_GBuffer = std::make_unique<Framebuffer>();

		Framebuffer::AttachmentInfo albedo{};
		albedo.format = m_Swapchain.imageFormat;

		Framebuffer::AttachmentInfo position{};
		position.format = VK_FORMAT_R16G16B16A16_SFLOAT;

		Framebuffer::AttachmentInfo normal{};
		normal.format = VK_FORMAT_R16G16B16A16_SFLOAT;

		Framebuffer::AttachmentInfo depth{};
		depth.format		   = FindDepthFormat();
		depth.imageLayout	   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depth.imageAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
		depth.imageUsageFlags  = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		m_GBuffer->CreateSampler();
		m_GBuffer->CreateAttachments({ albedo , position, normal, depth }, m_Swapchain.extent.width, m_Swapchain.extent.height);
	}

	void VulkanRendererBackend::CreateLightingHDRFramebuffer()
	{
		if (!m_HDRFramebuffer)
			m_HDRFramebuffer = std::make_unique<Framebuffer>();

		Framebuffer::AttachmentInfo attachment{};
		attachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		attachment.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		m_HDRFramebuffer->CreateSampler();
		m_HDRFramebuffer->CreateAttachments({ attachment }, m_Swapchain.extent.width, m_Swapchain.extent.height);
		m_HDRFramebuffer->CreateFramebuffer(m_LightingPipeline->m_RenderPass);
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

}