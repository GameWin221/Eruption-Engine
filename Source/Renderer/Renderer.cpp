#include "Renderer.hpp"

#include <Common/Helpers.hpp>

namespace en
{
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

	void Renderer::Update()
	{
		if(m_Scene)
			m_Scene->UpdateSceneCPU();
	}
	void Renderer::PreRender()
	{
		if (m_Scene)
		{
			if (m_Scene->m_GlobalDescriptorChanged)
				ResetAllFrames();
			else
				WaitForActiveFrame();
		}
		else 
			WaitForActiveFrame();

		if (m_Scene)
			m_Scene->UpdateSceneGPU();
		
		m_CameraBuffer->UpdateBuffer(
			m_FrameIndex,
			m_Scene->m_MainCamera,
			m_Scene->m_CSM.cascadeSplitDistances,
			m_Scene->m_CSM.cascadeFrustumSizeRatios,
			m_Scene->m_CSM.cascadeMatrices,
			m_Swapchain->GetExtent(),
			m_DebugMode
		);
		m_CameraBuffer->MapBuffer(m_FrameIndex);
	}
	void Renderer::Render()
	{
		MeasureFrameTime();

		BeginRender();

		if (m_Scene)
		{
			ShadowPass();

			DepthPass();

			ForwardPass();
		}

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

	void Renderer::WaitForActiveFrame()
	{
		vkWaitForFences(
			g_Ctx->m_LogicalDevice, 
			1U, &m_Frames[m_FrameIndex].submitFence, 
			VK_TRUE, UINT64_MAX
		);
	}
	void Renderer::ResetAllFrames()
	{
		for (const auto& frame : m_Frames)
		{
			vkWaitForFences(
				g_Ctx->m_LogicalDevice,
				1U, &frame.submitFence,
				VK_TRUE, UINT64_MAX
			);

			vkResetCommandBuffer(frame.commandBuffer, 0U);
		}
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
	void Renderer::ShadowPass()
	{
		if (m_SkipFrame) return;

		for (auto& light : m_Scene->m_PointLights)
		{
			if (light.m_ShadowmapIndex == -1)
				continue;

			m_Scene->m_PointShadowMaps[light.m_ShadowmapIndex].image->ChangeLayout(
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				m_Frames[m_FrameIndex].commandBuffer
			);

			for (uint32_t cubeSide = 0U; cubeSide < 6U; cubeSide++)
			{
				m_Scene->m_PerspectiveShadowsRenderPass->Begin(
					m_Frames[m_FrameIndex].commandBuffer,
					m_Scene->m_PointShadowMaps[light.m_ShadowmapIndex].framebuffers[cubeSide]->GetHandle(),
					m_Scene->m_PointShadowMaps[light.m_ShadowmapIndex].framebuffers[cubeSide]->m_Extent
				);

				m_Scene->m_PointShadowPipeline->Bind(
					m_Frames[m_FrameIndex].commandBuffer,
					m_Scene->m_PointShadowMaps[light.m_ShadowmapIndex].framebuffers[cubeSide]->m_Extent,
					VK_CULL_MODE_FRONT_BIT
				);

				m_Scene->m_PointShadowPipeline->BindDescriptorSet(m_Scene->m_ShadowsDescriptorSet->GetHandle());

				for (const auto& [name, sceneObject] : m_Scene->m_SceneObjects)
				{
					if (!sceneObject->m_Active || !sceneObject->m_Mesh->m_Active) continue;

					uint32_t pushConstant[3]{ sceneObject->GetMatrixIndex(), light.m_ShadowmapIndex, cubeSide };

					m_Scene->m_PointShadowPipeline->PushConstants(
						pushConstant,
						sizeof(uint32_t) * 3,
						0U,
						VK_SHADER_STAGE_VERTEX_BIT
					);

					for (const auto& subMesh : sceneObject->m_Mesh->m_SubMeshes)
					{
						if (!subMesh.m_Active) continue;

						m_Scene->m_PointShadowPipeline->BindVertexBuffer(subMesh.m_VertexBuffer);
						m_Scene->m_PointShadowPipeline->BindIndexBuffer(subMesh.m_IndexBuffer);

						m_Scene->m_PointShadowPipeline->DrawIndexed(subMesh.m_IndexCount);
					}
				}

				m_Scene->m_PerspectiveShadowsRenderPass->End();
			}
			m_Scene->m_PointShadowMaps[light.m_ShadowmapIndex].image->m_CurrentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
		for (auto& light : m_Scene->m_SpotLights)
		{
			if (light.m_ShadowmapIndex == -1)
				continue;

			m_Scene->m_SpotShadowMaps[light.m_ShadowmapIndex].image->ChangeLayout(
				VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				m_Frames[m_FrameIndex].commandBuffer
			);

			m_Scene->m_DirShadowsRenderPass->Begin(
				m_Frames[m_FrameIndex].commandBuffer,
				m_Scene->m_SpotShadowMaps[light.m_ShadowmapIndex].framebuffer->GetHandle(),
				m_Scene->m_SpotShadowMaps[light.m_ShadowmapIndex].framebuffer->m_Extent
			);

			m_Scene->m_SpotShadowPipeline->Bind(
				m_Frames[m_FrameIndex].commandBuffer,
				m_Scene->m_SpotShadowMaps[light.m_ShadowmapIndex].framebuffer->m_Extent,
				VK_CULL_MODE_FRONT_BIT
			);

			m_Scene->m_SpotShadowPipeline->BindDescriptorSet(m_Scene->m_ShadowsDescriptorSet->GetHandle());

			for (const auto& [name, sceneObject] : m_Scene->m_SceneObjects)
			{
				if (!sceneObject->m_Active || !sceneObject->m_Mesh->m_Active) continue;

				uint32_t pushConstant[2]{ sceneObject->GetMatrixIndex(), light.m_ShadowmapIndex };

				m_Scene->m_SpotShadowPipeline->PushConstants(
					pushConstant,
					sizeof(uint32_t) * 2,
					0U,
					VK_SHADER_STAGE_VERTEX_BIT
				);

				for (const auto& subMesh : sceneObject->m_Mesh->m_SubMeshes)
				{
					if (!subMesh.m_Active) continue;

					m_Scene->m_SpotShadowPipeline->BindVertexBuffer(subMesh.m_VertexBuffer);
					m_Scene->m_SpotShadowPipeline->BindIndexBuffer(subMesh.m_IndexBuffer);

					m_Scene->m_SpotShadowPipeline->DrawIndexed(subMesh.m_IndexCount);
				}
			}

			m_Scene->m_SpotShadowMaps[light.m_ShadowmapIndex].image->m_CurrentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			m_Scene->m_DirShadowsRenderPass->End();
		}
		for (auto& light : m_Scene->m_DirectionalLights)
		{
			if (light.m_ShadowmapIndex == -1)
				continue;

			for (uint32_t cascadeIndex = 0U; cascadeIndex < SHADOW_CASCADES; cascadeIndex++)
			{
				m_Scene->m_DirShadowMaps[light.m_ShadowmapIndex * SHADOW_CASCADES + cascadeIndex].image->ChangeLayout(
					VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
					VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
					VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
					m_Frames[m_FrameIndex].commandBuffer
				);

				m_Scene->m_DirShadowsRenderPass->Begin(
					m_Frames[m_FrameIndex].commandBuffer,
					m_Scene->m_DirShadowMaps[light.m_ShadowmapIndex * SHADOW_CASCADES + cascadeIndex].framebuffer->GetHandle(),
					m_Scene->m_DirShadowMaps[light.m_ShadowmapIndex * SHADOW_CASCADES + cascadeIndex].framebuffer->m_Extent
				);

				m_Scene->m_DirShadowPipeline->Bind(
					m_Frames[m_FrameIndex].commandBuffer,
					m_Scene->m_DirShadowMaps[light.m_ShadowmapIndex * SHADOW_CASCADES + cascadeIndex].framebuffer->m_Extent,
					VK_CULL_MODE_FRONT_BIT
				);

				m_Scene->m_DirShadowPipeline->BindDescriptorSet(m_Scene->m_ShadowsDescriptorSet->GetHandle(), 0U);
				m_Scene->m_DirShadowPipeline->BindDescriptorSet(m_CameraBuffer->GetDescriptorHandle(m_FrameIndex), 1U);

				for (const auto& [name, sceneObject] : m_Scene->m_SceneObjects)
				{
					if (!sceneObject->m_Active || !sceneObject->m_Mesh->m_Active) continue;

					uint32_t pushConstant[3] { sceneObject->GetMatrixIndex(), light.m_ShadowmapIndex, cascadeIndex };

					m_Scene->m_DirShadowPipeline->PushConstants(
						pushConstant,
						sizeof(uint32_t)*3,
						0U,
						VK_SHADER_STAGE_VERTEX_BIT
					);

					for (const auto& subMesh : sceneObject->m_Mesh->m_SubMeshes)
					{
						if (!subMesh.m_Active) continue;

						m_Scene->m_DirShadowPipeline->BindVertexBuffer(subMesh.m_VertexBuffer);
						m_Scene->m_DirShadowPipeline->BindIndexBuffer(subMesh.m_IndexBuffer);

						m_Scene->m_DirShadowPipeline->DrawIndexed(subMesh.m_IndexCount);
					}
				}

				m_Scene->m_DirShadowMaps[light.m_ShadowmapIndex * SHADOW_CASCADES + cascadeIndex].image->m_CurrentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				m_Scene->m_DirShadowsRenderPass->End();
			}
		}
	}
	void Renderer::DepthPass()
	{
		if (m_SkipFrame) return;

		m_DepthRenderPass->Begin(
			m_Frames[m_FrameIndex].commandBuffer,
			m_DepthFramebuffer->GetHandle(),
			m_DepthFramebuffer->m_Extent
		);

		m_DepthPipeline->Bind(m_Frames[m_FrameIndex].commandBuffer, m_Swapchain->GetExtent());

		m_DepthPipeline->BindDescriptorSet(m_CameraBuffer->GetDescriptorHandle(m_FrameIndex), 0U);
		m_DepthPipeline->BindDescriptorSet(m_Scene->m_GlobalDescriptorSet, 1U);

		for (const auto& [name, sceneObject] : m_Scene->m_SceneObjects)
		{
			if (!sceneObject->m_Active || !sceneObject->m_Mesh->m_Active) continue;

			m_DepthPipeline->PushConstants(&sceneObject->GetMatrixIndex(), sizeof(uint32_t), 0U, VK_SHADER_STAGE_VERTEX_BIT);

			for (const auto& subMesh : sceneObject->m_Mesh->m_SubMeshes)
			{
				if (!subMesh.m_Active) continue;

				m_DepthPipeline->BindVertexBuffer(subMesh.m_VertexBuffer);
				m_DepthPipeline->BindIndexBuffer(subMesh.m_IndexBuffer);

				m_DepthPipeline->DrawIndexed(subMesh.m_IndexCount);
			}
		}

		m_DepthRenderPass->End();
	}
	void Renderer::ForwardPass()
	{
		if (m_SkipFrame) return;

		m_RenderPass->Begin(
			m_Frames[m_FrameIndex].commandBuffer,
			m_RenderFramebuffers[m_Swapchain->m_ImageIndex]->GetHandle(),
			m_RenderFramebuffers[m_Swapchain->m_ImageIndex]->m_Extent
		);

			m_Pipeline->Bind(m_Frames[m_FrameIndex].commandBuffer, m_Swapchain->GetExtent());

			m_Pipeline->BindDescriptorSet(m_CameraBuffer->GetDescriptorHandle(m_FrameIndex), 0U);
			m_Pipeline->BindDescriptorSet(m_Scene->m_GlobalDescriptorSet, 1U);

			for (const auto& [name, sceneObject] : m_Scene->m_SceneObjects)
			{
				if(!sceneObject->m_Active || !sceneObject->m_Mesh->m_Active) continue;

				m_Pipeline->PushConstants(&sceneObject->GetMatrixIndex(), sizeof(uint32_t), 0U, VK_SHADER_STAGE_VERTEX_BIT);

				for (const auto& subMesh : sceneObject->m_Mesh->m_SubMeshes)
				{
					if (!subMesh.m_Active) continue;

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

		m_ImGuiRenderPass->Begin(
			m_Frames[m_FrameIndex].commandBuffer,
			m_ImGuiFramebuffers[m_Swapchain->m_ImageIndex]->GetHandle(),
			m_ImGuiFramebuffers[m_Swapchain->m_ImageIndex]->m_Extent
		);

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

		CreateDepthPass();

		CreateForwardPass();

		CreateImGuiRenderPass();

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

		DestroyPerFrameData();

		CreateBackend(false);

		m_ReloadQueued = false;
	}
	void Renderer::CreateBackend(bool newImGui)
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

			CreateDepthPass();

		EN_SUCCESS("Created the depth pass!")

			CreateDepthPipeline();

		EN_SUCCESS("Created the depth pipeline!")

			CreateForwardPass();

		EN_SUCCESS("Created the forward pass!")

			CreateForwardPipeline();

		EN_SUCCESS("Created the forward pipeline!")

			//CreateShadowPass();

		//EN_SUCCESS("Created the shadow pass!")

		//	CreatePointShadowPipeline();
		//	CreateSpotShadowPipeline();
		//	CreateDirShadowPipeline();
		//
		//EN_SUCCESS("Created the shadow pipelines (point, spot, dir)!")

		if (newImGui)
			CreateImGuiContext();
		else
			CreateImGuiRenderPass();
		
		EN_SUCCESS("Created the ImGui context!")

		EN_SUCCESS("Created the renderer Vulkan backend");

		vkDeviceWaitIdle(g_Ctx->m_LogicalDevice);
	}

	void Renderer::CreateDepthPass()
	{
		RenderPassAttachment depthAttachment{
			.imageView = m_DepthBuffer->GetViewHandle(),

			.format = m_DepthBuffer->m_Format,

			.refLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,

			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		};

		m_DepthRenderPass = MakeHandle<RenderPass>(RenderPassAttachment{}, depthAttachment);

		m_DepthFramebuffer = MakeHandle<Framebuffer>(m_DepthRenderPass, m_Swapchain->GetExtent(), RenderPassAttachment{}, depthAttachment);
	}
	void Renderer::CreateDepthPipeline()
	{
		constexpr VkPushConstantRange model{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 0U,
			.size = sizeof(uint32_t),
		};

		GraphicsPipeline::CreateInfo info{
			.renderPass = m_DepthRenderPass,

			.vShader = "Shaders/Depth.spv",

			.descriptorLayouts {CameraBuffer::GetLayout(), Scene::GetGlobalDescriptorLayout()},
			.pushConstantRanges {model},

			.useVertexBindings = true,
			.enableDepthTest = true,
			.enableDepthWrite = true,
			.blendEnable = false,

			.compareOp = VK_COMPARE_OP_LESS,
			.polygonMode = VK_POLYGON_MODE_FILL,
		};

		m_DepthPipeline = MakeHandle<GraphicsPipeline>(info);
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

		std::vector<RenderPassAttachment> attachments{};
		for (int i = 0; i < m_Swapchain->m_ImageViews.size(); i++)
		{
			attachments.emplace_back(RenderPassAttachment{
				.imageView = m_Swapchain->m_ImageViews[i],

				.format = m_Swapchain->GetFormat(),

				.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				.refLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,

				.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,

				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			});
		}

		RenderPassAttachment depthAttachment{
			.imageView = m_DepthBuffer->GetViewHandle(),

			.format = m_DepthBuffer->m_Format,

			.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.refLayout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.finalLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,

			.loadOp  = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		};

		m_RenderPass = MakeHandle<RenderPass>(attachments[0], depthAttachment, dependencies);

		m_RenderFramebuffers.resize(m_Swapchain->m_ImageViews.size());
		for (uint32_t i = 0U; i < m_Swapchain->m_ImageViews.size(); i++)
			m_RenderFramebuffers[i] = MakeHandle<Framebuffer>(m_RenderPass, m_Swapchain->GetExtent(), attachments[i], depthAttachment);
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

			.descriptorLayouts {CameraBuffer::GetLayout(), Scene::GetGlobalDescriptorLayout()},
			.pushConstantRanges {model, material},

			.useVertexBindings = true,
			.enableDepthTest   = true,
			.enableDepthWrite  = false,
			.blendEnable	   = false,

			.compareOp	 = VK_COMPARE_OP_LESS_OR_EQUAL,
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
			0U,
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

	void Renderer::CreateImGuiRenderPass()
	{
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

		std::vector<RenderPassAttachment> attachments{};
		for (int i = 0; i < m_Swapchain->m_ImageViews.size(); i++)
		{
			attachments.emplace_back(RenderPassAttachment{
				.imageView = m_Swapchain->m_ImageViews[i],

				.format = m_Swapchain->GetFormat(),

				.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				.refLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,

				.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,

				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			});
		}

		m_ImGuiRenderPass = MakeHandle<RenderPass>(attachments[0], RenderPassAttachment{}, dependencies);
		
		m_ImGuiFramebuffers.resize(m_Swapchain->m_ImageViews.size());
		for (uint32_t i = 0U; i < m_Swapchain->m_ImageViews.size(); i++)
			m_ImGuiFramebuffers[i] = MakeHandle<Framebuffer>(m_ImGuiRenderPass, m_Swapchain->GetExtent(), attachments[i]);
	}
	void Renderer::CreateImGuiContext()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		CreateImGuiRenderPass();

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

		ImGui_ImplVulkan_Init(&initInfo, m_ImGuiRenderPass->GetHandle());

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