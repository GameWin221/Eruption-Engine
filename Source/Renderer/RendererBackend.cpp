#include <Core/EnPch.hpp>
#include "RendererBackend.hpp"

#include <Common/Helpers.hpp>

namespace en
{
	Camera* g_DefaultCamera;

	VulkanRendererBackend* g_CurrentBackend;

	void VulkanRendererBackend::Init()
	{
		m_Ctx = &Context::Get();

		g_CurrentBackend = this;

		CameraInfo cameraInfo{};
		cameraInfo.dynamicallyScaled = true;

		if (!g_DefaultCamera)
			g_DefaultCamera = new Camera(cameraInfo);

		m_MainCamera = g_DefaultCamera;

		vkDeviceWaitIdle(m_Ctx->m_LogicalDevice);

		glfwSetFramebufferSizeCallback(Window::Get().m_GLFWWindow, VulkanRendererBackend::FramebufferResizeCallback);

		EN_SUCCESS("Init began!")

			m_Swapchain = std::make_unique<Swapchain>();
			m_Swapchain->CreateSwapchain(m_VSync);

		EN_SUCCESS("Created swapchain!")

			CreateHDROffscreen();

		EN_SUCCESS("Created high dynamic range framebuffer!")

			CreateGBuffer();

		EN_SUCCESS("Created the GBuffer!")

			CreateCommandBuffer();

		EN_SUCCESS("Created command buffer!")

			UpdateSwapchainInputs();

		EN_SUCCESS("Created swapchain inputs!")

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

			InitImGui();

		EN_SUCCESS("Created ImGui context!");

			m_Swapchain->CreateSwapchainFramebuffers(m_ImGui.renderPass);

		EN_SUCCESS("Created swapchain framebuffers!")

			CreateSyncObjects();

		EN_SUCCESS("Created sync objects!")

			m_CameraMatrices = std::make_unique<CameraMatricesBuffer>();

		EN_SUCCESS("Created camera matrices buffer!")

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
	void VulkanRendererBackend::SetVSync(const bool& vSync)
	{
		m_VSync = vSync;
		m_FramebufferResized = true;
	}
	VulkanRendererBackend::~VulkanRendererBackend()
	{
		vkDeviceWaitIdle(m_Ctx->m_LogicalDevice);

		m_ImGui.Destroy();

		for(auto& fence : m_SubmitFences)
			vkDestroyFence(m_Ctx->m_LogicalDevice, fence, nullptr);

		vkFreeCommandBuffers(m_Ctx->m_LogicalDevice, m_Ctx->m_CommandPool, 1U, m_CommandBuffers.data());
	}

	void VulkanRendererBackend::WaitForGPUIdle()
	{
		vkWaitForFences(m_Ctx->m_LogicalDevice, 1U, &m_SubmitFences[m_FrameIndex], VK_TRUE, UINT64_MAX);
	}

	void VulkanRendererBackend::UpdateLights()
	{
		m_Lights.changed = false;

		if (m_Lights.LBO.ambientLight != m_Scene->m_AmbientColor || m_Lights.lastPointLightsSize != m_Scene->GetAllPointLights().size() || m_Lights.lastSpotLightsSize != m_Scene->GetAllSpotLights().size() || m_Lights.lastDirLightsSize != m_Scene->GetAllDirectionalLights().size())
			m_Lights.changed = true;

		m_Lights.LBO.ambientLight = m_Scene->m_AmbientColor;

		auto& pointLights = m_Scene->GetAllPointLights();
		auto& spotLights = m_Scene->GetAllSpotLights();
		auto& dirLights = m_Scene->GetAllDirectionalLights();

		m_Lights.lastPointLightsSize = pointLights.size();
		m_Lights.lastSpotLightsSize = spotLights.size();
		m_Lights.lastDirLightsSize = dirLights.size();

		m_Lights.LBO.activePointLights = 0U;
		m_Lights.LBO.activeSpotLights = 0U;
		m_Lights.LBO.activeDirLights = 0U;

		m_Lights.camera.viewPos = m_MainCamera->m_Position;
		m_Lights.camera.debugMode = m_DebugMode;

		for (int i = 0; i < m_Lights.lastPointLightsSize; i++)
		{
			PointLight::Buffer& buffer = m_Lights.LBO.pointLights[m_Lights.LBO.activePointLights];
			PointLight& light = pointLights[i];

			const glm::vec3 lightCol = light.m_Color * (float)light.m_Active * light.m_Intensity;
			const float     lightRad = light.m_Radius * (float)light.m_Active;

			if (buffer.position != light.m_Position || buffer.radius != lightRad || buffer.color != lightCol)
				m_Lights.changed = true;

			if (lightCol == glm::vec3(0.0) || lightRad == 0.0f)
				continue;

			buffer.position = light.m_Position;
			buffer.color    = lightCol;
			buffer.radius   = lightRad;

			m_Lights.LBO.activePointLights++;
		}
		for (int i = 0; i < m_Lights.lastSpotLightsSize; i++)
		{
			SpotLight::Buffer& buffer = m_Lights.LBO.spotLights[m_Lights.LBO.activeSpotLights];
			SpotLight& light = spotLights[i];

			const glm::vec3 lightColor = light.m_Color * (float)light.m_Active * light.m_Intensity;

			if (buffer.position != light.m_Position || buffer.innerCutoff != light.m_InnerCutoff || buffer.direction != light.m_Direction || buffer.outerCutoff != light.m_OuterCutoff || buffer.color != lightColor || buffer.range != light.m_Range)
				m_Lights.changed = true;

			if (light.m_Range == 0.0f || light.m_Color == glm::vec3(0.0) || light.m_OuterCutoff == 0.0f)
				continue;

			buffer.position	   = light.m_Position;
			buffer.range	   = light.m_Range;
			buffer.outerCutoff = light.m_OuterCutoff;
			buffer.innerCutoff = light.m_InnerCutoff;
			buffer.direction   = light.m_Direction;
			buffer.color	   = lightColor;

			m_Lights.LBO.activeSpotLights++;
		}
		for (int i = 0; i < m_Lights.lastDirLightsSize; i++)
		{
			DirectionalLight::Buffer& buffer = m_Lights.LBO.dirLights[m_Lights.LBO.activeDirLights];
			DirectionalLight& light = dirLights[i];

			glm::vec3 lightCol = light.m_Color * (float)light.m_Active * light.m_Intensity;

			if (buffer.direction != light.m_Direction || buffer.color != lightCol)
				m_Lights.changed = true;

			if (lightCol == glm::vec3(0.0))
				continue;

			buffer.direction = light.m_Direction;
			buffer.color = lightCol;

			m_Lights.LBO.activeDirLights++;
		}

		if (m_Lights.changed)
		{
			// Reset unused lights
			memset(m_Lights.LBO.pointLights + m_Lights.LBO.activePointLights, 0, MAX_POINT_LIGHTS - m_Lights.LBO.activePointLights);
			memset(m_Lights.LBO.spotLights + m_Lights.LBO.activeSpotLights, 0, MAX_SPOT_LIGHTS - m_Lights.LBO.activeSpotLights);
			memset(m_Lights.LBO.dirLights + m_Lights.LBO.activeDirLights, 0, MAX_DIR_LIGHTS - m_Lights.LBO.activeDirLights);

			m_Lights.buffer->MapMemory(&m_Lights.LBO, m_Lights.buffer->m_BufferSize);
		}
	}

	void VulkanRendererBackend::BeginRender()
	{
		VkResult result = vkAcquireNextImageKHR(m_Ctx->m_LogicalDevice, m_Swapchain->m_Swapchain, UINT64_MAX, m_MainSemaphores[m_FrameIndex], VK_NULL_HANDLE, &m_SwapchainImageIndex);

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

		vkResetFences(m_Ctx->m_LogicalDevice, 1U, &m_SubmitFences[m_FrameIndex]);

		vkResetCommandBuffer(m_CommandBuffers[m_FrameIndex], 0U);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(m_CommandBuffers[m_FrameIndex], &beginInfo) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::BeginRender() - Failed to begin recording command buffer!");
	}
	void VulkanRendererBackend::DepthPass()
	{
		if (m_SkipFrame || !m_Scene) return;

		const Pipeline::RenderingInfo info{
			.depthAttachment {
				.imageView	 = m_GBuffer->m_Attachments[3].m_ImageView,
				.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
				.loadOp		 = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp	 = VK_ATTACHMENT_STORE_OP_STORE,

				.clearValue {
					.depthStencil = { 1.0f, 0U}
				}
			},

			.extent = m_Swapchain->GetExtent()
		};

		m_DepthPipeline->Bind(m_CommandBuffers[m_FrameIndex], info);

		m_CameraMatrices->UpdateMatrices(m_MainCamera, m_FrameIndex);

		// Bind the m_MainCamera once per geometry pass
		m_CameraMatrices->Bind(m_CommandBuffers[m_FrameIndex], m_DepthPipeline->m_Layout, m_FrameIndex);

		for (const auto& sceneObjectPair : m_Scene->m_SceneObjects)
		{
			SceneObject* object = sceneObjectPair.second.get();

			if (!object->m_Active || !object->m_Mesh->m_Active) continue;

			// Bind object data (model matrix) once per SceneObject in the m_RenderQueue
			object->GetObjectData().Bind(m_CommandBuffers[m_FrameIndex], m_DepthPipeline->m_Layout);

			for (const auto& subMesh : object->m_Mesh->m_SubMeshes)
			{
				if (!subMesh.m_Active) continue;

				// Bind SubMesh buffers and material once for each SubMesh in the sceneObject->m_Mesh->m_SubMeshes vector
				subMesh.m_VertexBuffer->Bind(m_CommandBuffers[m_FrameIndex]);
				subMesh.m_IndexBuffer->Bind(m_CommandBuffers[m_FrameIndex]);

				vkCmdDrawIndexed(m_CommandBuffers[m_FrameIndex], subMesh.m_IndexBuffer->m_IndicesCount, 1, 0, 0, 0);
			}
		}

		m_DepthPipeline->Unbind(m_CommandBuffers[m_FrameIndex]);
	}
	void VulkanRendererBackend::GeometryPass()
	{
		if (m_SkipFrame || !m_Scene) return;

		m_GBuffer->m_Attachments[0].ChangeLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_CommandBuffers[m_FrameIndex]);
		m_GBuffer->m_Attachments[1].ChangeLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_CommandBuffers[m_FrameIndex]);
		m_GBuffer->m_Attachments[2].ChangeLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_CommandBuffers[m_FrameIndex]);

		const Pipeline::RenderingInfo info{
			.colorAttachments {
				{
					.imageView   = m_GBuffer->m_Attachments[0].m_ImageView,
					.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.loadOp		 = VK_ATTACHMENT_LOAD_OP_CLEAR,
					.storeOp	 = VK_ATTACHMENT_STORE_OP_STORE,
					.clearValue  = { m_Scene->m_AmbientColor.r, m_Scene->m_AmbientColor.g, m_Scene->m_AmbientColor.b, 1.0f },
				},
				{
					.imageView   = m_GBuffer->m_Attachments[1].m_ImageView,
					.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.loadOp		 = VK_ATTACHMENT_LOAD_OP_CLEAR,
					.storeOp	 = VK_ATTACHMENT_STORE_OP_STORE,
				},
				{
					.imageView	 = m_GBuffer->m_Attachments[2].m_ImageView,
					.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.loadOp		 = VK_ATTACHMENT_LOAD_OP_CLEAR,
					.storeOp	 = VK_ATTACHMENT_STORE_OP_STORE,
				}
			}, 

			.depthAttachment{
				.imageView   = m_GBuffer->m_Attachments[3].m_ImageView,
				.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
				.loadOp		 = VK_ATTACHMENT_LOAD_OP_LOAD,
				.storeOp	 = VK_ATTACHMENT_STORE_OP_DONT_CARE,

				.clearValue {
					.depthStencil = { 1.0f, 0U}
				}
			},
			 
			.extent = m_Swapchain->GetExtent()
		};
		
		m_GeometryPipeline->Bind(m_CommandBuffers[m_FrameIndex], info);

		// Bind the m_MainCamera once per geometry pass
		m_CameraMatrices->Bind(m_CommandBuffers[m_FrameIndex], m_GeometryPipeline->m_Layout, m_FrameIndex);

		for (const auto& sceneObjectPair : m_Scene->m_SceneObjects)
		{
			auto& object = sceneObjectPair.second;

			if (!object->m_Active || !object->m_Mesh->m_Active) continue;

			// Bind object data (model matrix) once per SceneObject in the m_RenderQueue
			object->GetObjectData().Bind(m_CommandBuffers[m_FrameIndex], m_GeometryPipeline->m_Layout);

			for (auto& subMesh : object->m_Mesh->m_SubMeshes)
			{
				if (!subMesh.m_Active) continue;

				// Bind SubMesh buffers and material once for each SubMesh in the sceneObject->m_Mesh->m_SubMeshes vector
				subMesh.m_VertexBuffer->Bind(m_CommandBuffers[m_FrameIndex]);
				subMesh.m_IndexBuffer->Bind(m_CommandBuffers[m_FrameIndex]);
				subMesh.m_Material->Bind(m_CommandBuffers[m_FrameIndex], m_GeometryPipeline->m_Layout);

				subMesh.Draw(m_CommandBuffers[m_FrameIndex]);
			}
		}

		m_GeometryPipeline->Unbind(m_CommandBuffers[m_FrameIndex]);
	}
	void VulkanRendererBackend::LightingPass()
	{
		if (m_SkipFrame || !m_Scene) return;

		m_GBuffer->m_Attachments[0].ChangeLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_CommandBuffers[m_FrameIndex]);
		m_GBuffer->m_Attachments[1].ChangeLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_CommandBuffers[m_FrameIndex]);
		m_GBuffer->m_Attachments[2].ChangeLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_CommandBuffers[m_FrameIndex]);

		m_HDROffscreen->m_Attachments[0].ChangeLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_CommandBuffers[m_FrameIndex]);

		const Pipeline::RenderingInfo info{
			.colorAttachments{
				{
					.imageView	 = m_HDROffscreen->m_Attachments[0].m_ImageView,
					.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.loadOp		 = VK_ATTACHMENT_LOAD_OP_CLEAR,
					.storeOp	 = VK_ATTACHMENT_STORE_OP_STORE
				}
			},
			 
			.extent = m_Swapchain->GetExtent()
		};

		m_LightingPipeline->Bind(m_CommandBuffers[m_FrameIndex], info);

		vkCmdPushConstants(m_CommandBuffers[m_FrameIndex], m_LightingPipeline->m_Layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0U, sizeof(Lights::LightsCameraInfo), &m_Lights.camera);

		m_LightingInput->Bind(m_CommandBuffers[m_FrameIndex], m_LightingPipeline->m_Layout);

		vkCmdDraw(m_CommandBuffers[m_FrameIndex], 3U, 1U, 0U, 0U);

		m_LightingPipeline->Unbind(m_CommandBuffers[m_FrameIndex]);
	}
	void VulkanRendererBackend::PostProcessPass()
	{
		if (m_SkipFrame || !m_Scene) return;

		m_Swapchain->ChangeLayout(VK_IMAGE_LAYOUT_GENERAL, m_SwapchainImageIndex, m_CommandBuffers[m_FrameIndex]);

		m_HDROffscreen->m_Attachments[0].ChangeLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_CommandBuffers[m_FrameIndex]);

		const Pipeline::RenderingInfo info{
			.colorAttachments{
				{
					.imageView   = m_Swapchain->m_ImageViews[m_SwapchainImageIndex],
					.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
					.loadOp		 = VK_ATTACHMENT_LOAD_OP_CLEAR,
					.storeOp	 = VK_ATTACHMENT_STORE_OP_STORE
				}
			},

			.extent = m_Swapchain->GetExtent()
		};

		m_TonemappingPipeline->Bind(m_CommandBuffers[m_FrameIndex], info);

		m_PostProcessParams.exposure.value = m_MainCamera->m_Exposure;

		vkCmdPushConstants(m_CommandBuffers[m_FrameIndex], m_TonemappingPipeline->m_Layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0U, sizeof(PostProcessingParams::Exposure), &m_PostProcessParams.exposure);

		m_TonemappingInput->Bind(m_CommandBuffers[m_FrameIndex], m_TonemappingPipeline->m_Layout);

		vkCmdDraw(m_CommandBuffers[m_FrameIndex], 3U, 1U, 0U, 0U);

		m_TonemappingPipeline->Unbind(m_CommandBuffers[m_FrameIndex]);
	}
	void VulkanRendererBackend::AntialiasPass()
	{
		if (m_SkipFrame || !m_Scene || m_PostProcessParams.antialiasingMode == AntialiasingMode::None) return;

		const Pipeline::RenderingInfo info{
			.colorAttachments{
				{
					.imageView   = m_Swapchain->m_ImageViews[m_SwapchainImageIndex],
					.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
					.loadOp		 = VK_ATTACHMENT_LOAD_OP_LOAD,
					.storeOp	 = VK_ATTACHMENT_STORE_OP_STORE
				}
			},

			.extent = m_Swapchain->GetExtent()
		};

		m_AntialiasingPipeline->Bind(m_CommandBuffers[m_FrameIndex], info);

		m_SwapchainInputs[m_SwapchainImageIndex]->Bind(m_CommandBuffers[m_FrameIndex], m_AntialiasingPipeline->m_Layout);

		m_PostProcessParams.antialiasing.texelSizeX = 1.0f / static_cast<float>(m_Swapchain->GetExtent().width);
		m_PostProcessParams.antialiasing.texelSizeY = 1.0f / static_cast<float>(m_Swapchain->GetExtent().height);

		vkCmdPushConstants(m_CommandBuffers[m_FrameIndex], m_AntialiasingPipeline->m_Layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0U, sizeof(PostProcessingParams::Antialiasing), &m_PostProcessParams.antialiasing);

		vkCmdDraw(m_CommandBuffers[m_FrameIndex], 3U, 1U, 0U, 0U);

		m_AntialiasingPipeline->Unbind(m_CommandBuffers[m_FrameIndex]);
	}
	void VulkanRendererBackend::ImGuiPass()
	{
		if (m_SkipFrame || m_ImGuiRenderCallback == nullptr) return;

		m_ImGuiRenderCallback();

		const VkRenderPassBeginInfo renderPassInfo{
			.sType			 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass		 = m_ImGui.renderPass,
			.framebuffer	 = m_Swapchain->m_Framebuffers[m_SwapchainImageIndex],
			.renderArea		 = { {0U, 0U}, m_Swapchain->GetExtent() },
			.clearValueCount = 1U,
			.pClearValues	 = &m_BlackClearValue
		};

		constexpr VkCommandBufferBeginInfo beginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
		};

		if (vkBeginCommandBuffer(m_ImGui.commandBuffers[m_SwapchainImageIndex], &beginInfo) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::BeginRender() - Failed to begin recording ImGui command buffer!");

		vkCmdBeginRenderPass(m_ImGui.commandBuffers[m_SwapchainImageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_ImGui.commandBuffers[m_SwapchainImageIndex]);

		vkCmdEndRenderPass(m_ImGui.commandBuffers[m_SwapchainImageIndex]);

		if (vkEndCommandBuffer(m_ImGui.commandBuffers[m_SwapchainImageIndex]) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::EndRender() - Failed to record ImGui command buffer!");
	}
	void VulkanRendererBackend::EndRender()
	{
		if (m_SkipFrame) return;

		m_Swapchain->ChangeLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, m_SwapchainImageIndex, m_CommandBuffers[m_FrameIndex]);

		if (vkEndCommandBuffer(m_CommandBuffers[m_FrameIndex]) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::EndRender() - Failed to record command buffer!");

		constexpr VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		const VkCommandBuffer commandBuffers[] = { m_CommandBuffers[m_FrameIndex], m_ImGui.commandBuffers[m_SwapchainImageIndex] };

		VkSubmitInfo submitInfo{
			.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1U,
			.pWaitSemaphores	= &m_MainSemaphores[m_FrameIndex],
			.pWaitDstStageMask  = &waitStage,

			.commandBufferCount = 2U,
			.pCommandBuffers	= commandBuffers,

			.signalSemaphoreCount = 1U,
			.pSignalSemaphores	  = &m_PresentSemaphores[m_FrameIndex],
		};

		if (m_ImGuiRenderCallback == nullptr)
		{
			submitInfo.commandBufferCount = 1U;
			submitInfo.pCommandBuffers = &m_CommandBuffers[m_FrameIndex];
		}

		if (vkQueueSubmit(m_Ctx->m_GraphicsQueue, 1U, &submitInfo, m_SubmitFences[m_FrameIndex]) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::EndRender() - Failed to submit command buffer!");

		const VkPresentInfoKHR presentInfo{
			.sType				= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1U,
			.pWaitSemaphores	= &m_PresentSemaphores[m_FrameIndex],

			.swapchainCount = 1U,
			.pSwapchains	= &m_Swapchain->m_Swapchain,
			.pImageIndices  = &m_SwapchainImageIndex,
			.pResults	    = nullptr,
		};

		VkResult result = vkQueuePresentKHR(m_Ctx->m_PresentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized)
			RecreateFramebuffer();
		else if (result != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::EndRender() - Failed to present swap chain image!");

		m_FrameIndex = (m_FrameIndex + 1) % FRAMES_IN_FLIGHT;
	}

	void VulkanRendererBackend::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		g_CurrentBackend->m_FramebufferResized = true;
	}
	void VulkanRendererBackend::RecreateFramebuffer()
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(Window::Get().m_GLFWWindow, &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(Window::Get().m_GLFWWindow, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(m_Ctx->m_LogicalDevice);

		m_Swapchain = std::make_unique<Swapchain>();

		m_Swapchain->CreateSwapchain(m_VSync);

		CreateGBuffer();
		CreateHDROffscreen();

		UpdateSwapchainInputs();
		UpdateLightingInput();
		UpdateTonemappingInput();

		m_Swapchain->CreateSwapchainFramebuffers(m_ImGui.renderPass);

		ImGui_ImplVulkan_SetMinImageCount(m_Swapchain->m_ImageViews.size());

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

		vkDeviceWaitIdle(m_Ctx->m_LogicalDevice);

		m_LightingInput.reset();
		m_SwapchainInputs.clear();

		m_DepthPipeline.reset();

		m_GeometryPipeline.reset();

		m_LightingPipeline.reset();
		m_LightingInput.reset();

		m_TonemappingPipeline.reset();

		m_AntialiasingPipeline.reset();

		m_CameraMatrices.reset();

		m_GBuffer.reset();
		m_HDROffscreen.reset();

		for(auto& cmd : m_CommandBuffers)
			vkResetCommandBuffer(cmd, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

		vkResetFences(m_Ctx->m_LogicalDevice, FRAMES_IN_FLIGHT, m_SubmitFences.data());

		glfwSetFramebufferSizeCallback(Window::Get().m_GLFWWindow, VulkanRendererBackend::FramebufferResizeCallback);

		m_Swapchain = std::make_unique<Swapchain>();

		m_Swapchain->CreateSwapchain(m_VSync);

		CreateGBuffer();
		UpdateSwapchainInputs();
		CreateHDROffscreen();

		CreateCommandBuffer();

		InitDepthPipeline();

		InitGeometryPipeline();

		InitLightingPipeline();

		InitTonemappingPipeline();

		InitAntialiasingPipeline();

		m_Swapchain->CreateSwapchainFramebuffers(m_ImGui.renderPass);

		CreateSyncObjects();

		m_CameraMatrices = std::make_unique<CameraMatricesBuffer>();

		ImGui_ImplVulkan_SetMinImageCount(m_Swapchain->m_ImageViews.size());

		m_ReloadQueued = false;

		vkDeviceWaitIdle(m_Ctx->m_LogicalDevice);

		EN_SUCCESS("Succesfully reloaded the backend");
	}

	void VulkanRendererBackend::SetMainCamera(Camera* camera)
	{
		if (camera)
			m_MainCamera = camera;
		else
			m_MainCamera = g_DefaultCamera;
	}
	void VulkanRendererBackend::CreateGBuffer()
	{
		const DynamicFramebuffer::AttachmentInfo albedo{
			.format = m_Swapchain->GetFormat(),
			.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		constexpr DynamicFramebuffer::AttachmentInfo position{
			.format = VK_FORMAT_R16G16B16A16_SFLOAT,
			.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		constexpr DynamicFramebuffer::AttachmentInfo normal{
			.format = VK_FORMAT_R16G16B16A16_SFLOAT,
			.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		constexpr DynamicFramebuffer::AttachmentInfo depth{
			.format			  = VK_FORMAT_D32_SFLOAT,
			.imageAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT,
			.imageUsageFlags  = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			.initialLayout	  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};

		m_GBuffer = std::make_unique<DynamicFramebuffer>(std::vector<DynamicFramebuffer::AttachmentInfo>{ albedo, position, normal, depth }, m_Swapchain->GetExtent());
	}
	void VulkanRendererBackend::CreateHDROffscreen()
	{
		constexpr DynamicFramebuffer::AttachmentInfo attachment{
			.format		   = VK_FORMAT_R16G16B16A16_SFLOAT,
			.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		m_HDROffscreen = std::make_unique<DynamicFramebuffer>(std::vector<DynamicFramebuffer::AttachmentInfo>{attachment}, m_Swapchain->GetExtent());
	}

	void VulkanRendererBackend::InitDepthPipeline()
	{
		m_DepthPipeline = std::make_unique<Pipeline>();

		Shader vShader("Shaders/DepthVertex.spv", ShaderType::Vertex);

		constexpr VkPushConstantRange objectPushConstant {
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset		= 0U,
			.size		= sizeof(PerObjectData)
		};

		const Pipeline::PipelineInfo pipelineInfo{
			.depthFormat		= m_GBuffer->m_Attachments[3].m_Format,
			.vShader			= &vShader,
			.descriptorLayouts  = { CameraMatricesBuffer::GetLayout() },
			.pushConstantRanges = { objectPushConstant },
			.useVertexBindings  = true,
			.enableDepthTest    = true
		};

		m_DepthPipeline->CreatePipeline(pipelineInfo);
	}
	void VulkanRendererBackend::InitGeometryPipeline()
	{
		m_GeometryPipeline = std::make_unique<Pipeline>();

		Shader vShader("Shaders/GeometryVertex.spv", ShaderType::Vertex);
		Shader fShader("Shaders/GeometryFragment.spv", ShaderType::Fragment);

		constexpr VkPushConstantRange objectPushConstant{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset		= 0U,
			.size		= sizeof(PerObjectData),
		};

		constexpr VkPushConstantRange materialPushConstant{
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.offset		= sizeof(PerObjectData),
			.size		= 24U,
		};

		const Pipeline::PipelineInfo pipelineInfo{
			.colorFormats		= { m_GBuffer->m_Attachments[0].m_Format, m_GBuffer->m_Attachments[1].m_Format, m_GBuffer->m_Attachments[2].m_Format },
			.depthFormat		= m_GBuffer->m_Attachments[3].m_Format,
			.vShader			= &vShader,
			.fShader			= &fShader,
			.descriptorLayouts  = { CameraMatricesBuffer::GetLayout(), Material::GetLayout() },
			.pushConstantRanges = { objectPushConstant, materialPushConstant },
			.useVertexBindings  = true,
			.enableDepthTest	= true,
			.enableDepthWrite   = false,
			.compareOp			= VK_COMPARE_OP_LESS_OR_EQUAL,
		};

		m_GeometryPipeline->CreatePipeline(pipelineInfo);
	}

	void VulkanRendererBackend::UpdateLightingInput()
	{
		const PipelineInput::ImageInfo albedo{
			.index		  = 0U,
			.imageView	  = m_GBuffer->m_Attachments[0].m_ImageView,
			.imageSampler = m_GBuffer->m_Sampler,
		};

		const PipelineInput::ImageInfo position{
			.index		  = 1U,
			.imageView    = m_GBuffer->m_Attachments[1].m_ImageView,
			.imageSampler = m_GBuffer->m_Sampler
		};

		const PipelineInput::ImageInfo normal{
			.index		  = 2U,
			.imageView	  = m_GBuffer->m_Attachments[2].m_ImageView,
			.imageSampler = m_GBuffer->m_Sampler
		};

		const PipelineInput::BufferInfo buffer{
			.index  = 3U,
			.buffer = m_Lights.buffer->GetHandle(),
			.size   = sizeof(m_Lights.LBO)
		};

		if (!m_LightingInput)
			m_LightingInput = std::make_unique<PipelineInput>(std::vector<PipelineInput::ImageInfo>{albedo, position, normal}, buffer);
		else
			m_LightingInput->UpdateDescriptorSet(std::vector<PipelineInput::ImageInfo>{albedo, position, normal}, buffer);
	}
	void VulkanRendererBackend::InitLightingPipeline()
	{
		m_LightingPipeline = std::make_unique<Pipeline>();

		if (!m_Lights.buffer)
			m_Lights.buffer = std::make_unique<MemoryBuffer>(sizeof(m_Lights.LBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		UpdateLightingInput();

		constexpr VkPushConstantRange cameraPushConstant{
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.offset		= 0U,
			.size		= sizeof(Lights::LightsCameraInfo)
		};

		Shader vShader("Shaders/FullscreenTriVert.spv", ShaderType::Vertex);
		Shader fShader("Shaders/LightingFrag.spv", ShaderType::Fragment);

		const Pipeline::PipelineInfo pipelineInfo{
			.colorFormats		= { m_HDROffscreen->m_Attachments[0].m_Format},
			.vShader			= &vShader,
			.fShader			= &fShader,
			.descriptorLayouts  = { m_LightingInput->m_DescriptorLayout },
			.pushConstantRanges = { cameraPushConstant },
			.cullMode			= VK_CULL_MODE_FRONT_BIT
		};

		m_LightingPipeline->CreatePipeline(pipelineInfo);
	}

	void VulkanRendererBackend::UpdateTonemappingInput()
	{
		const PipelineInput::ImageInfo HDRColorBuffer{
			.index		  = 0U,
			.imageView    = m_HDROffscreen->m_Attachments[0].m_ImageView,
			.imageSampler = m_HDROffscreen->m_Sampler
		};

		if (!m_TonemappingInput)
			m_TonemappingInput = std::make_unique<PipelineInput>(std::vector<PipelineInput::ImageInfo>{ HDRColorBuffer });
		else
			m_TonemappingInput->UpdateDescriptorSet(std::vector<PipelineInput::ImageInfo>{ HDRColorBuffer });
	}
	void VulkanRendererBackend::InitTonemappingPipeline()
	{
		m_TonemappingPipeline = std::make_unique<Pipeline>();

		UpdateTonemappingInput();

		constexpr VkPushConstantRange exposurePushConstant{
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.offset		= 0U,
			.size		= sizeof(PostProcessingParams::Exposure)
		};

		Shader vShader("Shaders/FullscreenTriVert.spv", ShaderType::Vertex);
		Shader fShader("Shaders/TonemapFrag.spv", ShaderType::Fragment);

		const Pipeline::PipelineInfo pipelineInfo{
			.colorFormats		= { m_Swapchain->GetFormat() },
			.vShader			= &vShader,
			.fShader			= &fShader,
			.descriptorLayouts  = { m_TonemappingInput->m_DescriptorLayout },
			.pushConstantRanges = { exposurePushConstant },
			.cullMode			= VK_CULL_MODE_FRONT_BIT,
		};

		m_TonemappingPipeline->CreatePipeline(pipelineInfo);
	}

	void VulkanRendererBackend::UpdateSwapchainInputs()
	{
		m_SwapchainInputs.resize(m_Swapchain->m_ImageViews.size());

		for (int i = 0; i < m_SwapchainInputs.size(); i++)
		{
			const PipelineInput::ImageInfo image{
				.index		  = 0U,
				.imageView	  = m_Swapchain->m_ImageViews[i],
				.imageSampler = m_GBuffer->m_Sampler,
				.imageLayout  = VK_IMAGE_LAYOUT_GENERAL,
			};

			if (!m_SwapchainInputs[i])
				m_SwapchainInputs[i] = std::make_unique<PipelineInput>(std::vector<PipelineInput::ImageInfo>{ image });
			else
				m_SwapchainInputs[i]->UpdateDescriptorSet(std::vector<PipelineInput::ImageInfo>{ image });
		}
	}
	void VulkanRendererBackend::InitAntialiasingPipeline()
	{
		if (m_PostProcessParams.antialiasingMode == AntialiasingMode::None) return;

		m_AntialiasingPipeline = std::make_unique<Pipeline>();

		constexpr VkPushConstantRange antialiasingPushConstant{
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.offset = 0U,
			.size = sizeof(PostProcessingParams::Antialiasing)
		};

		Shader vShader("Shaders/FullscreenTriVert.spv", ShaderType::Vertex);
		Shader fShader("Shaders/FXAA.spv", ShaderType::Fragment);

		const Pipeline::PipelineInfo pipelineInfo{
			.colorFormats		= { m_Swapchain->GetFormat() },
			.vShader			= &vShader,
			.fShader			= &fShader,
			.descriptorLayouts  = { m_SwapchainInputs[0]->m_DescriptorLayout},
			.pushConstantRanges = { antialiasingPushConstant },
			.cullMode			 = VK_CULL_MODE_FRONT_BIT,
		};

		m_AntialiasingPipeline->CreatePipeline(pipelineInfo);
	}

	void VulkanRendererBackend::CreateCommandBuffer()
	{
		const VkCommandBufferAllocateInfo allocInfo{
			.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool		= m_Ctx->m_CommandPool,
			.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = FRAMES_IN_FLIGHT
		};

		if (vkAllocateCommandBuffers(m_Ctx->m_LogicalDevice, &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::GCreateCommandBuffer() - Failed to allocate command buffer!");
	}
	void VulkanRendererBackend::CreateSyncObjects()
	{
		constexpr VkFenceCreateInfo fenceInfo{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT
		};

		for(auto& fence : m_SubmitFences)
			if (vkCreateFence(m_Ctx->m_LogicalDevice, &fenceInfo, nullptr, &fence) != VK_SUCCESS)
				EN_ERROR("VulkanRendererBackend::CreateSyncObjects() - Failed to create submit fences!");

		constexpr VkSemaphoreCreateInfo semaphoreInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
		};

		for (auto& semaphore : m_MainSemaphores)
			if (vkCreateSemaphore(m_Ctx->m_LogicalDevice, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS)
				EN_ERROR("Pipeline::CreateSyncSemaphore - Failed to create main semaphores!");

		for (auto& semaphore : m_PresentSemaphores)
			if (vkCreateSemaphore(m_Ctx->m_LogicalDevice, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS)
				EN_ERROR("Pipeline::CreateSyncSemaphore - Failed to create present semaphores!");
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
		ImGui::StyleColorsDark();

		const VkAttachmentDescription colorAttachment{
			.format			= m_Swapchain->GetFormat(),
			.samples		= VK_SAMPLE_COUNT_1_BIT,
			.loadOp			= VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp		= VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout  = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		};

		constexpr VkAttachmentReference colorAttachmentRef{
			.attachment = 0U,
			.layout	  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};

		const VkSubpassDescription subpass{
			.pipelineBindPoint	  = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1U,
			.pColorAttachments    = &colorAttachmentRef
		};

		constexpr VkSubpassDependency dependency{
			.srcSubpass    = VK_SUBPASS_EXTERNAL,
			.dstSubpass    = 0U,
			.srcStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
		};

		const VkRenderPassCreateInfo renderPassInfo{
			.sType			 = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = 1U,
			.pAttachments	 = &colorAttachment,
			.subpassCount	 = 1U,
			.pSubpasses		 = &subpass,
			.dependencyCount = 1U,
			.pDependencies	 = &dependency,
		};

		if (vkCreateRenderPass(m_Ctx->m_LogicalDevice, &renderPassInfo, nullptr, &m_ImGui.renderPass) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::InitImGui() - Failed to create ImGui's render pass!");

		constexpr VkDescriptorPoolSize poolSizes[]{
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

		const VkDescriptorPoolCreateInfo poolInfo{
			.sType		   = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.flags		   = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
			.maxSets	   = 1000 * IM_ARRAYSIZE(poolSizes),
			.poolSizeCount = (uint32_t)IM_ARRAYSIZE(poolSizes),
			.pPoolSizes	   = poolSizes
		};

		if (vkCreateDescriptorPool(m_Ctx->m_LogicalDevice, &poolInfo, nullptr, &m_ImGui.descriptorPool) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::InitImGui() - Failed to create ImGui's descriptor pool!");

		ImGui_ImplGlfw_InitForVulkan(Window::Get().m_GLFWWindow, true);

		ImGui_ImplVulkan_InitInfo initInfo{
			.Instance		 = m_Ctx->m_Instance,
			.PhysicalDevice  = m_Ctx->m_PhysicalDevice,
			.Device			 = m_Ctx->m_LogicalDevice,
			.QueueFamily	 = m_Ctx->m_QueueFamilies.graphics.value(),
			.Queue			 = m_Ctx->m_GraphicsQueue,
			.DescriptorPool  = m_ImGui.descriptorPool,
			.MinImageCount   = static_cast<uint32_t>(m_Swapchain->m_ImageViews.size()),
			.ImageCount		 = static_cast<uint32_t>(m_Swapchain->m_ImageViews.size()),
			.CheckVkResultFn = ImGuiCheckResult
		};

		ImGui_ImplVulkan_Init(&initInfo, m_ImGui.renderPass);

		VkCommandBuffer cmd = Helpers::BeginSingleTimeGraphicsCommands();
		ImGui_ImplVulkan_CreateFontsTexture(cmd);
		Helpers::EndSingleTimeGraphicsCommands(cmd);

		m_ImGui.commandBuffers.resize(m_Swapchain->m_ImageViews.size());

		Helpers::CreateCommandPool(m_ImGui.commandPool, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		Helpers::CreateCommandBuffers(m_ImGui.commandBuffers.data(), static_cast<uint32_t>(m_ImGui.commandBuffers.size()), m_ImGui.commandPool);

		ImGui_ImplVulkanH_SelectSurfaceFormat(m_Ctx->m_PhysicalDevice, m_Ctx->m_WindowSurface, &m_Swapchain->GetFormat(), 1, VK_COLORSPACE_SRGB_NONLINEAR_KHR);
	}
}