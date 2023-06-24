#include "Renderer.hpp"

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

		DestroyPerFrameData();
	}

	void Renderer::Update()
	{
		m_ClusterFrustumChanged = false;

		if (m_Scene)
		{
			m_Scene->UpdateSceneCPU();
			UpdateCSM();

			glm::mat4 oldInvProj = m_CameraBuffer->m_CBOs[m_FrameIndex].invProj;

			m_CameraBuffer->UpdateBuffer(
				m_FrameIndex,
				m_Scene->m_MainCamera,
				m_CSM.cascadeSplitDistances,
				m_CSM.cascadeFrustumSizeRatios,
				m_CSM.cascadeMatrices,
				m_Swapchain->GetExtent(),
				m_DebugMode
			);

			if (oldInvProj != m_CameraBuffer->m_CBOs[m_FrameIndex].invProj)
				m_ClusterFrustumChanged = true;
		}
	}
	void Renderer::PreRender()
	{
		if (m_Scene)
		{
			if (m_Scene->m_GlobalDescriptorChanged)
				ResetAllFrames();
			else
				WaitForActiveFrame();

			m_CameraBuffer->MapBuffer(m_FrameIndex);
		}
		else 
			WaitForActiveFrame();
	}
	void Renderer::Render()
	{
		MeasureFrameTime();

		BeginRender();

		m_Scene->UpdateSceneGPU(m_Frames[m_FrameIndex].commandBuffer);

		if (m_Scene)
		{
			ShadowPass();
			ClusterComputePass();
			DepthPass();
			SSAOPass();
			ForwardPass();
			AntialiasingPass();
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

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
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
		
		for (const auto& i : m_Scene->m_ActivePointLightsShadowIDs)
		{
			const auto& light = m_Scene->m_PointLights[i];

			m_PointShadowMaps[light.m_ShadowmapIndex]->ChangeLayout(
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				m_Frames[m_FrameIndex].commandBuffer
			);

			for (uint32_t cubeSide = 0U; cubeSide < 6U; cubeSide++)
			{
				GraphicsPass::RenderInfo renderInfo{
					.colorAttachmentView = m_PointShadowMaps[light.m_ShadowmapIndex]->GetLayerViewHandle(cubeSide),
					.depthAttachmentView = m_PointShadowDepthBuffer->GetViewHandle(),

					.colorAttachmentLayout = m_PointShadowMaps[light.m_ShadowmapIndex]->GetLayout(),
					.depthAttachmentLayout = m_PointShadowDepthBuffer->GetLayout(),

					.extent = m_PointShadowMaps[light.m_ShadowmapIndex]->m_Size,

					.cullMode = VK_CULL_MODE_FRONT_BIT
				};

				m_PointShadowPass->Begin(m_Frames[m_FrameIndex].commandBuffer, renderInfo);

				m_PointShadowPass->BindDescriptorSet(m_Scene->m_LightingDescriptorSet->GetHandle());

				for (const auto& [name, sceneObject] : m_Scene->m_SceneObjects)
				{
					if (!sceneObject->m_Active || !sceneObject->m_Mesh->m_Active) continue;

					uint32_t pushConstant[3]{ sceneObject->GetMatrixIndex(), light.m_ShadowmapIndex, cubeSide };

					m_PointShadowPass->PushConstants(
						pushConstant,
						sizeof(uint32_t) * 3,
						0U,
						VK_SHADER_STAGE_VERTEX_BIT
					);

					for (const auto& subMesh : sceneObject->m_Mesh->m_SubMeshes)
					{
						if (!subMesh.m_Active) continue;

						m_PointShadowPass->BindVertexBuffer(subMesh.m_VertexBuffer);
						m_PointShadowPass->BindIndexBuffer(subMesh.m_IndexBuffer);

						m_PointShadowPass->DrawIndexed(subMesh.m_IndexCount);
					}
				}

				m_PointShadowPass->End();
			}

			m_PointShadowMaps[light.m_ShadowmapIndex]->ChangeLayout(
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				m_Frames[m_FrameIndex].commandBuffer
			);
		}
		
		for (const auto& i : m_Scene->m_ActiveSpotLightsShadowIDs)
		{
			const auto& light = m_Scene->m_SpotLights[i];

			m_SpotShadowMaps[light.m_ShadowmapIndex]->ChangeLayout(
				VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
				VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				m_Frames[m_FrameIndex].commandBuffer
			);

			GraphicsPass::RenderInfo renderInfo{
				.depthAttachmentView = m_SpotShadowMaps[light.m_ShadowmapIndex]->GetViewHandle(),
				.depthAttachmentLayout = m_SpotShadowMaps[light.m_ShadowmapIndex]->GetLayout(),

				.extent = m_SpotShadowMaps[light.m_ShadowmapIndex]->m_Size,

				.cullMode = VK_CULL_MODE_FRONT_BIT
			};

			m_SpotShadowPass->Begin(m_Frames[m_FrameIndex].commandBuffer, renderInfo);

			m_SpotShadowPass->BindDescriptorSet(m_Scene->m_LightingDescriptorSet->GetHandle());

			for (const auto& [name, sceneObject] : m_Scene->m_SceneObjects)
			{
				if (!sceneObject->m_Active || !sceneObject->m_Mesh->m_Active) continue;

				uint32_t pushConstant[2]{ sceneObject->GetMatrixIndex(), light.m_ShadowmapIndex };

				m_SpotShadowPass->PushConstants(
					pushConstant,
					sizeof(uint32_t) * 2,
					0U,
					VK_SHADER_STAGE_VERTEX_BIT
				);

				for (const auto& subMesh : sceneObject->m_Mesh->m_SubMeshes)
				{
					if (!subMesh.m_Active) continue;

					m_SpotShadowPass->BindVertexBuffer(subMesh.m_VertexBuffer);
					m_SpotShadowPass->BindIndexBuffer(subMesh.m_IndexBuffer);

					m_SpotShadowPass->DrawIndexed(subMesh.m_IndexCount);
				}
			}

			m_SpotShadowPass->End();

			m_SpotShadowMaps[light.m_ShadowmapIndex]->ChangeLayout(
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				m_Frames[m_FrameIndex].commandBuffer
			);
		}
		
		for (const auto& i : m_Scene->m_ActiveDirLightsShadowIDs)
		{
			const auto& light = m_Scene->m_DirectionalLights[i];

			for (uint32_t cascadeIndex = 0U; cascadeIndex < SHADOW_CASCADES; cascadeIndex++)
			{
				m_DirShadowMaps[light.m_ShadowmapIndex * SHADOW_CASCADES + cascadeIndex]->ChangeLayout(
					VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
					VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
					m_Frames[m_FrameIndex].commandBuffer
				);

				GraphicsPass::RenderInfo renderInfo{
					.depthAttachmentView = m_DirShadowMaps[light.m_ShadowmapIndex * SHADOW_CASCADES + cascadeIndex]->GetViewHandle(),
					.depthAttachmentLayout = m_DirShadowMaps[light.m_ShadowmapIndex * SHADOW_CASCADES + cascadeIndex]->GetLayout(),

					.extent = m_DirShadowMaps[light.m_ShadowmapIndex * SHADOW_CASCADES + cascadeIndex]->m_Size,

					.cullMode = VK_CULL_MODE_FRONT_BIT
				};

				m_DirShadowPass->Begin(m_Frames[m_FrameIndex].commandBuffer, renderInfo);

				m_DirShadowPass->BindDescriptorSet(m_Scene->m_LightingDescriptorSet->GetHandle(), 0U);
				m_DirShadowPass->BindDescriptorSet(m_CameraBuffer->GetDescriptorHandle(m_FrameIndex), 1U);

				for (const auto& [name, sceneObject] : m_Scene->m_SceneObjects)
				{
					if (!sceneObject->m_Active || !sceneObject->m_Mesh->m_Active) continue;

					uint32_t pushConstant[3] { sceneObject->GetMatrixIndex(), light.m_ShadowmapIndex, cascadeIndex };

					m_DirShadowPass->PushConstants(
						pushConstant,
						sizeof(uint32_t)*3,
						0U,
						VK_SHADER_STAGE_VERTEX_BIT
					);

					for (const auto& subMesh : sceneObject->m_Mesh->m_SubMeshes)
					{
						if (!subMesh.m_Active) continue;

						m_DirShadowPass->BindVertexBuffer(subMesh.m_VertexBuffer);
						m_DirShadowPass->BindIndexBuffer(subMesh.m_IndexBuffer);

						m_DirShadowPass->DrawIndexed(subMesh.m_IndexCount);
					}
				}

				m_DirShadowPass->End();

				m_DirShadowMaps[light.m_ShadowmapIndex * SHADOW_CASCADES + cascadeIndex]->ChangeLayout(
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
					VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					m_Frames[m_FrameIndex].commandBuffer
				);
			}
		}
	}
	void Renderer::ClusterComputePass()
	{
		if (m_SkipFrame)
			return;
		
		if (m_ClusterFrustumChanged)
		{
			uint32_t sizeX = (uint32_t)std::ceilf((float)m_Swapchain->GetExtent().width / CLUSTERED_TILES_X);
			uint32_t sizeY = (uint32_t)std::ceilf((float)m_Swapchain->GetExtent().height / CLUSTERED_TILES_Y);

			glm::uvec4 screenSize(m_Swapchain->GetExtent().width, m_Swapchain->GetExtent().height, sizeX, sizeY);

			m_ClusterAABBCreationPass->Bind(m_Frames[m_FrameIndex].commandBuffer);
			m_ClusterAABBCreationPass->PushConstants(&screenSize, sizeof(glm::uvec4), 0U, VK_SHADER_STAGE_COMPUTE_BIT);
			m_ClusterAABBCreationPass->BindDescriptorSet(m_ClusterSSBOs.aabbClustersDescriptor, 0U, VK_PIPELINE_BIND_POINT_COMPUTE);
			m_ClusterAABBCreationPass->BindDescriptorSet(m_CameraBuffer->GetDescriptorHandle(m_FrameIndex), 1U, VK_PIPELINE_BIND_POINT_COMPUTE);
			m_ClusterAABBCreationPass->Dispatch(CLUSTERED_TILES_X, CLUSTERED_TILES_Y, CLUSTERED_TILES_Z);

			m_ClusterFrustumChanged = false;
		}
		
		m_ClusterLightCullingPass->Bind(m_Frames[m_FrameIndex].commandBuffer);

		m_ClusterLightCullingPass->BindDescriptorSet(m_ClusterSSBOs.clusterLightCullingDescriptor, 0U, VK_PIPELINE_BIND_POINT_COMPUTE);
		m_ClusterLightCullingPass->BindDescriptorSet(m_CameraBuffer->GetDescriptorHandle(m_FrameIndex), 1U, VK_PIPELINE_BIND_POINT_COMPUTE);
		m_ClusterLightCullingPass->BindDescriptorSet(m_Scene->m_LightsBufferDescriptorSet, 2U, VK_PIPELINE_BIND_POINT_COMPUTE);

		m_ClusterLightCullingPass->Dispatch(1U, 1U, CLUSTERED_BATCHES);
	}
	void Renderer::DepthPass()
	{
		if (m_SkipFrame || !m_Settings.depthPrePass) return;

		GraphicsPass::RenderInfo renderInfo {
			.depthAttachmentView = m_DepthBuffer->GetViewHandle(),
			.depthAttachmentLayout = m_DepthBuffer->GetLayout(),
			.extent = m_DepthBuffer->m_Size
		};

		m_DepthPass->Begin(m_Frames[m_FrameIndex].commandBuffer, renderInfo);

		m_DepthPass->BindDescriptorSet(m_CameraBuffer->GetDescriptorHandle(m_FrameIndex), 0U);
		m_DepthPass->BindDescriptorSet(m_Scene->m_GlobalDescriptorSet, 1U);

		for (const auto& [name, sceneObject] : m_Scene->m_SceneObjects)
		{
			if (!sceneObject->m_Active || !sceneObject->m_Mesh->m_Active) continue;

			m_DepthPass->PushConstants(&sceneObject->GetMatrixIndex(), sizeof(uint32_t), 0U, VK_SHADER_STAGE_VERTEX_BIT);

			for (const auto& subMesh : sceneObject->m_Mesh->m_SubMeshes)
			{
				if (!subMesh.m_Active) continue;

				m_DepthPass->BindVertexBuffer(subMesh.m_VertexBuffer);
				m_DepthPass->BindIndexBuffer(subMesh.m_IndexBuffer);

				m_DepthPass->DrawIndexed(subMesh.m_IndexCount);
			}
		}

		m_DepthPass->End();
		
		if (m_Settings.ambientOcclusionMode == AmbientOcclusionMode::None)
			m_DepthBuffer->ChangeLayout(m_DepthBuffer->GetLayout(),
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				m_Frames[m_FrameIndex].commandBuffer
			);
		else
			m_DepthBuffer->ChangeLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				m_Frames[m_FrameIndex].commandBuffer
			);
	}
	void Renderer::SSAOPass()
	{
		if (m_SkipFrame) return;

		m_SSAOTarget->ChangeLayout(
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			m_Frames[m_FrameIndex].commandBuffer
		);

		m_Settings.ambientOcclusion.screenWidth = m_SSAOTarget->m_Size.width;
		m_Settings.ambientOcclusion.screenHeight = m_SSAOTarget->m_Size.height;

		switch (m_Settings.ambientOcclusionQuality)
		{
		case QualityLevel::Low:
			m_Settings.ambientOcclusion._samples = 16U;
			m_Settings.ambientOcclusion._noiseScale = 4.0f;
			break;
		case QualityLevel::Medium:
			m_Settings.ambientOcclusion._samples = 32U;
			m_Settings.ambientOcclusion._noiseScale = 4.0f;
			break;
		case QualityLevel::High:
			m_Settings.ambientOcclusion._samples = 32U;
			m_Settings.ambientOcclusion._noiseScale = 2.0f;
			break;
		case QualityLevel::Ultra:
			m_Settings.ambientOcclusion._samples = 32U;
			m_Settings.ambientOcclusion._noiseScale = 1.0f;
			break;
		}

		GraphicsPass::RenderInfo renderInfo {
			.colorAttachmentView = m_SSAOTarget->GetViewHandle(),
			.colorAttachmentLayout = m_SSAOTarget->GetLayout(),
			.extent = m_SSAOTarget->m_Size,
			.clearColor = { 1.0f, 1.0f, 1.0f, 1.0f },
			.cullMode = VK_CULL_MODE_FRONT_BIT,
		};

		m_SSAOPass->Begin(m_Frames[m_FrameIndex].commandBuffer, renderInfo);
		if (m_Settings.ambientOcclusionMode != AmbientOcclusionMode::None && m_Settings.depthPrePass)
		{
			m_SSAOPass->PushConstants(&m_Settings.ambientOcclusion, sizeof(m_Settings.ambientOcclusion), 0U, VK_SHADER_STAGE_FRAGMENT_BIT);
			m_SSAOPass->BindDescriptorSet(m_CameraBuffer->GetDescriptorHandle(m_FrameIndex), 0U);
			m_SSAOPass->BindDescriptorSet(m_DepthBufferDescriptor, 1U);
			m_SSAOPass->Draw(3U);
		}
		m_SSAOPass->End();

		m_DepthBuffer->ChangeLayout(
			VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
			VK_ACCESS_SHADER_READ_BIT,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			m_Frames[m_FrameIndex].commandBuffer
		);

		m_SSAOTarget->ChangeLayout(
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			m_Frames[m_FrameIndex].commandBuffer
		);
	}
	void Renderer::ForwardPass()
	{
		if (m_SkipFrame) return;
		
		if(m_Settings.antialiasingMode != AntialiasingMode::None)
			m_AliasedImage->ChangeLayout(
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				m_Frames[m_FrameIndex].commandBuffer
		);

		GraphicsPass::RenderInfo renderInfo{
			.colorAttachmentView = m_Settings.antialiasingMode != AntialiasingMode::None ? m_AliasedImage->GetViewHandle() : m_Swapchain->m_ImageViews[m_Swapchain->m_ImageIndex],
			.depthAttachmentView = m_DepthBuffer->GetViewHandle(),

			.colorAttachmentLayout = m_Settings.antialiasingMode != AntialiasingMode::None ? m_AliasedImage->GetLayout() : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.depthAttachmentLayout = m_DepthBuffer->GetLayout(),

			.extent = m_Settings.antialiasingMode != AntialiasingMode::None ? m_AliasedImage->m_Size : m_Swapchain->GetExtent(),

			.clearColor{
				m_Scene->m_AmbientColor.r, m_Scene->m_AmbientColor.g, m_Scene->m_AmbientColor.b, 1.0f
			},

			.depthLoadOp = m_Settings.depthPrePass ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR,
			.depthStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		};

		m_ForwardPass->Begin(m_Frames[m_FrameIndex].commandBuffer, renderInfo);

			m_ForwardPass->BindDescriptorSet(m_CameraBuffer->GetDescriptorHandle(m_FrameIndex), 0U);
			m_ForwardPass->BindDescriptorSet(m_Scene->m_GlobalDescriptorSet, 1U);
			m_ForwardPass->BindDescriptorSet(m_ShadowMapsDescriptor, 2U);
			m_ForwardPass->BindDescriptorSet(m_ClusterDescriptor, 3U);
			m_ForwardPass->BindDescriptorSet(m_SSAODescriptor, 4U);

			m_ForwardPass->PushConstants(&m_Scene->m_MainCamera->m_Exposure, sizeof(float), sizeof(uint32_t)*2, VK_SHADER_STAGE_FRAGMENT_BIT);

			for (const auto& [name, sceneObject] : m_Scene->m_SceneObjects)
			{
				if(!sceneObject->m_Active || !sceneObject->m_Mesh->m_Active) continue;

				m_ForwardPass->PushConstants(&sceneObject->GetMatrixIndex(), sizeof(uint32_t), 0U, VK_SHADER_STAGE_VERTEX_BIT);

				for (const auto& subMesh : sceneObject->m_Mesh->m_SubMeshes)
				{
					if (!subMesh.m_Active) continue;

					m_ForwardPass->PushConstants(&subMesh.GetMaterialIndex(), sizeof(uint32_t), sizeof(uint32_t), VK_SHADER_STAGE_FRAGMENT_BIT);

					m_ForwardPass->BindVertexBuffer(subMesh.m_VertexBuffer);
					m_ForwardPass->BindIndexBuffer(subMesh.m_IndexBuffer);

					m_ForwardPass->DrawIndexed(subMesh.m_IndexCount);
				}
			}

		m_ForwardPass->End();

	}
	void Renderer::AntialiasingPass()
	{
		if (m_SkipFrame || m_Settings.antialiasingMode == AntialiasingMode::None) return;

		m_AliasedImage->ChangeLayout(
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			m_Frames[m_FrameIndex].commandBuffer
		);

		m_Settings.antialiasing.texelSizeX = 1.0f / m_Swapchain->GetExtent().width;
		m_Settings.antialiasing.texelSizeY = 1.0f / m_Swapchain->GetExtent().height;

		GraphicsPass::RenderInfo renderInfo{
			.colorAttachmentView = m_Swapchain->m_ImageViews[m_Swapchain->m_ImageIndex],
			.colorAttachmentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,

			.extent = m_Swapchain->GetExtent(),

			.cullMode = VK_CULL_MODE_FRONT_BIT,
		};

		m_AntialiasingPass->Begin(m_Frames[m_FrameIndex].commandBuffer, renderInfo);
			m_AntialiasingPass->BindDescriptorSet(m_AntialiasingDescriptor);
			m_AntialiasingPass->PushConstants(&m_Settings.antialiasing, sizeof(m_Settings.antialiasing), 0U, VK_SHADER_STAGE_FRAGMENT_BIT);
			m_AntialiasingPass->Draw(3U);
		m_AntialiasingPass->End();
	}
	void Renderer::ImGuiPass()
	{
		if (m_SkipFrame || m_ImGuiRenderCallback == nullptr) return;
		
		m_ImGuiRenderCallback();

		m_ImGuiContext->Render(m_Frames[m_FrameIndex].commandBuffer, m_Swapchain->m_ImageIndex);
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
		else if (m_ReloadQueued)
			ReloadBackendImpl();
		else if (result != VK_SUCCESS)
			EN_ERROR("Renderer::EndRender() - Failed to present swap chain image!");

		m_FrameIndex = (m_FrameIndex + 1) % FRAMES_IN_FLIGHT;
	}

	void Renderer::SetVSyncEnabled(const bool enabled)
	{
		m_Settings.vSync = enabled;
		m_FramebufferResized = true;
	}
	void Renderer::SetDepthPrepassEnabled(const bool enabled)
	{
		m_Settings.depthPrePass = enabled;
		m_ReloadQueued = true;
	}

	void Renderer::SetShadowCascadesWeight(const float weight)
	{
		m_Settings.cascadeSplitWeight = weight;
		ReloadBackend();
	}
	void Renderer::SetShadowCascadesFarPlane(const float farPlane)
	{
		m_Settings.cascadeFarPlane = farPlane;
		ReloadBackend();
	}

	void Renderer::SetPointShadowResolution(const uint32_t resolution)
	{
		m_Settings.pointLightShadowResolution = resolution;
		ReloadBackend();
	}
	void Renderer::SetSpotShadowResolution(const uint32_t resolution)
	{
		m_Settings.spotLightShadowResolution = resolution;
		ReloadBackend();
	}
	void Renderer::SetDirShadowResolution(const uint32_t resolution)
	{
		m_Settings.dirLightShadowResolution = resolution;
		ReloadBackend();
	}
	void Renderer::SetShadowFormat(const VkFormat format)
	{
		m_Settings.shadowsFormat = format;
		ReloadBackend();
	}

	void Renderer::SetAntialiasingMode(const AntialiasingMode antialiasingMode)
	{
		m_Settings.antialiasingMode = antialiasingMode;
		ReloadBackend();
	}
	void Renderer::SetAmbientOcclusionMode(const AmbientOcclusionMode aoMode)
	{
		m_Settings.ambientOcclusionMode = aoMode;
		ReloadBackend();
	}

	//void Renderer::SetAntialaliasingQuality(const QualityLevel quality)
	//{
	//	m_Settings.antialiasingQuality = quality;
	//	ReloadBackend();
	//}
	void Renderer::SetAmbientOcclusionQuality(const QualityLevel quality)
	{
		m_Settings.ambientOcclusionQuality = quality;
		ReloadBackend();
	}

	void Renderer::UpdateCSM()
	{
		// CSM Implementation heavily inspired with:
		// Blaze engine by kidrigger - https://github.com/kidrigger/Blaze
		// Flex Engine by ajweeks - https://github.com/ajweeks/FlexEngine


		float ratio = m_Settings.cascadeFarPlane / m_Scene->m_MainCamera->m_NearPlane;

		for (int i = 1; i < SHADOW_CASCADES; i++)
		{
			float si = i / float(SHADOW_CASCADES);

			float nearPlane = m_Settings.cascadeSplitWeight * (
				m_Scene->m_MainCamera->m_NearPlane * powf(ratio, si)) + 
				(1.0f - m_Settings.cascadeSplitWeight) * (m_Scene->m_MainCamera->m_NearPlane + 
				(m_Settings.cascadeFarPlane - m_Scene->m_MainCamera->m_NearPlane) * si
			);

			float farPlane = nearPlane * 1.005f;
			m_CSM.cascadeSplitDistances[i - 1] = farPlane;
		}

		m_CSM.cascadeSplitDistances[SHADOW_CASCADES - 1] = m_Settings.cascadeFarPlane;

		glm::vec4 frustumClipSpace[8]
		{
			{-1.0f, -1.0f, -1.0f, 1.0f},
			{-1.0f,  1.0f, -1.0f, 1.0f},
			{ 1.0f, -1.0f, -1.0f, 1.0f},
			{ 1.0f,  1.0f, -1.0f, 1.0f},
			{-1.0f, -1.0f,  1.0f, 1.0f},
			{-1.0f,  1.0f,  1.0f, 1.0f},
			{ 1.0f, -1.0f,  1.0f, 1.0f},
			{ 1.0f,  1.0f,  1.0f, 1.0f},
		};

		glm::mat4 invViewProj = glm::inverse(m_Scene->m_MainCamera->GetProjMatrix() * m_Scene->m_MainCamera->GetViewMatrix());

		for (auto& vert : frustumClipSpace)
		{
			vert = invViewProj * vert;
			vert /= vert.w;
		}

		float cosine = glm::dot(glm::normalize(glm::vec3(frustumClipSpace[4]) - m_Scene->m_MainCamera->m_Position), m_Scene->m_MainCamera->GetFront());
		glm::vec3 cornerRay = glm::normalize(glm::vec3(frustumClipSpace[4] - frustumClipSpace[0]));

		float prevFarPlane = m_Scene->m_MainCamera->m_NearPlane;

		for (int i = 0; i < SHADOW_CASCADES; ++i)
		{
			float farPlane = m_CSM.cascadeSplitDistances[i];
			float secTheta = 1.0f / cosine;
			float cDist = 0.5f * (farPlane + prevFarPlane) * secTheta * secTheta;
			m_CSM.cascadeCenters[i] = m_Scene->m_MainCamera->GetFront() * cDist + m_Scene->m_MainCamera->m_Position;

			float nearRatio = prevFarPlane / m_Settings.cascadeFarPlane;
			glm::vec3 corner = cornerRay * nearRatio + m_Scene->m_MainCamera->m_Position;

			m_CSM.cascadeRadiuses[i] = glm::distance(m_CSM.cascadeCenters[i], corner);

			prevFarPlane = farPlane;
		}

		m_CSM.cascadeFrustumSizeRatios[0] = 1.0f;
		for (int i = 1; i < SHADOW_CASCADES; i++)
			m_CSM.cascadeFrustumSizeRatios[i] = m_CSM.cascadeRadiuses[i] / m_CSM.cascadeRadiuses[0];

		for (const auto& i : m_Scene->m_ActiveDirLightsShadowIDs)
		{
			const DirectionalLight& light = m_Scene->m_DirectionalLights[i];

			for (uint32_t j = 0U; j < SHADOW_CASCADES; ++j)
			{
				float radius = m_CSM.cascadeRadiuses[j];
				glm::vec3 center = m_CSM.cascadeCenters[j];

				glm::mat4 lightProj = glm::ortho(-radius, radius, -radius, radius, 0.0f, light.m_FarPlane * radius);
				glm::mat4 lightView = glm::lookAt(center - (light.m_FarPlane - 1.0f) * -glm::normalize(light.m_Direction + glm::vec3(0.00000127f)) * radius, center, glm::vec3(0, 1, 0));

				glm::mat4 shadowViewProj = lightProj * lightView;
				glm::vec4 shadowOrigin = shadowViewProj * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f) * float(m_Settings.dirLightShadowResolution) / 2.0f;

				glm::vec4 shadowOffset = (glm::round(shadowOrigin) - shadowOrigin) * 2.0f / float(m_Settings.dirLightShadowResolution) * glm::vec4(1, 1, 0, 0);

				glm::mat4 shadowProj = lightProj;
				shadowProj[3] += shadowOffset;

				m_CSM.cascadeMatrices[i][j] = shadowProj * lightView;
			}
		}
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
		m_Swapchain = MakeHandle<Swapchain>(m_Settings.vSync);

		CreateDepthBuffer();
		CreateAATarget();
		CreateSSAOTarget();

		m_ImGuiContext->UpdateFramebuffers(m_Swapchain->GetExtent(), m_Swapchain->m_ImageViews);

		m_FramebufferResized = false;

		vkDeviceWaitIdle(g_Ctx->m_LogicalDevice);

		EN_LOG("Resized to (" + std::to_string(m_Swapchain->GetExtent().width) + ", " + std::to_string(m_Swapchain->GetExtent().height) + ")");
	}
	void Renderer::ReloadBackend()
	{
		m_ReloadQueued = true;
	}
	void Renderer::ReloadBackendImpl()
	{
		ResetAllFrames();
		
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
		m_Swapchain = MakeHandle<Swapchain>(m_Settings.vSync);

		EN_SUCCESS("Created swapchain!")

			m_CameraBuffer = MakeHandle<CameraBuffer>();

		EN_SUCCESS("Created the camera buffer!")

			CreatePerFrameData();

		EN_SUCCESS("Created per frame data!")

			m_FullscreenSampler = MakeHandle<Sampler>(VK_FILTER_LINEAR);

		EN_SUCCESS("Created fullscreen sampler!")

			CreateShadowResources();

		EN_SUCCESS("Created shadow resources!")

			CreateShadowPasses();

		EN_SUCCESS("Created shadow passes!")

			CreateDepthBuffer();

		EN_SUCCESS("Created depth buffer!")

			CreateAATarget();

		EN_SUCCESS("Created the antialiasing target!")

			CreateSSAOTarget();

		EN_SUCCESS("Created the SSAO target!")

			CreateDepthPass();

		EN_SUCCESS("Created the depth pass!")

			CreateClusterBuffers();

		EN_SUCCESS("Created the cluster buffers!")

			CreateClusterPasses();

		EN_SUCCESS("Created the cluster pass!")

			CreateSSAOPass();

		EN_SUCCESS("Created the SSAO pass!")

			CreateForwardPass();

		EN_SUCCESS("Created the forward pass!")

			CreateAntialiasingPass();

		EN_SUCCESS("Created the antialiasing pass!")

			if (newImGui)
				m_ImGuiContext = MakeHandle<ImGuiContext>(m_Swapchain->GetFormat(), m_Swapchain->GetExtent(), m_Swapchain->m_ImageViews);
			else
				m_ImGuiContext->UpdateFramebuffers(m_Swapchain->GetExtent(), m_Swapchain->m_ImageViews);

		EN_SUCCESS("Created the ImGui context!")
		
		EN_SUCCESS("Created the renderer Vulkan backend");

		vkDeviceWaitIdle(g_Ctx->m_LogicalDevice);
	}

	void Renderer::CreateShadowResources()
	{
		m_PointShadowDepthBuffer = MakeHandle<Image>(
			VkExtent2D{
				m_Settings.pointLightShadowResolution,
				m_Settings.pointLightShadowResolution
			},
			m_Settings.shadowsFormat,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			0U,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		);

		m_ShadowSampler = MakeHandle<Sampler>(VK_FILTER_LINEAR);

		for (auto& shadowMap : m_PointShadowMaps)
		{
			shadowMap = MakeHandle<Image>(
				VkExtent2D{
					m_Settings.pointLightShadowResolution,
					m_Settings.pointLightShadowResolution
				},
				m_Settings.shadowsFormat == VK_FORMAT_D32_SFLOAT ? VK_FORMAT_R32_SFLOAT : VK_FORMAT_R16_SFLOAT,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				6U
			);
		}
		for (auto& shadowMap : m_SpotShadowMaps)
		{
			shadowMap = MakeHandle<Image>(
				VkExtent2D{
					m_Settings.spotLightShadowResolution,
					m_Settings.spotLightShadowResolution
				},
				m_Settings.shadowsFormat,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_IMAGE_ASPECT_DEPTH_BIT,
				0U,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
		}
		for (auto& shadowMap : m_DirShadowMaps)
		{
			shadowMap = MakeHandle<Image>(
				VkExtent2D{
					m_Settings.dirLightShadowResolution,
					m_Settings.dirLightShadowResolution
				},
				m_Settings.shadowsFormat,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_IMAGE_ASPECT_DEPTH_BIT,
				0U,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
		}

		std::vector<DescriptorInfo::ImageInfoContent> pointShadowMaps{};
		std::vector<DescriptorInfo::ImageInfoContent> spotShadowMaps{};
		std::vector<DescriptorInfo::ImageInfoContent> dirShadowMaps{};

		for (const auto& shadowMap : m_PointShadowMaps)
		{
			pointShadowMaps.emplace_back(DescriptorInfo::ImageInfoContent{
				.imageView = shadowMap->GetViewHandle(),
				.imageSampler = m_ShadowSampler->GetHandle()
			});
		}
		for (const auto& shadowMap : m_SpotShadowMaps)
		{
			spotShadowMaps.emplace_back(DescriptorInfo::ImageInfoContent{
				.imageView = shadowMap->GetViewHandle(),
				.imageSampler = m_ShadowSampler->GetHandle()
			});
		}
		for (const auto& shadowMap : m_DirShadowMaps)
		{
			dirShadowMaps.emplace_back(DescriptorInfo::ImageInfoContent{
				.imageView = shadowMap->GetViewHandle(),
				.imageSampler = m_ShadowSampler->GetHandle()
			});
		}

		m_ShadowMapsDescriptor = MakeHandle<DescriptorSet>(DescriptorInfo{
			std::vector<DescriptorInfo::ImageInfo>{
			DescriptorInfo::ImageInfo {
					.index = 0U,
					.count = static_cast<uint32_t>(pointShadowMaps.size()),

					.contents = pointShadowMaps,

					.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
					.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				},
				DescriptorInfo::ImageInfo {
					.index = 1U,
					.count = static_cast<uint32_t>(spotShadowMaps.size()),

					.contents = spotShadowMaps,

					.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
					.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				},
				DescriptorInfo::ImageInfo {
					.index = 2U,
					.count = static_cast<uint32_t>(dirShadowMaps.size()),

					.contents = dirShadowMaps,

					.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
					.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				},
			}
			});

	}

	void Renderer::CreateShadowPasses()
	{
		constexpr VkPushConstantRange model_lightIndex_cascadeIndex{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 0U,
			.size = sizeof(uint32_t) * 3,
		};

		GraphicsPass::CreateInfo dirInfo{
			.vShader = "Shaders/DirShadowVert.spv",
			//.fShader = "Shaders/ShadowFrag.spv",

			.descriptorLayouts  {Scene::GetLightingDescriptorLayout(), CameraBuffer::GetLayout()},
			.pushConstantRanges {model_lightIndex_cascadeIndex},

			.depthFormat = m_Settings.shadowsFormat,

			.useVertexBindings = true,
			.enableDepthTest = true,
			.enableDepthWrite = true,
			.blendEnable = false,

			.compareOp = VK_COMPARE_OP_LESS,
			.polygonMode = VK_POLYGON_MODE_FILL,
		};

		m_DirShadowPass = MakeHandle<GraphicsPass>(dirInfo);

		constexpr VkPushConstantRange model_lightIndex{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 0U,
			.size = sizeof(uint32_t) * 2,
		};

		GraphicsPass::CreateInfo spotInfo{
			.vShader = "Shaders/SpotShadowVert.spv",
			//.fShader = "Shaders/ShadowFrag.spv",

			.descriptorLayouts  {Scene::GetLightingDescriptorLayout()},
			.pushConstantRanges {model_lightIndex},

			.depthFormat = m_Settings.shadowsFormat,

			.useVertexBindings = true,
			.enableDepthTest = true,
			.enableDepthWrite = true,
			.blendEnable = false,

			.compareOp = VK_COMPARE_OP_LESS,
			.polygonMode = VK_POLYGON_MODE_FILL,
		};

		m_SpotShadowPass = MakeHandle<GraphicsPass>(spotInfo);

		constexpr VkPushConstantRange model_lightIndex_shadowmapIndex{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 0U,
			.size = sizeof(uint32_t) * 3,
		};

		GraphicsPass::CreateInfo pointInfo{
			.vShader = "Shaders/PointShadowVert.spv",
			.fShader = "Shaders/ShadowFrag.spv",

			.descriptorLayouts  {Scene::GetLightingDescriptorLayout()},
			.pushConstantRanges {model_lightIndex_shadowmapIndex},

			.colorFormat = m_Settings.shadowsFormat == VK_FORMAT_D32_SFLOAT ? VK_FORMAT_R32_SFLOAT : VK_FORMAT_R16_SFLOAT,
			.depthFormat = m_Settings.shadowsFormat,

			.useVertexBindings = true,
			.enableDepthTest = true,
			.enableDepthWrite = true,
			.blendEnable = false,

			.compareOp = VK_COMPARE_OP_LESS,
			.polygonMode = VK_POLYGON_MODE_FILL,
		};

		m_PointShadowPass = MakeHandle<GraphicsPass>(pointInfo);
	}
	void Renderer::CreateSSAOPass()
	{
		constexpr VkPushConstantRange ao {
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.offset = 0U,
			.size = sizeof(m_Settings.ambientOcclusion),
		};

		GraphicsPass::CreateInfo info{
			.vShader = "Shaders/FullscreenTri.spv",
			.fShader = "Shaders/SSAO.spv",

			.descriptorLayouts {CameraBuffer::GetLayout(), m_DepthBufferDescriptor->GetLayout()},
			.pushConstantRanges {ao},

			.colorFormat = m_SSAOTarget->m_Format,
		};

		m_SSAOPass = MakeHandle<GraphicsPass>(info);
	}
	void Renderer::CreateDepthPass()
	{
		constexpr VkPushConstantRange model{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 0U,
			.size = sizeof(uint32_t),
		};

		GraphicsPass::CreateInfo info{
			.vShader = "Shaders/Depth.spv",

			.descriptorLayouts {CameraBuffer::GetLayout(), Scene::GetGlobalDescriptorLayout()},
			.pushConstantRanges {model},

			.depthFormat = m_DepthBuffer->m_Format,

			.useVertexBindings = true,
			.enableDepthTest = true,
			.enableDepthWrite = true,
			.blendEnable = false,

			.compareOp = VK_COMPARE_OP_LESS,
			.polygonMode = VK_POLYGON_MODE_FILL,
		};

		m_DepthPass = MakeHandle<GraphicsPass>(info);
	}
	void Renderer::CreateForwardPass()
	{
		m_ClusterDescriptor = MakeHandle<DescriptorSet>(DescriptorInfo{
			std::vector<DescriptorInfo::ImageInfo>{},
			std::vector<DescriptorInfo::BufferInfo>{
				DescriptorInfo::BufferInfo {
					.index = 0U,
					.buffer = m_ClusterSSBOs.pointLightIndices->GetHandle(),
					.size = m_ClusterSSBOs.pointLightIndices->GetSize(),

					.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				},
				DescriptorInfo::BufferInfo {
					.index = 1U,
					.buffer = m_ClusterSSBOs.pointLightGrid->GetHandle(),
					.size = m_ClusterSSBOs.pointLightGrid->GetSize(),

					.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				},
				DescriptorInfo::BufferInfo {
					.index = 2U,
					.buffer = m_ClusterSSBOs.spotLightIndices->GetHandle(),
					.size = m_ClusterSSBOs.spotLightIndices->GetSize(),

					.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				},
				DescriptorInfo::BufferInfo {
					.index = 3U,
					.buffer = m_ClusterSSBOs.spotLightGrid->GetHandle(),
					.size = m_ClusterSSBOs.spotLightGrid->GetSize(),

					.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				},
			},
		});

		constexpr VkPushConstantRange model {
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset		= 0U,
			.size		= sizeof(uint32_t),
		};
		constexpr VkPushConstantRange material_postprocessing {
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.offset		= sizeof(uint32_t),
			.size		= sizeof(uint32_t)+sizeof(float),
		};

		GraphicsPass::CreateInfo info{
			.vShader = "Shaders/ForwardVert.spv",
			.fShader = "Shaders/ForwardFrag.spv",

			.descriptorLayouts {
				CameraBuffer::GetLayout(),
				Scene::GetGlobalDescriptorLayout(),
				m_ShadowMapsDescriptor->GetLayout(),
				m_ClusterDescriptor->GetLayout(),
				m_SSAODescriptor->GetLayout(),
			},
			.pushConstantRanges {model, material_postprocessing},

			.colorFormat = m_Settings.antialiasingMode == AntialiasingMode::None ? m_Swapchain->GetFormat() : m_AliasedImage->m_Format,
			.depthFormat = m_DepthBuffer->m_Format,

			.useVertexBindings = true,
			.enableDepthTest   = true,
			.enableDepthWrite  = !m_Settings.depthPrePass,
			.blendEnable	   = false,

			.compareOp	 = m_Settings.depthPrePass ? VK_COMPARE_OP_LESS_OR_EQUAL : VK_COMPARE_OP_LESS,
			.polygonMode = VK_POLYGON_MODE_FILL,
		};

		m_ForwardPass = MakeHandle<GraphicsPass>(info);
	}
	void Renderer::CreateAntialiasingPass()
	{
		constexpr VkPushConstantRange antialiasing{
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.offset = 0U,
			.size = sizeof(AntialiasingProperties),
		};

		GraphicsPass::CreateInfo info{
			.vShader = "Shaders/FullscreenTri.spv",
			.fShader = "Shaders/FXAA.spv",

			.descriptorLayouts {m_AntialiasingDescriptor->GetLayout()},
			.pushConstantRanges {antialiasing},

			.colorFormat = m_Swapchain->GetFormat(),
		};

		m_AntialiasingPass = MakeHandle<GraphicsPass>(info);
	}

	void Renderer::CreateClusterBuffers()
	{
		uint32_t clusterCount = CLUSTERED_TILES_X * CLUSTERED_TILES_Y * CLUSTERED_TILES_Z;

		m_ClusterSSBOs.aabbClusters = MakeHandle<MemoryBuffer>(
			sizeof(glm::vec4) * 2U * clusterCount,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);

		m_ClusterSSBOs.pointLightIndices = MakeHandle<MemoryBuffer>(
			sizeof(uint32_t) * clusterCount * MAX_POINT_LIGHTS,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);

		m_ClusterSSBOs.pointLightGrid = MakeHandle<MemoryBuffer>(
			sizeof(uint32_t) * 2U * clusterCount,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);

		m_ClusterSSBOs.pointLightGlobalIndexOffset = MakeHandle<MemoryBuffer>(
			sizeof(uint32_t),
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);

		m_ClusterSSBOs.spotLightIndices = MakeHandle<MemoryBuffer>(
			sizeof(uint32_t) * clusterCount * MAX_SPOT_LIGHTS,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);

		m_ClusterSSBOs.spotLightGrid = MakeHandle<MemoryBuffer>(
			sizeof(uint32_t) * 2U * clusterCount,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);

		m_ClusterSSBOs.spotLightGlobalIndexOffset = MakeHandle<MemoryBuffer>(
			sizeof(uint32_t),
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);
	}
	void Renderer::CreateClusterPasses()
	{
		DescriptorInfo::BufferInfo aabbBufferInfo{
			.index = 0U,
			.buffer = m_ClusterSSBOs.aabbClusters->GetHandle(),
			.size = m_ClusterSSBOs.aabbClusters->GetSize(),
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT
		};

		m_ClusterSSBOs.aabbClustersDescriptor = MakeHandle<DescriptorSet>(DescriptorInfo{
			std::vector<DescriptorInfo::ImageInfo>{},
			std::vector<DescriptorInfo::BufferInfo>{
				aabbBufferInfo
			}}
		);

		DescriptorInfo::BufferInfo pointLightGridBufferInfo{
			.index = 1U,
			.buffer = m_ClusterSSBOs.pointLightGrid->GetHandle(),
			.size = m_ClusterSSBOs.pointLightGrid->GetSize(),
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		};

		DescriptorInfo::BufferInfo pointLightIndicesBufferInfo{
			.index = 2U,
			.buffer = m_ClusterSSBOs.pointLightIndices->GetHandle(),
			.size = m_ClusterSSBOs.pointLightIndices->GetSize(),
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT
		};

		DescriptorInfo::BufferInfo pointLightGlobalIndexOffsetBuffer{
			.index = 3U,
			.buffer = m_ClusterSSBOs.pointLightGlobalIndexOffset->GetHandle(),
			.size = m_ClusterSSBOs.pointLightGlobalIndexOffset->GetSize(),
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT
		};

		DescriptorInfo::BufferInfo spotLightGridBufferInfo{
			.index = 4U,
			.buffer = m_ClusterSSBOs.spotLightGrid->GetHandle(),
			.size = m_ClusterSSBOs.spotLightGrid->GetSize(),
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		};

		DescriptorInfo::BufferInfo spotLightIndicesBufferInfo{
			.index = 5U,
			.buffer = m_ClusterSSBOs.spotLightIndices->GetHandle(),
			.size = m_ClusterSSBOs.spotLightIndices->GetSize(),
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT
		};

		DescriptorInfo::BufferInfo spotLightGlobalIndexOffsetBuffer{
			.index = 6U,
			.buffer = m_ClusterSSBOs.spotLightGlobalIndexOffset->GetHandle(),
			.size = m_ClusterSSBOs.spotLightGlobalIndexOffset->GetSize(),
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT
		};

		m_ClusterSSBOs.clusterLightCullingDescriptor = MakeHandle<DescriptorSet>(DescriptorInfo{
			std::vector<DescriptorInfo::ImageInfo>{},
			std::vector<DescriptorInfo::BufferInfo>{
				aabbBufferInfo,
				pointLightGridBufferInfo,
				pointLightIndicesBufferInfo,
				pointLightGlobalIndexOffsetBuffer,
				spotLightGridBufferInfo,
				spotLightIndicesBufferInfo,
				spotLightGlobalIndexOffsetBuffer,
			}}
		);

		constexpr VkPushConstantRange stvPushConstant{
			.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
			.offset = 0U,
			.size = sizeof(glm::uvec4)
		};

		ComputePass::CreateInfo aabbInfo{
			.sourcePath = "Shaders/ClusterAABB.spv",
			.descriptorLayouts = { m_ClusterSSBOs.aabbClustersDescriptor->GetLayout(), CameraBuffer::GetLayout() },
			.pushConstantRanges = { stvPushConstant },
		};

		m_ClusterAABBCreationPass = MakeHandle<ComputePass>(aabbInfo);

		ComputePass::CreateInfo lightCullInfo{
			.sourcePath = "Shaders/ClusterLightCulling.spv",
			.descriptorLayouts = {
				m_ClusterSSBOs.clusterLightCullingDescriptor->GetLayout(),
				CameraBuffer::GetLayout(),
				Scene::GetLightsBufferDescriptorLayout()
			},
		};

		m_ClusterLightCullingPass = MakeHandle<ComputePass>(lightCullInfo);
	}

	void Renderer::CreateAATarget()
	{
		m_AliasedImage = MakeHandle<Image>(
			m_Swapchain->GetExtent(),
			m_Swapchain->GetFormat(),
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			0U,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);

		DescriptorInfo info {
			std::vector<DescriptorInfo::ImageInfo>{
				DescriptorInfo::ImageInfo {
					.contents {{
						.imageView = m_AliasedImage->GetViewHandle(),
						.imageSampler = m_FullscreenSampler->GetHandle()
					}}
				}
			},
			std::vector<DescriptorInfo::BufferInfo>{},
		};

		if (m_AntialiasingDescriptor)
			m_AntialiasingDescriptor->Update(info);
		else
			m_AntialiasingDescriptor = MakeHandle<DescriptorSet>(info);
	}
	void Renderer::CreateSSAOTarget()
	{
		m_SSAOTarget = MakeHandle<Image>(
			m_Swapchain->GetExtent(),
			VK_FORMAT_R8_UNORM,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT, 
			0U,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);

		DescriptorInfo info{
			std::vector<DescriptorInfo::ImageInfo>{
				DescriptorInfo::ImageInfo {
					.contents {{
						.imageView = m_SSAOTarget->GetViewHandle(),
						.imageSampler = m_FullscreenSampler->GetHandle()
					}},
				}
			},
			std::vector<DescriptorInfo::BufferInfo>{},
		};

		if (m_SSAODescriptor)
			m_SSAODescriptor->Update(info);
		else
			m_SSAODescriptor = MakeHandle<DescriptorSet>(info);
	}
	void Renderer::CreateDepthBuffer()
	{
		m_DepthBuffer = MakeHandle<Image>(
			m_Swapchain->GetExtent(),
			VK_FORMAT_D32_SFLOAT,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			0U,
			VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
		);

		DescriptorInfo info{
			std::vector<DescriptorInfo::ImageInfo>{
				DescriptorInfo::ImageInfo {
					.contents {{
						.imageView = m_DepthBuffer->GetViewHandle(),
						.imageSampler = m_FullscreenSampler->GetHandle()
					}}
				}
			},
			std::vector<DescriptorInfo::BufferInfo>{},
		};

		if (m_DepthBufferDescriptor)
			m_DepthBufferDescriptor->Update(info);
		else
			m_DepthBufferDescriptor = MakeHandle<DescriptorSet>(info);
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
				EN_ERROR("GraphicsPass::CreatePerFrameData - Failed to create a main semaphore!");

			if (vkCreateSemaphore(g_Ctx->m_LogicalDevice, &semaphoreInfo, nullptr, &frame.presentSemaphore) != VK_SUCCESS)
				EN_ERROR("GraphicsPass::CreatePerFrameData - Failed to create a present semaphore!");
		
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
}