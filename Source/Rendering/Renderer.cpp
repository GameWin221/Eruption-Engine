#include "EnPch.hpp"
#include "Rendering/Renderer.hpp"

namespace en
{
	Renderer* g_CurrentRenderer = nullptr;
	Camera* g_DefaultCamera = nullptr;

	Renderer::Renderer(RendererInfo& rendererInfo)
	{
		m_Ctx = &Context::GetContext();
		m_Window = &Window::GetMainWindow();
		m_RendererInfo = rendererInfo;

		g_CurrentRenderer = this;

		CameraInfo cameraInfo{};
		cameraInfo.dynamicallyScaled = true;

		if (!g_DefaultCamera)
			g_DefaultCamera = new Camera(cameraInfo);

		m_MainCamera = g_DefaultCamera;

		glfwSetFramebufferSizeCallback(m_Window->m_GLFWWindow, Renderer::FramebufferResizeCallback);

		VKCreateCommandBuffer();
		VKCreateSwapChain();

		VKCreateDescriptorPool();
		VKCreatePresentDPool();

		VKCreateRenderPass();
		VKCreateDescriptorSetLayout();
		VKCreateGraphicsPipeline();

		VKCreateGBuffer();

		VKCreatePresentPass();
		VKCreatePresentDSetLayout();
		VKCreatePresentPipeline();
		VKCreateFramebuffers();

		VKCreateSyncObjects();
	}
	Renderer::~Renderer()
	{
		vkDeviceWaitIdle(m_Ctx->m_LogicalDevice);

		m_OffscreenFramebuffer.Destroy(m_Ctx->m_LogicalDevice);

		vkDestroyFence(m_Ctx->m_LogicalDevice, m_InFlightFence, nullptr);

		m_SwapChain.Destroy(m_Ctx->m_LogicalDevice);

		m_GraphicsPipeline.Destroy(m_Ctx->m_LogicalDevice);
		m_PresentationPipeline.Destroy(m_Ctx->m_LogicalDevice);
	}

	void Renderer::PrepareModel(Model* model)
	{
		for (auto& mesh : model->m_Meshes)
			PrepareMesh(&mesh, model);
	}
	void Renderer::RemoveModel(Model* model)
	{
		for (auto& mesh : model->m_Meshes)
			RemoveMesh(&mesh);
	}
	void Renderer::EnqueueModel(Model* model)
	{
		for (auto& mesh : model->m_Meshes)
			EnqueueMesh(&mesh);
	}

	void Renderer::PrepareMesh(Mesh* mesh, Model* parent)
	{
		m_MeshData[mesh].parent = parent;

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType			 = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_GraphicsPipeline.descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(1);
		allocInfo.pSetLayouts = &m_GraphicsPipeline.descriptorSetLayout;

		if (vkAllocateDescriptorSets(m_Ctx->m_LogicalDevice, &allocInfo, &m_MeshData[mesh].descriptorSet) != VK_SUCCESS)
			throw std::runtime_error(" Renderer::PrepareMesh() - Failed to allocate descriptor sets!");

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_MeshData[mesh].parent->m_UniformBuffer->m_Buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView   = mesh->m_Texture->m_ImageView;
		imageInfo.sampler     = mesh->m_Texture->m_ImageSampler;


		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = m_MeshData[mesh].descriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = m_MeshData[mesh].descriptorSet;
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(m_Ctx->m_LogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
	void Renderer::RemoveMesh(Mesh* mesh)
	{
		vkDeviceWaitIdle(m_Ctx->m_LogicalDevice);

		vkFreeDescriptorSets(m_Ctx->m_LogicalDevice, m_GraphicsPipeline.descriptorPool, 1, &m_MeshData[mesh].descriptorSet);
		m_MeshData.erase(mesh);
	}
	void Renderer::EnqueueMesh(Mesh* mesh)
	{
		m_MeshQueue.emplace_back(mesh);
	}

	void Renderer::Render()
	{
		vkWaitForFences(m_Ctx->m_LogicalDevice, 1, &m_InFlightFence, VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(m_Ctx->m_LogicalDevice, m_SwapChain.swapChain, UINT64_MAX, m_GraphicsPipeline.finishedSemaphore, VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || m_FramebufferResized)
		{
			RecreateFramebuffer();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
			throw std::runtime_error("Renderer.cpp::Renderer::Render() - Failed to acquire swap chain image!");

		vkResetFences(m_Ctx->m_LogicalDevice, 1, &m_InFlightFence);

		vkResetCommandBuffer(m_CommandBuffer, 0);

		RecordCommandBuffer(m_CommandBuffer, imageIndex);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_GraphicsPipeline.finishedSemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffer;

		VkSemaphore signalSemaphores[] = { m_PresentationPipeline.finishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(m_Ctx->m_GraphicsQueue, 1, &submitInfo, m_InFlightFence) != VK_SUCCESS)
			throw std::runtime_error("Renderer::Renderer::Render() - Failed to submit draw command buffer!");

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_SwapChain.swapChain;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;
		
		result = vkQueuePresentKHR(m_Ctx->m_PresentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized)
			RecreateFramebuffer();
		else if (result != VK_SUCCESS)
			throw std::runtime_error("Renderer::Render() - Failed to present swap chain image!");
	}
	void Renderer::RecordCommandBuffer(VkCommandBuffer& commandBuffer, uint32_t imageIndex)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
			throw std::runtime_error("Renderer::RecordCommandBuffer() - Failed to begin recording command buffer!");
		
		GraphicsPass(commandBuffer);
		PresentationPass(commandBuffer, imageIndex);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
			throw std::runtime_error("Renderer::RecordCommandBuffer() - Failed to record command buffer!");
	}

	void Renderer::GraphicsPass(VkCommandBuffer& commandBuffer)
	{
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_GraphicsPipeline.renderPass;
		renderPassInfo.framebuffer = m_OffscreenFramebuffer.framebuffer;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_SwapChain.extent;

		std::array<VkClearValue, 4> clearValues{};
		clearValues[0].color = m_RendererInfo.clearColor;
		clearValues[1].color = m_RendererInfo.clearColor;
		clearValues[2].color = m_RendererInfo.clearColor;

		clearValues[3].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline.pipeline);

		for (int i = 0; const auto & mesh : m_MeshQueue)
		{
			m_MeshData.at(mesh).parent->m_UniformBuffer->m_UBO.proj = m_MainCamera->GetProjMatrix();
			m_MeshData.at(mesh).parent->m_UniformBuffer->m_UBO.view = m_MainCamera->GetViewMatrix();
			m_MeshData.at(mesh).parent->m_UniformBuffer->Bind();

			mesh->m_VertexBuffer->Bind(commandBuffer);
			mesh->m_IndexBuffer->Bind(commandBuffer);

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline.layout, 0, 1, &m_MeshData.at(mesh).descriptorSet, 0, nullptr);

			vkCmdDrawIndexed(commandBuffer, mesh->m_IndexBuffer->GetSize(), 1, 0, 0, 0);

			i++;
		}
		m_MeshQueue.clear();

		vkCmdEndRenderPass(commandBuffer);
	}
	void Renderer::PresentationPass(VkCommandBuffer& commandBuffer, uint32_t imageIndex)
	{
		Helpers::TransitionImageLayout(m_OffscreenFramebuffer.albedo.image  , m_SwapChain.imageFormat	   , VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, commandBuffer);
		Helpers::TransitionImageLayout(m_OffscreenFramebuffer.position.image, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, commandBuffer);
		Helpers::TransitionImageLayout(m_OffscreenFramebuffer.normal.image  , VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, commandBuffer);
		
		VkClearValue clearValue = { 0.0f,0.0f, 0.0f, 1.0f };

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType			 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass		 = m_PresentationPipeline.renderPass;
		renderPassInfo.framebuffer		 = m_SwapChain.framebuffers[imageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_SwapChain.extent;
		renderPassInfo.clearValueCount	 = 1;
		renderPassInfo.pClearValues		 = &clearValue;

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PresentationPipeline.pipeline);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PresentationPipeline.layout, 0, 1, &m_PresentationDescriptor, 0, nullptr);

		vkCmdDraw(commandBuffer, 6, 1, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

		Helpers::TransitionImageLayout(m_OffscreenFramebuffer.albedo.image  , m_SwapChain.imageFormat	   , VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, commandBuffer);
		Helpers::TransitionImageLayout(m_OffscreenFramebuffer.position.image, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, commandBuffer);
		Helpers::TransitionImageLayout(m_OffscreenFramebuffer.normal.image  , VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, commandBuffer);
	}

	void Renderer::RecreateFramebuffer()
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(m_Window->m_GLFWWindow, &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(m_Window->m_GLFWWindow, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(m_Ctx->m_LogicalDevice);

		m_GraphicsPipeline.Destroy(m_Ctx->m_LogicalDevice);
		m_PresentationPipeline.Destroy(m_Ctx->m_LogicalDevice);

		m_OffscreenFramebuffer.Destroy(m_Ctx->m_LogicalDevice);

		m_SwapChain.Destroy(m_Ctx->m_LogicalDevice);

		VKCreateSwapChain();
		VKCreateGBuffer();
		VKCreateGraphicsPipeline();
		VKCreatePresentPipeline();

		VKCreateFramebuffers();

		m_FramebufferResized = false;
	}

	void Renderer::SetMainCamera(Camera* camera)
	{
		if (camera)
			m_MainCamera = camera;
		else
			m_MainCamera = g_DefaultCamera;
	}

	Renderer& Renderer::GetRenderer()
	{
		return *g_CurrentRenderer;
	}

	void Renderer::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		g_CurrentRenderer->m_FramebufferResized = true;
	}

	void Renderer::VKCreateCommandBuffer()
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_Ctx->m_CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(m_Ctx->m_LogicalDevice, &allocInfo, &m_CommandBuffer) != VK_SUCCESS)
			throw std::runtime_error("Renderer.cpp::Renderer::VKCreateCommandBuffer() - Failed to allocate command buffers!");
	}
	void Renderer::VKCreateSwapChain()
	{
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_Ctx->m_PhysicalDevice);

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR   presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D		   extent = ChooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
			imageCount = swapChainSupport.capabilities.maxImageCount;


		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_Ctx->m_WindowSurface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		Helpers::QueueFamilyIndices indices = Helpers::FindQueueFamilies(m_Ctx->m_PhysicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}


		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(m_Ctx->m_LogicalDevice, &createInfo, nullptr, &m_SwapChain.swapChain) != VK_SUCCESS)
			throw std::runtime_error("Context::VKCreateSwapChain() - Failed to create swap chain!");

		vkGetSwapchainImagesKHR(m_Ctx->m_LogicalDevice, m_SwapChain.swapChain, &imageCount, nullptr);
		m_SwapChain.images.resize(imageCount);
		vkGetSwapchainImagesKHR(m_Ctx->m_LogicalDevice, m_SwapChain.swapChain, &imageCount, m_SwapChain.images.data());

		m_SwapChain.imageFormat = surfaceFormat.format;
		m_SwapChain.extent = extent;

		m_SwapChain.imageViews.resize(m_SwapChain.images.size());

		for (size_t i = 0; i < m_SwapChain.imageViews.size(); i++)
			Helpers::CreateImageView(m_SwapChain.images[i], m_SwapChain.imageViews[i], m_SwapChain.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}
	void Renderer::VKCreateRenderPass()
	{
		VkAttachmentDescription albedoAttachment{};
		albedoAttachment.format			= m_SwapChain.imageFormat;
		albedoAttachment.samples		= VK_SAMPLE_COUNT_1_BIT;
		albedoAttachment.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
		albedoAttachment.storeOp		= VK_ATTACHMENT_STORE_OP_STORE;
		albedoAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		albedoAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		albedoAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		albedoAttachment.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference albedoAttachmentRef{};
		albedoAttachmentRef.attachment = 0;
		albedoAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription positionAttachment{};
		positionAttachment.format		  = VK_FORMAT_R16G16B16A16_SFLOAT;
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
		normalAttachment.format         = VK_FORMAT_R16G16B16A16_SFLOAT;
		normalAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
		normalAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
		normalAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
		normalAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		normalAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		normalAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		normalAttachment.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference normalAttachmentRef{};
		normalAttachmentRef.attachment = 2;
		normalAttachmentRef.layout	   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format		   = FindDepthFormat();
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
		subpass.colorAttachmentCount    = 3;
		subpass.pColorAttachments       = colorAttachments.data();
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
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(m_Ctx->m_LogicalDevice, &renderPassInfo, nullptr, &m_GraphicsPipeline.renderPass) != VK_SUCCESS)
			throw std::runtime_error("Renderer::VKCreateRenderPass() - Failed to create render pass!");
	}
	void Renderer::VKCreateGraphicsPipeline()
	{
		Shader vShader("Shaders/vertex.spv"  , ShaderType::Vertex);
		Shader fShader("Shaders/fragment.spv", ShaderType::Fragment);

		VkPipelineShaderStageCreateInfo shaderStages[] = { vShader.m_ShaderInfo, fShader.m_ShaderInfo };

		auto bindingDescription    = Vertex::GetBindingDescription();
		auto attributeDescriptions = Vertex::GetAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType							= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount   = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType				     = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology				 = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width  = (float)m_SwapChain.extent.width;
		viewport.height = (float)m_SwapChain.extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = m_SwapChain.extent;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType			= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports    = &viewport;
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
		colorBlendAttachment.blendEnable		 = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp		 = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp		 = VK_BLEND_OP_ADD;

		VkPipelineColorBlendAttachmentState positionBlendAttachment{};
		positionBlendAttachment.colorWriteMask		= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		positionBlendAttachment.blendEnable			= VK_FALSE;
		positionBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		positionBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		positionBlendAttachment.colorBlendOp		= VK_BLEND_OP_ADD;
		positionBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		positionBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		positionBlendAttachment.alphaBlendOp		= VK_BLEND_OP_ADD;


		std::array<VkPipelineColorBlendAttachmentState, 3> colorBlendAttachments = {
			colorBlendAttachment,
			positionBlendAttachment,
			positionBlendAttachment
		};

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType				= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable		= VK_FALSE;
		colorBlending.logicOp			= VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount	= 3;
		colorBlending.pAttachments	    = colorBlendAttachments.data();
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
		depthStencil.stencilTestEnable	   = VK_TRUE;
		depthStencil.front				   = {};
		depthStencil.back				   = {};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType		  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts	  = &m_GraphicsPipeline.descriptorSetLayout;

		if (vkCreatePipelineLayout(m_Ctx->m_LogicalDevice, &pipelineLayoutInfo, nullptr, &m_GraphicsPipeline.layout) != VK_SUCCESS)
			throw std::runtime_error("Renderer::VKCreateGraphicsPipeline() - Failed to create pipeline layout!");

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages    = shaderStages;

		pipelineInfo.pVertexInputState	 = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState		 = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState   = &multisampling;
		pipelineInfo.pDepthStencilState  = &depthStencil;
		pipelineInfo.pColorBlendState    = &colorBlending;
		pipelineInfo.pDynamicState       = nullptr;

		pipelineInfo.layout		= m_GraphicsPipeline.layout;
		pipelineInfo.renderPass = m_GraphicsPipeline.renderPass;
		pipelineInfo.subpass	= 0;

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex  = -1;

		if (vkCreateGraphicsPipelines(m_Ctx->m_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline.pipeline) != VK_SUCCESS)
			throw std::runtime_error("Renderer::VKCreateGraphicsPipeline() - Failed to create graphics pipeline!");
	}

	void Renderer::VKCreateGBuffer()
	{
		// To make the code shorter
		GBuffer& gbuffer = m_OffscreenFramebuffer;

		// Albedo (R, G, B, Specular)
		Helpers::CreateImage(m_SwapChain.extent.width, m_SwapChain.extent.height, m_SwapChain.imageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gbuffer.albedo.image, gbuffer.albedo.imageMemory);
		Helpers::CreateImageView(gbuffer.albedo.image, gbuffer.albedo.imageView, m_SwapChain.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		Helpers::TransitionImageLayout(gbuffer.albedo.image, m_SwapChain.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	
		// Position
		Helpers::CreateImage(m_SwapChain.extent.width, m_SwapChain.extent.height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gbuffer.position.image, gbuffer.position.imageMemory);
		Helpers::CreateImageView(gbuffer.position.image, gbuffer.position.imageView, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
		Helpers::TransitionImageLayout(gbuffer.position.image, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		// Normal
		Helpers::CreateImage(m_SwapChain.extent.width, m_SwapChain.extent.height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gbuffer.normal.image, gbuffer.normal.imageMemory);
		Helpers::CreateImageView(gbuffer.normal.image, gbuffer.normal.imageView, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
		Helpers::TransitionImageLayout(gbuffer.normal.image, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		// Depth
		VkFormat depthFormat = FindDepthFormat();
		Helpers::CreateImage(m_SwapChain.extent.width, m_SwapChain.extent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gbuffer.depth.image, gbuffer.depth.imageMemory);
		Helpers::CreateImageView(gbuffer.depth.image, gbuffer.depth.imageView, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
		Helpers::TransitionImageLayout(gbuffer.depth.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		std::array<VkImageView, 4> attachments = {
			gbuffer.albedo.imageView,
			gbuffer.position.imageView,
			gbuffer.normal.imageView,
			gbuffer.depth.imageView
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType		    = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass      = m_GraphicsPipeline.renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments	= attachments.data();
		framebufferInfo.width			= m_SwapChain.extent.width;
		framebufferInfo.height			= m_SwapChain.extent.height;
		framebufferInfo.layers			= 1;

		if (vkCreateFramebuffer(m_Ctx->m_LogicalDevice, &framebufferInfo, nullptr, &gbuffer.framebuffer) != VK_SUCCESS)
			throw std::runtime_error("Renderer::VKCreateGBuffer() - Failed to create the GBuffer!");

		// Sampler
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
		samplerInfo.compareEnable	 = VK_FALSE;
		samplerInfo.compareOp		 = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode		 = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias		 = 0.0f;
		samplerInfo.minLod			 = 0.0f;
		samplerInfo.maxLod			 = 0.0f;

		if (vkCreateSampler(m_Ctx->m_LogicalDevice, &samplerInfo, nullptr, &m_OffscreenFramebuffer.sampler) != VK_SUCCESS)
			throw std::runtime_error("Renderer::VKCreateGBuffer() - Failed to create texture sampler!");
		
	}
	void Renderer::VKCreateSyncObjects()
	{
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		if (vkCreateSemaphore(m_Ctx->m_LogicalDevice, &semaphoreInfo, nullptr, &m_GraphicsPipeline.finishedSemaphore) != VK_SUCCESS ||
			vkCreateSemaphore(m_Ctx->m_LogicalDevice, &semaphoreInfo, nullptr, &m_PresentationPipeline.finishedSemaphore) != VK_SUCCESS ||
			vkCreateFence(m_Ctx->m_LogicalDevice, &fenceInfo, nullptr, &m_InFlightFence) != VK_SUCCESS)
		{
			throw std::runtime_error("Renderer::VKCreateSyncObjects() - Failed to create semaphores!");
		}
	}
	void Renderer::VKCreateDescriptorSetLayout()
	{
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(m_Ctx->m_LogicalDevice, &layoutInfo, nullptr, &m_GraphicsPipeline.descriptorSetLayout) != VK_SUCCESS)
			throw std::runtime_error("Renderer::VKCreateDescriptorSetLayout() - Failed to create descriptor set layout!");
	}
	void Renderer::VKCreateDescriptorPool()
	{
		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(1);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(1);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(m_MaxMeshes);
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		if (vkCreateDescriptorPool(m_Ctx->m_LogicalDevice, &poolInfo, nullptr, &m_GraphicsPipeline.descriptorPool) != VK_SUCCESS)
			throw std::runtime_error("Renderer::VKCreateDescriptorPool() - Failed to create descriptor pool!");
	}
	
	void Renderer::VKCreatePresentPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = m_SwapChain.imageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = nullptr;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcAccessMask = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(m_Ctx->m_LogicalDevice, &renderPassInfo, nullptr, &m_PresentationPipeline.renderPass) != VK_SUCCESS)
			throw std::runtime_error("Renderer::VKCreatePresentPass() - Failed to create render pass!");
	}
	void Renderer::VKCreatePresentDSetLayout()
	{
		std::array<VkDescriptorSetLayoutBinding, 3> bindings;

		for (int i = 0; auto& binding : bindings)
		{
			binding.binding			   = static_cast<uint32_t>(i);
			binding.descriptorCount    = 1;
			binding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			binding.pImmutableSamplers = nullptr;
			binding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;

			i++;
		}
		
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType		= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings	= bindings.data();

		if (vkCreateDescriptorSetLayout(m_Ctx->m_LogicalDevice, &layoutInfo, nullptr, &m_PresentationPipeline.descriptorSetLayout) != VK_SUCCESS)
			throw std::runtime_error("Renderer::VKCreatePresentDSetLayout() - Failed to create present descriptor set layout!");

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType				 = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool	 = m_PresentationPipeline.descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(1);
		allocInfo.pSetLayouts		 = &m_PresentationPipeline.descriptorSetLayout;

		if (vkAllocateDescriptorSets(m_Ctx->m_LogicalDevice, &allocInfo, &m_PresentationDescriptor) != VK_SUCCESS)
			throw std::runtime_error("Renderer::VKCreatePresentDSetLayout() - Failed to allocate presentation descriptor set!");

		std::array<VkDescriptorImageInfo, 3> imageInfos{};

		// Albedo
		imageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfos[0].imageView   = m_OffscreenFramebuffer.albedo.imageView;
		imageInfos[0].sampler     = m_OffscreenFramebuffer.sampler;

		// Position
		imageInfos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfos[1].imageView   = m_OffscreenFramebuffer.position.imageView;
		imageInfos[1].sampler     = m_OffscreenFramebuffer.sampler;

		// Normal
		imageInfos[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfos[2].imageView   = m_OffscreenFramebuffer.normal.imageView;
		imageInfos[2].sampler     = m_OffscreenFramebuffer.sampler;

		std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

		for (int i = 0; auto& descriptorWrite : descriptorWrites)
		{
			descriptorWrite.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet			= m_PresentationDescriptor;
			descriptorWrite.dstBinding		= i;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pImageInfo		= &imageInfos[i];
			i++;
		}

		vkUpdateDescriptorSets(m_Ctx->m_LogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
	void Renderer::VKCreatePresentDPool()
	{
		std::array<VkDescriptorPoolSize, 4> poolSizes{};

		for (auto& poolSize : poolSizes)
		{
			poolSize.type			 = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSize.descriptorCount = 1;
		}

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType		   = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes	   = poolSizes.data();
		poolInfo.maxSets	   = 1;

		if (vkCreateDescriptorPool(m_Ctx->m_LogicalDevice, &poolInfo, nullptr, &m_PresentationPipeline.descriptorPool) != VK_SUCCESS)
			throw std::runtime_error("Renderer::VKCreatePresentDPool() - Failed to create present descriptor pool!");
	}
	void Renderer::VKCreatePresentPipeline()
	{
		Shader vShader("Shaders/presentVert.spv", ShaderType::Vertex);
		Shader fShader("Shaders/presentFrag.spv", ShaderType::Fragment);

		VkPipelineShaderStageCreateInfo shaderStages[] = { vShader.m_ShaderInfo, fShader.m_ShaderInfo };

		auto bindingDescription = Vertex::GetBindingDescription();
		auto attributeDescriptions = Vertex::GetAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_SwapChain.extent.width;
		viewport.height = (float)m_SwapChain.extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = m_SwapChain.extent;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_FALSE;
		depthStencil.depthWriteEnable = VK_FALSE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f;
		depthStencil.maxDepthBounds = 1.0f;
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = {};
		depthStencil.back = {};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &m_PresentationPipeline.descriptorSetLayout;

		if (vkCreatePipelineLayout(m_Ctx->m_LogicalDevice, &pipelineLayoutInfo, nullptr, &m_PresentationPipeline.layout) != VK_SUCCESS)
			throw std::runtime_error("Renderer::VKCreatePresentPipeline() - Failed to create present pipeline layout!");

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;

		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr;

		pipelineInfo.layout = m_PresentationPipeline.layout;
		pipelineInfo.renderPass = m_PresentationPipeline.renderPass;
		pipelineInfo.subpass = 0;

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(m_Ctx->m_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_PresentationPipeline.pipeline) != VK_SUCCESS)
			throw std::runtime_error("Renderer::VKCreatePresentPipeline() - Failed to create present pipeline!");
	}
	void Renderer::VKCreateFramebuffers()
	{
		m_SwapChain.framebuffers.resize(m_SwapChain.imageViews.size());

		for (size_t i = 0; i < m_SwapChain.framebuffers.size(); i++)
		{
			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_PresentationPipeline.renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = &m_SwapChain.imageViews[i];
			framebufferInfo.width = m_SwapChain.extent.width;
			framebufferInfo.height = m_SwapChain.extent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(m_Ctx->m_LogicalDevice, &framebufferInfo, nullptr, &m_SwapChain.framebuffers[i]) != VK_SUCCESS)
				throw std::runtime_error("Renderer::VKCreateFramebuffers() - Failed to create framebuffer!");
		}
	}
	
	SwapChainSupportDetails Renderer::QuerySwapChainSupport(VkPhysicalDevice& device)
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
	VkSurfaceFormatKHR Renderer::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return availableFormat;
		}

		return availableFormats[0];
	}
	VkPresentModeKHR Renderer::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				return availablePresentMode;
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}
	VkExtent2D Renderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
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

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	VkFormat Renderer::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
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

		throw std::runtime_error("Renderer.cpp::Renderer::FindSupportedFormat - Failed to find supported format!");
	}
	VkFormat Renderer::FindDepthFormat()
	{
		return FindSupportedFormat
		(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	bool Renderer::HasStencilComponent(const VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}
}