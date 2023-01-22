#include "Renderer.hpp"

#include <Common/Helpers.hpp>

namespace en
{
	Handle<Camera> g_DefaultCamera;
	Renderer* g_CurrentBackend;

	Renderer::Renderer()
	{
		m_Ctx = &Context::Get();

		g_CurrentBackend = this;

		if (!g_DefaultCamera)
			g_DefaultCamera = MakeHandle<Camera>();

		m_MainCamera = g_DefaultCamera;

		vkDeviceWaitIdle(m_Ctx->m_LogicalDevice);

		Window::Get().SetResizeCallback(Renderer::FramebufferResizeCallback);

		EN_SUCCESS("Init began!")

			m_Swapchain = MakeHandle<Swapchain>(m_VSync);

		EN_SUCCESS("Created swapchain!")

			m_LightsBuffer = MakeHandle<LightsBuffer>();

		EN_SUCCESS("Created lights buffer!");

		m_CameraBuffer = MakeHandle<CameraBuffer>();

		EN_SUCCESS("Created the camera buffer!")

			m_CSMBuffer = MakeHandle<CSMBuffer>();

		EN_SUCCESS("Created the Cascaded Shadow Maps buffer!")

			CreateClusterSSBOs();

		EN_SUCCESS("Created cluster rendering SSBOs!")

			CreateSSAOBuffer();

		EN_SUCCESS("Created SSAO buffer!");

		CreateDepthBuffer();

		EN_SUCCESS("Created Depth Buffer!")

			CreateHDROffscreen();

		EN_SUCCESS("Created high dynamic range image!")

			CreateSSAOTarget();

		EN_SUCCESS("Created SSAO target image!")

			InitShadows();

		EN_SUCCESS("Initialized the shadows!")

			UpdateForwardInput();

		EN_SUCCESS("Created GBuffer descriptor set!")

			UpdateSwapchainInputs();

		EN_SUCCESS("Created swapchain images descriptor sets!")

			UpdateHDRInput();

		EN_SUCCESS("Created high dynamic range image descriptor set!")

			UpdateSSAOInput();

		EN_SUCCESS("Created SSAO descriptor set!")

			CreateCommandBuffer();

		EN_SUCCESS("Created command buffer!")

			CreateClusterComputePipelines();

		EN_SUCCESS("Created cluster rendering compute pipelines!")

			InitDepthPipeline();

		EN_SUCCESS("Created depth pipeline!")

			InitShadowPipeline();

		EN_SUCCESS("Created shadow pipeline!")

			InitOmniShadowPipeline();

		EN_SUCCESS("Created omnidirectional shadow pipeline!")

			InitForwardClusteredPipeline();

		EN_SUCCESS("Created forward clustered pipeline!")

			InitSSAOPipeline();

		EN_SUCCESS("Created SSAO pipeline!")

			InitTonemappingPipeline();

		EN_SUCCESS("Created tonemapping pipeline!")

			InitAntialiasingPipeline();

		EN_SUCCESS("Created antialiasing pipeline!");

		InitImGui();

		EN_SUCCESS("Created ImGui context!");

		m_Swapchain->CreateSwapchainFramebuffers(m_ImGui.renderPass);

		EN_SUCCESS("Created swapchain framebuffers!")

			CreateSyncObjects();

		EN_SUCCESS("Created sync objects!")

			EN_SUCCESS("Created the renderer Vulkan backend");

		vkDeviceWaitIdle(m_Ctx->m_LogicalDevice);
	}
	Renderer::~Renderer()
	{
		vkDeviceWaitIdle(m_Ctx->m_LogicalDevice);

		DestroyShadows();

		m_ImGui.Destroy();

		for (auto& semaphore : m_MainSemaphores)
			vkDestroySemaphore(m_Ctx->m_LogicalDevice, semaphore, nullptr);

		for (auto& semaphore : m_PresentSemaphores)
			vkDestroySemaphore(m_Ctx->m_LogicalDevice, semaphore, nullptr);
		
		for(auto& fence : m_SubmitFences)
			vkDestroyFence(m_Ctx->m_LogicalDevice, fence, nullptr);

		vkDestroySampler(m_Ctx->m_LogicalDevice, m_MainSampler, nullptr);

		vkFreeCommandBuffers(m_Ctx->m_LogicalDevice, m_Ctx->m_GraphicsCommandPool, 1U, m_CommandBuffers.data());
	}

	void Renderer::Render()
	{
		BeginRender();

		DepthPass();
		SSAOPass();
		ShadowPass();
		ClusteredForwardPass();
		TonemappingPass();
		AntialiasPass();
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
		vkWaitForFences(m_Ctx->m_LogicalDevice, 1U, &m_SubmitFences[m_FrameIndex], VK_TRUE, UINT64_MAX);
	}

	void Renderer::UpdateLights()
	{
		m_ClusterFrustumChanged = false;
		m_LightsBufferChanged = false;

		glm::mat4 oldInvProj = m_CameraBuffer->m_CBOs[m_FrameIndex].invProj;

		m_CameraBuffer->UpdateBuffer(m_FrameIndex, m_MainCamera, m_Swapchain->GetExtent(), m_DebugMode);

		if (oldInvProj != m_CameraBuffer->m_CBOs[m_FrameIndex].invProj)
			m_ClusterFrustumChanged = true;

		auto& pointLights = m_Scene->m_PointLights;
		auto& spotLights = m_Scene->m_SpotLights;
		auto& dirLights = m_Scene->m_DirectionalLights;

		uint32_t oldActivePointLights = m_LightsBuffer->LBO.activePointLights;
		uint32_t oldActiveSpotLights = m_LightsBuffer->LBO.activeSpotLights;
		uint32_t oldActiveDirLights = m_LightsBuffer->LBO.activeDirLights;

		m_LightsBuffer->LBO.activePointLights = 0U;
		m_LightsBuffer->LBO.activeSpotLights = 0U;
		m_LightsBuffer->LBO.activeDirLights = 0U;

		if (m_LightsBuffer->LBO.ambientLight != m_Scene->m_AmbientColor)
			m_LightsBufferChanged = true;

		m_LightsBuffer->LBO.ambientLight = m_Scene->m_AmbientColor;

		for (auto& l : pointLights)
			l.m_ShadowmapIndex = -1;
		for (auto& l : spotLights)
			l.m_ShadowmapIndex = -1;
		for (auto& l : dirLights)
			l.m_ShadowmapIndex = -1;

		// Sorting light casters by distance to use only the closest ones
		
		float maxShadowDist = std::numeric_limits<float>::max();

		int shadowCount = 0;
		for (const auto& l : pointLights)
			shadowCount += l.m_CastShadows;

		// If more than 'MAX_POINT_LIGHT_SHADOWS' pointlights cast shadows
		if (shadowCount > MAX_POINT_LIGHT_SHADOWS)
		{
			std::vector<PointLight*> shadowedLights;
			shadowedLights.reserve(shadowCount);

			// Make a vector out of lights that cast shadows
			for (auto& l : pointLights)
				if (l.m_CastShadows)
					shadowedLights.emplace_back(&l);

			// Sort the lights by the distance to the camera in a growing order
			std::sort(shadowedLights.begin(), shadowedLights.end(),
			[this](const PointLight* a, const PointLight* b) -> bool {
					return glm::distance(m_MainCamera->m_Position, a->m_Position) < glm::distance(m_MainCamera->m_Position, b->m_Position);
			});
				
			// Maximum distance of active shadowcasters is the distance to the 'MAX_POINT_LIGHT_SHADOWS' light from the camera
			maxShadowDist = glm::distance(m_MainCamera->m_Position, shadowedLights[MAX_POINT_LIGHT_SHADOWS]->m_Position);
		}

		for (int i = 0, index = 0; i < pointLights.size() && index < MAX_POINT_LIGHT_SHADOWS; i++)
			if (pointLights[i].m_CastShadows && glm::distance(m_MainCamera->m_Position, pointLights[i].m_Position) < maxShadowDist) 
				pointLights[i].m_ShadowmapIndex = index++; // Set the shadowmap index if the light casts shadows and is closer to the camera than the max distance


		maxShadowDist = std::numeric_limits<float>::max();

		shadowCount = 0;
		for (const auto& l : spotLights)
			shadowCount += l.m_CastShadows;

		// If more than 'MAX_SPOT_LIGHT_SHADOWS' spotlights cast shadows
		if (shadowCount > MAX_SPOT_LIGHT_SHADOWS)
		{
			std::vector<SpotLight*> shadowedLights;
			shadowedLights.reserve(shadowCount);

			// Make a vector out of lights that cast shadows
			for (auto& l : spotLights)
				if (l.m_CastShadows)
					shadowedLights.emplace_back(&l);

			// Sort the lights by the distance to the camera in a growing order
			std::sort(shadowedLights.begin(), shadowedLights.end(),
				[this](const SpotLight* a, const SpotLight* b) -> bool {
					return glm::distance(m_MainCamera->m_Position, a->m_Position) < glm::distance(m_MainCamera->m_Position, b->m_Position);
				});

			// Maximum distance of active shadowcasters is the distance to the 'MAX_SPOT_LIGHT_SHADOWS' light from the camera
			maxShadowDist = glm::distance(m_MainCamera->m_Position, shadowedLights[MAX_SPOT_LIGHT_SHADOWS]->m_Position);
		}

		for (int i = 0, index = 0; i < spotLights.size() && index < MAX_SPOT_LIGHT_SHADOWS; i++)
			if (spotLights[i].m_CastShadows && glm::distance(m_MainCamera->m_Position, spotLights[i].m_Position) < maxShadowDist)
				spotLights[i].m_ShadowmapIndex = index++;

		for (int i = 0, index = 0; i < dirLights.size() && index < MAX_DIR_LIGHT_SHADOWS; i++)
			if(dirLights[i].m_CastShadows)
				dirLights[i].m_ShadowmapIndex = index++ * SHADOW_CASCADES;
		
		UpdateShadowFrustums();

		for (const auto& light : pointLights)
		{
			PointLight::Buffer& buffer = m_LightsBuffer->LBO.pointLights[m_LightsBuffer->LBO.activePointLights];

			glm::vec3 lightCol = light.m_Color * (float)light.m_Active * light.m_Intensity;
			float     lightRad = light.m_Radius * (float)light.m_Active;

			if (lightCol == glm::vec3(0.0) || lightRad <= 0.0f)
				continue;

			if (buffer.position		  != light.m_Position		||
				buffer.color		  != lightCol				||
				buffer.radius		  != lightRad				||
				buffer.shadowmapIndex != light.m_ShadowmapIndex ||
				buffer.shadowSoftness != light.m_ShadowSoftness ||
				buffer.pcfSampleRate  != light.m_PCFSampleRate	||
				buffer.bias			  != light.m_ShadowBias
			) m_LightsBufferChanged = true;

			buffer.position		  = light.m_Position;
			buffer.color		  = lightCol;
			buffer.radius		  = lightRad;
			buffer.shadowmapIndex = light.m_ShadowmapIndex;
			buffer.shadowSoftness = light.m_ShadowSoftness;
			buffer.pcfSampleRate  = light.m_PCFSampleRate;
			buffer.bias			  = light.m_ShadowBias;

			if (light.m_ShadowmapIndex != -1)
			{
				auto& matrices = m_Shadows.point.shadowMatrices;

				const glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, light.m_Radius);
				
				matrices[light.m_ShadowmapIndex][0] = proj * glm::lookAt(light.m_Position, light.m_Position + glm::vec3( 1.0,  0.0,  0.0), glm::vec3(0.0, -1.0, 0.0));
				matrices[light.m_ShadowmapIndex][1] = proj * glm::lookAt(light.m_Position, light.m_Position + glm::vec3(-1.0,  0.0,  0.0), glm::vec3(0.0, -1.0, 0.0));
				matrices[light.m_ShadowmapIndex][2] = proj * glm::lookAt(light.m_Position, light.m_Position + glm::vec3( 0.0,  1.0,  0.0), glm::vec3(0.0, 0.0, 1.0));
				matrices[light.m_ShadowmapIndex][3] = proj * glm::lookAt(light.m_Position, light.m_Position + glm::vec3( 0.0, -1.0,  0.0), glm::vec3(0.0, 0.0, -1.0));
				matrices[light.m_ShadowmapIndex][4] = proj * glm::lookAt(light.m_Position, light.m_Position + glm::vec3( 0.0,  0.0,  1.0), glm::vec3(0.0, -1.0, 0.0));
				matrices[light.m_ShadowmapIndex][5] = proj * glm::lookAt(light.m_Position, light.m_Position + glm::vec3( 0.0,  0.0, -1.0), glm::vec3(0.0, -1.0, 0.0));
			
				m_Shadows.point.lightPositions[light.m_ShadowmapIndex] = light.m_Position;
				m_Shadows.point.farPlanes[light.m_ShadowmapIndex] = light.m_Radius;
			}

			m_LightsBuffer->LBO.activePointLights++;
		}
		for (const auto& light : spotLights)
		{
			SpotLight::Buffer& buffer = m_LightsBuffer->LBO.spotLights[m_LightsBuffer->LBO.activeSpotLights];

			glm::vec3 lightColor = light.m_Color * (float)light.m_Active * light.m_Intensity;

			if (light.m_Range == 0.0f || lightColor == glm::vec3(0.0) || light.m_OuterCutoff == 0.0f)
				continue;

			if (buffer.color		  != lightColor							 ||
				buffer.range		  != light.m_Range						 ||
				buffer.outerCutoff	  != light.m_OuterCutoff				 ||
				buffer.position		  != light.m_Position					 ||
				buffer.innerCutoff	  != light.m_InnerCutoff				 ||
				buffer.direction	  != glm::normalize(light.m_Direction) ||
				buffer.shadowmapIndex != light.m_ShadowmapIndex				 ||
				buffer.shadowSoftness != light.m_ShadowSoftness				 ||
				buffer.pcfSampleRate  != light.m_PCFSampleRate				 ||
				buffer.bias			  != light.m_ShadowBias
			) m_LightsBufferChanged = true;

			buffer.color		  = lightColor;
			buffer.range		  = light.m_Range;
			buffer.outerCutoff	  = light.m_OuterCutoff;
			buffer.position		  = light.m_Position;
			buffer.innerCutoff	  = light.m_InnerCutoff;
			buffer.direction	  = glm::normalize(light.m_Direction);
			buffer.shadowmapIndex = light.m_ShadowmapIndex;
			buffer.shadowSoftness = light.m_ShadowSoftness;
			buffer.pcfSampleRate  = light.m_PCFSampleRate;
			buffer.bias			  = light.m_ShadowBias;

			if (light.m_ShadowmapIndex != -1)
			{
				glm::mat4 view = glm::lookAt(light.m_Position, light.m_Position + light.m_Direction + glm::vec3(0.0000001f, 0, 0.0000001f), glm::vec3(0.0, 1.0, 0.0));
				glm::mat4 proj = glm::perspective((light.m_OuterCutoff + 0.02f) * glm::pi<float>(), 1.0f, 0.01f, light.m_Range);
				buffer.lightMat = proj * view;
			}

			m_LightsBuffer->LBO.activeSpotLights++;
		}
		for (uint32_t i = 0U; const auto& light : dirLights)
		{
			DirectionalLight::Buffer& buffer = m_LightsBuffer->LBO.dirLights[m_LightsBuffer->LBO.activeDirLights];

			glm::vec3 lightCol = light.m_Color * (float)light.m_Active * light.m_Intensity;

			if (lightCol == glm::vec3(0.0f))
				continue;

			if (buffer.color		  != lightCol							 ||
				buffer.shadowmapIndex != light.m_ShadowmapIndex				 ||
				buffer.direction	  != glm::normalize(light.m_Direction) ||
				buffer.shadowSoftness != light.m_ShadowSoftness				 ||
				buffer.pcfSampleRate  != light.m_PCFSampleRate				 ||
				buffer.bias			  != light.m_ShadowBias
			) m_LightsBufferChanged = true;

			buffer.color	      = lightCol;
			buffer.shadowmapIndex = light.m_ShadowmapIndex;
			buffer.direction	  = glm::normalize(light.m_Direction);
			buffer.shadowSoftness = light.m_ShadowSoftness;
			buffer.pcfSampleRate  = light.m_PCFSampleRate;
			buffer.bias			  = light.m_ShadowBias;

			if (light.m_ShadowmapIndex != -1)
				RecalculateShadowMatrices(light, m_CSMBuffer->m_CSMBOs[m_FrameIndex].cascadeLightMatrices[i++]);

			m_LightsBuffer->LBO.activeDirLights++;
		}

		if (oldActivePointLights != m_LightsBuffer->LBO.activePointLights ||
			oldActiveSpotLights  != m_LightsBuffer->LBO.activeSpotLights  ||
			oldActiveDirLights   != m_LightsBuffer->LBO.activeDirLights
		) m_LightsBufferChanged = true;

		// Reset last (point/spot)lights for clustered rendering
		m_LightsBuffer->LBO.pointLights[m_LightsBuffer->LBO.activePointLights].color = glm::vec3(0.0f);
		m_LightsBuffer->LBO.pointLights[m_LightsBuffer->LBO.activePointLights].radius = 0.0f;

		m_LightsBuffer->LBO.spotLights[m_LightsBuffer->LBO.activeSpotLights].color = glm::vec3(0.0f);
		m_LightsBuffer->LBO.spotLights[m_LightsBuffer->LBO.activeSpotLights].range = 0.0f;
		m_LightsBuffer->LBO.spotLights[m_LightsBuffer->LBO.activeSpotLights].direction = glm::vec3(0.0f);
		m_LightsBuffer->LBO.spotLights[m_LightsBuffer->LBO.activeSpotLights].outerCutoff = 0.0f;
	}

	void Renderer::BeginRender()
	{
		m_CameraBuffer->MapBuffer(m_FrameIndex);
		m_CSMBuffer->MapBuffer(m_FrameIndex);

		if(m_LightsBufferChanged)
			m_LightsBuffer->MapStagingMemory(m_FrameIndex);

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
			EN_ERROR("Renderer::BeginRender() - Failed to acquire swap chain image!");

		vkResetFences(m_Ctx->m_LogicalDevice, 1U, &m_SubmitFences[m_FrameIndex]);

		vkResetCommandBuffer(m_CommandBuffers[m_FrameIndex], 0U);

		constexpr VkCommandBufferBeginInfo beginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
		};

		if (vkBeginCommandBuffer(m_CommandBuffers[m_FrameIndex], &beginInfo) != VK_SUCCESS)
			EN_ERROR("Renderer::BeginRender() - Failed to begin recording command buffer!");

		if (m_LightsBufferChanged)
			m_LightsBuffer->CopyStagingToDevice(m_FrameIndex, m_CommandBuffers[m_FrameIndex]);
	}
	void Renderer::DepthPass()
	{
		if (m_SkipFrame || !m_Scene) return;

		const GraphicsPipeline::BindInfo info {
			.depthAttachment {
				.imageView	 = m_DepthBuffer->GetViewHandle(),
				.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
				.loadOp		 = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp	 = VK_ATTACHMENT_STORE_OP_STORE,

				.clearValue {
					.depthStencil = { 1.0f, 0U}
				}
			},

			.extent = m_Swapchain->GetExtent()
		};

		m_DepthPipeline->BeginRendering(m_CommandBuffers[m_FrameIndex], info);

		glm::mat4 cameraMatrix = m_CameraBuffer->m_CBOs[m_FrameIndex].proj * m_CameraBuffer->m_CBOs[m_FrameIndex].view;

		for (const auto& [name, object] : m_Scene->m_SceneObjects)
		{
			if (!object->m_Active || !object->m_Mesh->m_Active) continue;

			const DepthStageInfo cameraInfo{
				.modelMatrix = object->GetModelMatrix(),
				.viewProjMatrix = cameraMatrix
			};

			m_DepthPipeline->PushConstants(&cameraInfo, sizeof(DepthStageInfo), 0U, VK_SHADER_STAGE_VERTEX_BIT);

			for (const auto& subMesh : object->m_Mesh->m_SubMeshes)
			{
				if (!subMesh.m_Active) continue;

				m_DepthPipeline->BindVertexBuffer(subMesh.m_VertexBuffer);
				m_DepthPipeline->BindIndexBuffer(subMesh.m_IndexBuffer);
				m_DepthPipeline->DrawIndexed(subMesh.m_IndexBuffer->m_IndicesCount);
			}
		}

		m_DepthPipeline->EndRendering();
	}
	void Renderer::ShadowPass()
	{
		if (m_SkipFrame || !m_Scene) return;

		Helpers::TransitionImageLayout(
			m_Shadows.point.sharedImage, m_Shadows.shadowFormat, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			0U, 6*MAX_POINT_LIGHT_SHADOWS, 1U,
			m_CommandBuffers[m_FrameIndex]
		);

		Helpers::TransitionImageLayout(
			m_Shadows.spot.sharedImage, m_Shadows.shadowFormat, VK_IMAGE_ASPECT_DEPTH_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			0U, MAX_SPOT_LIGHT_SHADOWS, 1U,
			m_CommandBuffers[m_FrameIndex]
		);

		Helpers::TransitionImageLayout(
			m_Shadows.dir.sharedImage, m_Shadows.shadowFormat, VK_IMAGE_ASPECT_DEPTH_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			0U, MAX_DIR_LIGHT_SHADOWS * SHADOW_CASCADES, 1U,
			m_CommandBuffers[m_FrameIndex]
		);
		for (int i = 0; i < m_LightsBuffer->LBO.activePointLights; i++)
		{
			if (m_LightsBuffer->LBO.pointLights[i].shadowmapIndex == -1)
				continue;

			for (int j = 0; j < 6; j++)
			{
				const GraphicsPipeline::BindInfo info{
					.colorAttachments {
						{
							.imageView = m_Shadows.point.singleViews[m_LightsBuffer->LBO.pointLights[i].shadowmapIndex][j],
							.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,

							.clearValue = { 0.0f, 0.0f, 0.0f, 0.0f },
						}
					},

					.depthAttachment {
						.imageView = m_Shadows.point.depthView,
						.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
						.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,

						.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
						.clearValue {
							.depthStencil = {1.0f, 0U}
						}
					},

					.cullMode = VK_CULL_MODE_FRONT_BIT,

					.extent = VkExtent2D{m_Shadows.point.resolution, m_Shadows.point.resolution},
				};
				
				m_OmniShadowPipeline->BeginRendering(m_CommandBuffers[m_FrameIndex], info);

				for (const auto& [name, object] : m_Scene->m_SceneObjects)
				{
					if (!object->m_Active || !object->m_Mesh->m_Active) continue;

					glm::mat4 model = object->GetModelMatrix();

					auto& pos = m_Shadows.point.lightPositions[m_LightsBuffer->LBO.pointLights[i].shadowmapIndex];
					auto& farPlane = m_Shadows.point.farPlanes[m_LightsBuffer->LBO.pointLights[i].shadowmapIndex];

					// Store light position and far plane in the last row
					model[0][3] = pos.x;
					model[1][3] = pos.y;
					model[2][3] = pos.z;
					model[3][3] = farPlane;

					const Shadows::Point::OmniShadowPushConstant pushConstant {
						.viewProj = m_Shadows.point.shadowMatrices[m_LightsBuffer->LBO.pointLights[i].shadowmapIndex][j],
						.model = model,
					};

					m_OmniShadowPipeline->PushConstants(&pushConstant, sizeof(Shadows::Point::OmniShadowPushConstant), 0U, VK_SHADER_STAGE_VERTEX_BIT);
					
					for (const auto& subMesh : object->m_Mesh->m_SubMeshes)
					{
						if (!subMesh.m_Active) continue;

						m_OmniShadowPipeline->BindVertexBuffer(subMesh.m_VertexBuffer);
						m_OmniShadowPipeline->BindIndexBuffer(subMesh.m_IndexBuffer);
						m_OmniShadowPipeline->DrawIndexed(subMesh.m_IndexBuffer->m_IndicesCount);
					}
				}

				m_OmniShadowPipeline->EndRendering();

			}
		}
		for (int i = 0; i < m_LightsBuffer->LBO.activeSpotLights; i++)
		{
			if (m_LightsBuffer->LBO.spotLights[i].shadowmapIndex == -1)
				continue;

			const GraphicsPipeline::BindInfo info{
				.depthAttachment {
					.imageView = m_Shadows.spot.singleViews[m_LightsBuffer->LBO.spotLights[i].shadowmapIndex],

					.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,

					.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,

					.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
	
					.clearValue {
						.depthStencil = { 1.0f, 0U}
					}
				},

				.cullMode = VK_CULL_MODE_FRONT_BIT,

				.extent = VkExtent2D{m_Shadows.spot.resolution, m_Shadows.spot.resolution},
			};

			m_ShadowPipeline->BeginRendering(m_CommandBuffers[m_FrameIndex], info);

			for (const auto& [name, object] : m_Scene->m_SceneObjects)
			{
				if (!object->m_Active || !object->m_Mesh->m_Active) continue;

				const DepthStageInfo cameraInfo{
					.modelMatrix = object->GetModelMatrix(),
					.viewProjMatrix = m_LightsBuffer->LBO.spotLights[i].lightMat,
				};

				m_ShadowPipeline->PushConstants(&cameraInfo, sizeof(DepthStageInfo), 0U, VK_SHADER_STAGE_VERTEX_BIT);

				for (const auto& subMesh : object->m_Mesh->m_SubMeshes)
				{
					if (!subMesh.m_Active) continue;

					m_ShadowPipeline->BindVertexBuffer(subMesh.m_VertexBuffer);
					m_ShadowPipeline->BindIndexBuffer(subMesh.m_IndexBuffer);
					m_ShadowPipeline->DrawIndexed(subMesh.m_IndexBuffer->m_IndicesCount);
				}
			}

			m_ShadowPipeline->EndRendering();
		}
		for (int i = 0; i < m_LightsBuffer->LBO.activeDirLights; i++)
		{
			if (m_LightsBuffer->LBO.dirLights[i].shadowmapIndex == -1)
				continue;

			for (int j = 0; j < SHADOW_CASCADES; j++)
			{
				const GraphicsPipeline::BindInfo info{
					.depthAttachment {
						.imageView = m_Shadows.dir.singleViews[m_LightsBuffer->LBO.dirLights[i].shadowmapIndex + j],

						.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,

						.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,

						.storeOp = VK_ATTACHMENT_STORE_OP_STORE,

						.clearValue {
							.depthStencil = { 1.0f, 0U}
						}
					},

					.cullMode = VK_CULL_MODE_FRONT_BIT,

					.extent = VkExtent2D{m_Shadows.dir.resolution, m_Shadows.dir.resolution},
				};

				m_ShadowPipeline->BeginRendering(m_CommandBuffers[m_FrameIndex], info);

				for (const auto& [name, object] : m_Scene->m_SceneObjects)
				{
					if (!object->m_Active || !object->m_Mesh->m_Active) continue;

					const DepthStageInfo cameraInfo{
						.modelMatrix = object->GetModelMatrix(),
						.viewProjMatrix = m_CSMBuffer->m_CSMBOs[m_FrameIndex].cascadeLightMatrices[i][j],
					};

					m_ShadowPipeline->PushConstants(&cameraInfo, sizeof(DepthStageInfo), 0U, VK_SHADER_STAGE_VERTEX_BIT);

					for (const auto& subMesh : object->m_Mesh->m_SubMeshes)
					{
						if (!subMesh.m_Active) continue;

						m_ShadowPipeline->BindVertexBuffer(subMesh.m_VertexBuffer);
						m_ShadowPipeline->BindIndexBuffer(subMesh.m_IndexBuffer);
						m_ShadowPipeline->DrawIndexed(subMesh.m_IndexBuffer->m_IndicesCount);
					}
				}

				m_ShadowPipeline->EndRendering();
			}
		}

		Helpers::TransitionImageLayout(m_Shadows.point.sharedImage, m_Shadows.shadowFormat, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0U, 6 * MAX_POINT_LIGHT_SHADOWS, 1U,
			m_CommandBuffers[m_FrameIndex]
		);

		Helpers::TransitionImageLayout(m_Shadows.spot.sharedImage, m_Shadows.shadowFormat, VK_IMAGE_ASPECT_DEPTH_BIT,
			VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0U, MAX_SPOT_LIGHT_SHADOWS, 1U,
			m_CommandBuffers[m_FrameIndex]
		);
		Helpers::TransitionImageLayout(m_Shadows.dir.sharedImage, m_Shadows.shadowFormat, VK_IMAGE_ASPECT_DEPTH_BIT,
			VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0U, MAX_DIR_LIGHT_SHADOWS* SHADOW_CASCADES, 1U,
			m_CommandBuffers[m_FrameIndex]
		);
	}
	void Renderer::ClusteredForwardPass()
	{
		if (m_SkipFrame || !m_Scene)
			return;

		if (m_ClusterFrustumChanged)
		{
			uint32_t sizeX = (uint32_t)std::ceilf((float)m_Swapchain->GetExtent().width / CLUSTERED_TILES_X);
			uint32_t sizeY = (uint32_t)std::ceilf((float)m_Swapchain->GetExtent().height / CLUSTERED_TILES_Y);

			glm::uvec4 screenSize(m_Swapchain->GetExtent().width, m_Swapchain->GetExtent().height, sizeX, sizeY);

			m_ClusterAABBCompute->Bind(m_CommandBuffers[m_FrameIndex]);

			m_ClusterAABBCompute->PushConstants(&screenSize, sizeof(glm::uvec4), 0U, VK_SHADER_STAGE_COMPUTE_BIT);

			m_ClusterAABBCompute->BindDescriptorSet(m_ClusterSSBOs.aabbClustersDescriptor, 0U, VK_PIPELINE_BIND_POINT_COMPUTE);

			m_ClusterAABBCompute->BindDescriptorSet(m_CameraBuffer->GetDescriptorHandle(m_FrameIndex), 1U, VK_PIPELINE_BIND_POINT_COMPUTE);

			m_ClusterAABBCompute->Dispatch(CLUSTERED_TILES_X, CLUSTERED_TILES_Y, CLUSTERED_TILES_Z);

			m_ClusterFrustumChanged = false;
		}

		m_ClusterLightCullingCompute->Bind(m_CommandBuffers[m_FrameIndex]);
		
		m_ClusterLightCullingCompute->BindDescriptorSet(m_ClusterSSBOs.clusterLightCullingDescriptor, 0U, VK_PIPELINE_BIND_POINT_COMPUTE);
		
		m_ClusterLightCullingCompute->BindDescriptorSet(m_CameraBuffer->GetDescriptorHandle(m_FrameIndex), 1U, VK_PIPELINE_BIND_POINT_COMPUTE);
		
		m_ClusterLightCullingCompute->Dispatch(1U, 1U, CLUSTERED_BATCHES);

		m_HDROffscreen->ChangeLayout(
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			m_CommandBuffers[m_FrameIndex]
		);

		const GraphicsPipeline::BindInfo info {
			.colorAttachments {
				{
					.imageView   = m_HDROffscreen->GetViewHandle(),
					.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.loadOp		 = VK_ATTACHMENT_LOAD_OP_CLEAR,
					.storeOp	 = VK_ATTACHMENT_STORE_OP_STORE,	

					.clearValue {
						.color = {m_Scene->m_AmbientColor.x, m_Scene->m_AmbientColor.y, m_Scene->m_AmbientColor.z, 1.0},
					}
				}
			},

			.depthAttachment {
				.imageView   = m_DepthBuffer->GetViewHandle(),
				.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
				.loadOp		 = VK_ATTACHMENT_LOAD_OP_LOAD,
				.storeOp	 = VK_ATTACHMENT_STORE_OP_STORE,
			},

			.extent = m_Swapchain->GetExtent(),
		};

		m_ForwardClusteredPipeline->BeginRendering(m_CommandBuffers[m_FrameIndex], info);
		
		m_ForwardClusteredPipeline->BindDescriptorSet(m_CameraBuffer->GetDescriptorHandle(m_FrameIndex), 0U);

		m_ForwardClusteredPipeline->BindDescriptorSet(m_ForwardClusteredDescriptor->GetHandle(), 2U);

		m_ForwardClusteredPipeline->BindDescriptorSet(m_CSMBuffer->GetDescriptorHandle(m_FrameIndex), 3U);

		for (const auto& [name, object] : m_Scene->m_SceneObjects)
		{
			if (!object->m_Active || !object->m_Mesh->m_Active) continue;
		
			m_ForwardClusteredPipeline->PushConstants(&object->GetModelMatrix(), sizeof(glm::mat4), 0U, VK_SHADER_STAGE_VERTEX_BIT);
		
			for (auto& subMesh : object->m_Mesh->m_SubMeshes)
			{
				if (!subMesh.m_Active) continue;
			
				m_ForwardClusteredPipeline->PushConstants(subMesh.m_Material->GetMatBufferPtr(), 24U, sizeof(glm::mat4), VK_SHADER_STAGE_FRAGMENT_BIT);
				m_ForwardClusteredPipeline->BindDescriptorSet(subMesh.m_Material->GetDescriptorSet(), 1U);
			
				m_ForwardClusteredPipeline->BindVertexBuffer(subMesh.m_VertexBuffer);
				m_ForwardClusteredPipeline->BindIndexBuffer(subMesh.m_IndexBuffer);
				m_ForwardClusteredPipeline->DrawIndexed(subMesh.m_IndexBuffer->m_IndicesCount);
			}
		}
		
		m_ForwardClusteredPipeline->EndRendering();
	}
	void Renderer::SSAOPass()
	{
		if (m_SkipFrame || !m_Scene || m_PostProcessParams.ambientOcclusionMode == AmbientOcclusionMode::None)
			return;
			
		m_DepthBuffer->ChangeLayout(
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			m_CommandBuffers[m_FrameIndex]
		);

		m_SSAOTarget->ChangeLayout(
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			m_CommandBuffers[m_FrameIndex]
		);
		
		const GraphicsPipeline::BindInfo info{
			.colorAttachments {
				{
					.imageView   = m_SSAOTarget->GetViewHandle(),
					.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.loadOp		 = VK_ATTACHMENT_LOAD_OP_CLEAR,
					.storeOp	 = VK_ATTACHMENT_STORE_OP_STORE,
				}
			},
		
			.cullMode = VK_CULL_MODE_FRONT_BIT,
		
			.extent = m_Swapchain->GetExtent()
		};
		
		m_SSAOPipeline->BeginRendering(m_CommandBuffers[m_FrameIndex], info);
		
		m_PostProcessParams.ambientOcclusion.screenWidth = static_cast<float>(m_Swapchain->GetExtent().width);
		m_PostProcessParams.ambientOcclusion.screenHeight = static_cast<float>(m_Swapchain->GetExtent().height);
		
		m_SSAOPipeline->PushConstants(&m_PostProcessParams.ambientOcclusion, sizeof(PostProcessingParams::AmbientOcclusion), 0U, VK_SHADER_STAGE_FRAGMENT_BIT);
		
		m_SSAOPipeline->BindDescriptorSet(m_CameraBuffer->GetDescriptorHandle(m_FrameIndex), 0U);
		m_SSAOPipeline->BindDescriptorSet(m_SSAOInput, 1U);

		m_SSAOPipeline->Draw(3U);
		
		m_SSAOPipeline->EndRendering();
		
		m_DepthBuffer->ChangeLayout(
			VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			m_CommandBuffers[m_FrameIndex]
		);

		m_SSAOTarget->ChangeLayout(
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			m_CommandBuffers[m_FrameIndex]
		);
	}
	void Renderer::TonemappingPass()
	{
		if (m_SkipFrame || !m_Scene) return;
		
		m_Swapchain->ChangeLayout(m_SwapchainImageIndex, VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			m_CommandBuffers[m_FrameIndex]);

		m_HDROffscreen->ChangeLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, m_CommandBuffers[m_FrameIndex]);

		const GraphicsPipeline::BindInfo info{
			.colorAttachments{
				{
					.imageView   = m_Swapchain->m_ImageViews[m_SwapchainImageIndex],
					.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
					.loadOp		 = VK_ATTACHMENT_LOAD_OP_CLEAR,
					.storeOp	 = VK_ATTACHMENT_STORE_OP_STORE
				}
			},

			.cullMode = VK_CULL_MODE_FRONT_BIT,

			.extent = m_Swapchain->GetExtent()
		};

		m_TonemappingPipeline->BeginRendering(m_CommandBuffers[m_FrameIndex], info);

		m_PostProcessParams.exposure.value = m_MainCamera->m_Exposure;

		m_TonemappingPipeline->PushConstants(&m_PostProcessParams.exposure, sizeof(PostProcessingParams::Exposure), 0U, VK_SHADER_STAGE_FRAGMENT_BIT);

		m_TonemappingPipeline->BindDescriptorSet(m_HDRInput);

		m_TonemappingPipeline->Draw(3U);

		m_TonemappingPipeline->EndRendering();
	}
	void Renderer::AntialiasPass()
	{
		if (m_SkipFrame || !m_Scene || m_PostProcessParams.antialiasingMode == AntialiasingMode::None) return;
		
		const GraphicsPipeline::BindInfo info{
			.colorAttachments{
				{
					.imageView	 = m_Swapchain->m_ImageViews[m_SwapchainImageIndex],
					.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
					.loadOp	     = VK_ATTACHMENT_LOAD_OP_LOAD,
					.storeOp     = VK_ATTACHMENT_STORE_OP_STORE
				}
			},

			.cullMode = VK_CULL_MODE_FRONT_BIT,

			.extent = m_Swapchain->GetExtent()
		};

		m_AntialiasingPipeline->BeginRendering(m_CommandBuffers[m_FrameIndex], info);
		
		m_AntialiasingPipeline->BindDescriptorSet(m_SwapchainInputs[m_SwapchainImageIndex]->GetHandle());

		m_PostProcessParams.antialiasing.texelSizeX = 1.0f / static_cast<float>(m_Swapchain->GetExtent().width);
		m_PostProcessParams.antialiasing.texelSizeY = 1.0f / static_cast<float>(m_Swapchain->GetExtent().height);

		m_AntialiasingPipeline->PushConstants(&m_PostProcessParams.antialiasing, sizeof(PostProcessingParams::Antialiasing), 0U, VK_SHADER_STAGE_FRAGMENT_BIT);

		m_AntialiasingPipeline->Draw(3U);

		m_AntialiasingPipeline->EndRendering();
	}
	void Renderer::ImGuiPass()
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
			EN_ERROR("Renderer::BeginRender() - Failed to begin recording ImGui command buffer!");

		vkCmdBeginRenderPass(m_ImGui.commandBuffers[m_SwapchainImageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_ImGui.commandBuffers[m_SwapchainImageIndex]);

		vkCmdEndRenderPass(m_ImGui.commandBuffers[m_SwapchainImageIndex]);

		if (vkEndCommandBuffer(m_ImGui.commandBuffers[m_SwapchainImageIndex]) != VK_SUCCESS)
			EN_ERROR("Renderer::EndRender() - Failed to record ImGui command buffer!");
	}
	void Renderer::EndRender()
	{
		if (m_SkipFrame) return;
		
		m_Swapchain->ChangeLayout(m_SwapchainImageIndex, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
								  0U, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
								  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
								  m_CommandBuffers[m_FrameIndex]);

		if (vkEndCommandBuffer(m_CommandBuffers[m_FrameIndex]) != VK_SUCCESS)
			EN_ERROR("Renderer::EndRender() - Failed to record command buffer!");

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
			EN_ERROR("Renderer::EndRender() - Failed to submit command buffer!");

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
			EN_ERROR("Renderer::EndRender() - Failed to present swap chain image!");

		m_FrameIndex = (m_FrameIndex + 1) % FRAMES_IN_FLIGHT;
	}

	void Renderer::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		g_CurrentBackend->m_FramebufferResized = true;
	}
	void Renderer::RecreateFramebuffer()
	{
		glm::ivec2 size(0);

		while (size.x == 0 || size.y == 0)
		{
			size = Window::Get().GetFramebufferSize();
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(m_Ctx->m_LogicalDevice);

		m_Swapchain.reset();
		m_Swapchain = MakeHandle<Swapchain>(m_VSync);

		CreateDepthBuffer();
		CreateHDROffscreen();
		CreateSSAOTarget();

		//UpdateClusterAABBs();

		UpdateForwardInput();
		UpdateSwapchainInputs();
		UpdateHDRInput();
		UpdateSSAOInput();

		for (auto& fence : m_SubmitFences)
			vkDestroyFence(m_Ctx->m_LogicalDevice, fence, nullptr);

		for (auto& semaphore : m_MainSemaphores)
			vkDestroySemaphore(m_Ctx->m_LogicalDevice, semaphore, nullptr);

		for (auto& semaphore : m_PresentSemaphores)
			vkDestroySemaphore(m_Ctx->m_LogicalDevice, semaphore, nullptr);

		CreateSyncObjects();

		m_Swapchain->CreateSwapchainFramebuffers(m_ImGui.renderPass);

		ImGui_ImplVulkan_SetMinImageCount(m_Swapchain->m_ImageViews.size());

		m_FramebufferResized = false;

		vkDeviceWaitIdle(m_Ctx->m_LogicalDevice);
	}
	void Renderer::ReloadBackend()
	{
		m_ReloadQueued = true;
	}
	void Renderer::ReloadBackendImpl()
	{
		EN_LOG("Reloading the renderer backend...");

		vkDeviceWaitIdle(m_Ctx->m_LogicalDevice);

		//for(auto& descriptor : m_ForwardClusteredDescriptor)
			//descriptor.reset();

		m_ForwardClusteredDescriptor.reset();

		m_HDRInput.reset();
		m_SSAOInput.reset();
		m_SwapchainInputs.clear();
		m_DepthPipeline.reset();
		m_ShadowPipeline.reset();
		m_OmniShadowPipeline.reset();
		m_ForwardClusteredPipeline.reset();
		m_SSAOPipeline.reset();
		m_TonemappingPipeline.reset();
		m_AntialiasingPipeline.reset();

		m_SSAOBuffer.reset();
		m_CameraBuffer.reset();

		DestroyShadows();

		m_Swapchain.reset();

		m_DepthBuffer.reset();
		m_HDROffscreen.reset();
		m_SSAOTarget.reset();

		for (auto& cmd : m_CommandBuffers)
			vkResetCommandBuffer(cmd, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

		for (auto& fence : m_SubmitFences)
			vkDestroyFence(m_Ctx->m_LogicalDevice, fence, nullptr);

		for (auto& semaphore : m_MainSemaphores)
			vkDestroySemaphore(m_Ctx->m_LogicalDevice, semaphore, nullptr);

		for (auto& semaphore : m_PresentSemaphores)
			vkDestroySemaphore(m_Ctx->m_LogicalDevice, semaphore, nullptr);

		Window::Get().SetResizeCallback(Renderer::FramebufferResizeCallback);

		m_Swapchain = MakeHandle<Swapchain>(m_VSync);

		m_CameraBuffer = MakeHandle<CameraBuffer>();
		m_CSMBuffer = MakeHandle<CSMBuffer>();

		CreateDepthBuffer();
		CreateHDROffscreen();
		CreateSSAOTarget();
		CreateSSAOBuffer();
		CreateClusterSSBOs();

		InitShadows();

		UpdateForwardInput();
		UpdateSwapchainInputs();
		UpdateHDRInput();
		UpdateSSAOInput();

		//UpdateClusterAABBs();

		CreateClusterComputePipelines();
		InitDepthPipeline();
		InitShadowPipeline();
		InitOmniShadowPipeline();
		InitForwardClusteredPipeline();
		InitSSAOPipeline();
		InitTonemappingPipeline();
		InitAntialiasingPipeline();

		CreateSyncObjects();

		m_Swapchain->CreateSwapchainFramebuffers(m_ImGui.renderPass);

		ImGui_ImplVulkan_SetMinImageCount(m_Swapchain->m_ImageViews.size());

		m_ReloadQueued = false;

		vkDeviceWaitIdle(m_Ctx->m_LogicalDevice);

		EN_SUCCESS("Succesfully reloaded the backend");
	}

	void Renderer::SetMainCamera(en::Handle<Camera> camera)
	{
		if (camera)
			m_MainCamera = camera;
		else
			m_MainCamera = g_DefaultCamera;

		UpdateShadowFrustums();

		//EN_SUCCESS("Updated cluster AABBs")
	}

	void Renderer::SetShadowCascadesFarPlane(float farPlane)
	{
		m_Shadows.cascadeFarPlane = farPlane;

		UpdateShadowFrustums();
	}

	void Renderer::SetPointShadowResolution(uint32_t resolution)
	{
		m_Shadows.point.resolution = resolution;

		ReloadBackend();
	}
	void Renderer::SetSpotShadowResolution(uint32_t resolution)
	{
		m_Shadows.spot.resolution = resolution;

		ReloadBackend();
	}
	void Renderer::SetDirShadowResolution(uint32_t resolution)
	{
		m_Shadows.dir.resolution = resolution;

		ReloadBackend();
	}

	void Renderer::SetShadowFormat(VkFormat format)
	{
		m_Shadows.shadowFormat = format;

		ReloadBackend();
	}
	void Renderer::SetShadowCascadesWeight(float weight)
	{
		m_Shadows.cascadeSplitWeight = weight;

		UpdateShadowFrustums();
	}

	void Renderer::CreateSSAOBuffer()
	{
		std::uniform_real_distribution<float> dist(0.0, 1.0);
		std::default_random_engine gen;

		std::array<glm::vec4, 64> kernels{};
		std::array<glm::vec4, 16> noise{};

		for (int i = 0; i < kernels.size(); ++i)
		{
			glm::vec3 sample(
				dist(gen) * 2.0f - 1.0f,
				dist(gen) * 2.0f - 1.0f,
				dist(gen)
			);

			sample = glm::normalize(sample);
			sample *= dist(gen);

			float scale = (float)i / 64.0f;
			sample *= glm::mix(0.1f, 1.0f, scale * scale);

			kernels[i] = glm::vec4(sample, 0.0f);

		}

		for (int i = 0; i < noise.size(); ++i)
		{
			noise[i] = glm::vec4(
				dist(gen) * 2.0f - 1.0f,
				dist(gen) * 2.0f - 1.0f,
				0.0f,
				0.0f
			);
		}

		std::array<glm::vec4, kernels.size() + noise.size()> data{};

		std::move(kernels.begin(), kernels.end(), data.begin());
		std::move(noise.begin(), noise.end(), data.begin() + kernels.size());

		constexpr VkDeviceSize dataSize = sizeof(glm::vec4) * data.size();

		MemoryBuffer stagingBuffer(
			dataSize, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			VMA_MEMORY_USAGE_CPU_TO_GPU
		);
		stagingBuffer.MapMemory(data.data(), dataSize);

		m_SSAOBuffer = MakeHandle<MemoryBuffer>(dataSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);

		stagingBuffer.CopyTo(m_SSAOBuffer);
	}
	void Renderer::CreateClusterSSBOs()
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

	void Renderer::CreateClusterComputePipelines()
	{
		DescriptorSet::BufferInfo aabbBufferInfo{
			.index = 0U,
			.buffer = m_ClusterSSBOs.aabbClusters->GetHandle(),
			.size = m_ClusterSSBOs.aabbClusters->m_BufferSize,
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT
		};

		m_ClusterSSBOs.aabbClustersDescriptor = MakeHandle<DescriptorSet>(
			std::vector<DescriptorSet::ImageInfo>{},
			std::vector<DescriptorSet::BufferInfo>{aabbBufferInfo}
		);

		DescriptorSet::BufferInfo lightBuffer{
			.index = 1U,
			.buffer = m_LightsBuffer->GetHandle(),
			.size = m_LightsBuffer->GetSize(),
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT
		};

		DescriptorSet::BufferInfo pointLightGridBufferInfo{
			.index = 2U,
			.buffer = m_ClusterSSBOs.pointLightGrid->GetHandle(),
			.size = m_ClusterSSBOs.pointLightGrid->m_BufferSize,
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		};

		DescriptorSet::BufferInfo pointLightIndicesBufferInfo{
			.index = 3U,
			.buffer = m_ClusterSSBOs.pointLightIndices->GetHandle(),
			.size = m_ClusterSSBOs.pointLightIndices->m_BufferSize,
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT
		};

		const DescriptorSet::BufferInfo pointLightGlobalIndexOffsetBuffer{
			.index = 4U,
			.buffer = m_ClusterSSBOs.pointLightGlobalIndexOffset->GetHandle(),
			.size = m_ClusterSSBOs.pointLightGlobalIndexOffset->m_BufferSize,
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT
		};

		DescriptorSet::BufferInfo spotLightGridBufferInfo{
			.index = 5U,
			.buffer = m_ClusterSSBOs.spotLightGrid->GetHandle(),
			.size = m_ClusterSSBOs.spotLightGrid->m_BufferSize,
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		};

		DescriptorSet::BufferInfo spotLightIndicesBufferInfo{
			.index = 6U,
			.buffer = m_ClusterSSBOs.spotLightIndices->GetHandle(),
			.size = m_ClusterSSBOs.spotLightIndices->m_BufferSize,
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT
		};

		const DescriptorSet::BufferInfo spotLightGlobalIndexOffsetBuffer{
			.index = 7U,
			.buffer = m_ClusterSSBOs.spotLightGlobalIndexOffset->GetHandle(),
			.size = m_ClusterSSBOs.spotLightGlobalIndexOffset->m_BufferSize,
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT
		};

		m_ClusterSSBOs.clusterLightCullingDescriptor = MakeHandle<DescriptorSet>(
			std::vector<DescriptorSet::ImageInfo>{},
			std::vector<DescriptorSet::BufferInfo>{
			aabbBufferInfo,
			lightBuffer,
			pointLightGridBufferInfo,
			pointLightIndicesBufferInfo,
			pointLightGlobalIndexOffsetBuffer,
			spotLightGridBufferInfo,
			spotLightIndicesBufferInfo,
			spotLightGlobalIndexOffsetBuffer,
		});

		constexpr VkPushConstantRange stvPushConstant{
			.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
			.offset = 0U,
			.size = sizeof(glm::uvec4)
		};

		ComputePipeline::CreateInfo aabbInfo{
			.sourcePath = "Shaders/ClusterAABB.spv",
			.descriptorLayouts = { m_ClusterSSBOs.aabbClustersDescriptor->GetLayout(), m_CameraBuffer->GetLayout()},
			.pushConstantRanges = { stvPushConstant },
		};

		m_ClusterAABBCompute = MakeHandle<ComputePipeline>(aabbInfo);

		ComputePipeline::CreateInfo lightCullInfo{
			.sourcePath = "Shaders/ClusterLightCulling.spv",
			.descriptorLayouts = { m_ClusterSSBOs.clusterLightCullingDescriptor->GetLayout(), m_CameraBuffer->GetLayout()},
		};

		m_ClusterLightCullingCompute = MakeHandle<ComputePipeline>(lightCullInfo);
	}

	void Renderer::CreateDepthBuffer()
	{
		m_DepthBuffer = MakeHandle<Image>(
			m_Swapchain->GetExtent(),
			VK_FORMAT_D32_SFLOAT,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		);

		Helpers::CreateSampler(m_MainSampler/*, VK_FILTER_NEAREST*/);
	}
	void Renderer::CreateHDROffscreen()
	{
		m_HDROffscreen = MakeHandle<Image>(m_Swapchain->GetExtent(), VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}
	void Renderer::CreateSSAOTarget()
	{
		m_SSAOTarget = MakeHandle<Image>(
			m_Swapchain->GetExtent(), VK_FORMAT_R16_SFLOAT, 
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
			VK_IMAGE_ASPECT_COLOR_BIT, 
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);
	}

	void Renderer::UpdateForwardInput()
	{
		const DescriptorSet::ImageInfo pointShadowmaps{
			.index = 0U,
			.imageView = m_Shadows.point.sharedView,
			.imageSampler = m_Shadows.sampler,
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
		};

		const DescriptorSet::ImageInfo spotShadowmaps{
			.index = 1U,
			.imageView = m_Shadows.spot.sharedView,
			.imageSampler = m_Shadows.sampler,
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
		};

		const DescriptorSet::ImageInfo dirShadowmaps{
			.index = 2U,
			.imageView = m_Shadows.dir.sharedView,
			.imageSampler = m_Shadows.sampler,
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
		};

		const DescriptorSet::BufferInfo lightBuffer{
			.index = 3U,
			.buffer = m_LightsBuffer->GetHandle(),
			.size = m_LightsBuffer->GetSize(),
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		};

		const DescriptorSet::BufferInfo pointLightIndexBuffer{
			.index = 4U,
			.buffer = m_ClusterSSBOs.pointLightIndices->GetHandle(),
			.size = m_ClusterSSBOs.pointLightIndices->m_BufferSize,
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
		};

		const DescriptorSet::BufferInfo pointLightGridBuffer{
			.index = 5U,
			.buffer = m_ClusterSSBOs.pointLightGrid->GetHandle(),
			.size = m_ClusterSSBOs.pointLightGrid->m_BufferSize,
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
		};

		const DescriptorSet::BufferInfo spotLightIndexBuffer{
			.index = 6U,
			.buffer = m_ClusterSSBOs.spotLightIndices->GetHandle(),
			.size = m_ClusterSSBOs.spotLightIndices->m_BufferSize,
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
		};

		const DescriptorSet::BufferInfo spotLightGridBuffer{
			.index = 7U,
			.buffer = m_ClusterSSBOs.spotLightGrid->GetHandle(),
			.size = m_ClusterSSBOs.spotLightGrid->m_BufferSize,
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
		};

		const DescriptorSet::ImageInfo ssao {
			.index = 8U,
			.imageView = m_SSAOTarget->GetViewHandle(),
			.imageSampler = m_MainSampler,
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
		};

		auto imageInfos = { 
			pointShadowmaps, 
			spotShadowmaps, 
			dirShadowmaps,
			ssao,
		};

		auto bufferInfos = { 
			lightBuffer, 

			pointLightIndexBuffer, 
			pointLightGridBuffer,

			spotLightIndexBuffer,
			spotLightGridBuffer,
		};

		if (!m_ForwardClusteredDescriptor)
			m_ForwardClusteredDescriptor = MakeHandle<DescriptorSet>(imageInfos, bufferInfos);
		else
			m_ForwardClusteredDescriptor->Update(imageInfos, bufferInfos);
	}
	void Renderer::UpdateHDRInput()
	{
		const DescriptorSet::ImageInfo HDRColorBuffer{
			.index		  = 0U,
			.imageView	  = m_HDROffscreen->GetViewHandle(),
			.imageSampler = m_MainSampler
		};

		auto imageInfos = { HDRColorBuffer};

		if (!m_HDRInput)
			m_HDRInput = MakeHandle<DescriptorSet>(imageInfos, std::vector<DescriptorSet::BufferInfo>{});
		else
			m_HDRInput->Update(imageInfos, std::vector<DescriptorSet::BufferInfo>{});
	}
	void Renderer::UpdateSwapchainInputs()
	{
		m_SwapchainInputs.resize(m_Swapchain->m_ImageViews.size());

		for (int i = 0; i < m_SwapchainInputs.size(); i++)
		{
			const DescriptorSet::ImageInfo image {
				.index		  = 0U,
				.imageView	  = m_Swapchain->m_ImageViews[i],
				.imageSampler = m_MainSampler,
				.imageLayout  = VK_IMAGE_LAYOUT_GENERAL,
			};

			auto imageInfo = { image };

			if (!m_SwapchainInputs[i])
				m_SwapchainInputs[i] = MakeHandle<DescriptorSet>(imageInfo, std::vector<DescriptorSet::BufferInfo>{});
			else
				m_SwapchainInputs[i]->Update(imageInfo, std::vector<DescriptorSet::BufferInfo>{});
		}
	}
	void Renderer::UpdateSSAOInput()
	{
		const DescriptorSet::ImageInfo depth{
			.index		  = 0U,
			.imageView	  = m_DepthBuffer->GetViewHandle(),
			.imageSampler = m_MainSampler
		};

		const DescriptorSet::BufferInfo buffer {
			.index  = 1U,
			.buffer = m_SSAOBuffer->GetHandle(),
			.size	= m_SSAOBuffer->m_BufferSize
		};
		
		auto imageInfos = { depth };
		auto bufferInfos = { buffer };
		
		if (!m_SSAOInput)
			m_SSAOInput = MakeHandle<DescriptorSet>(imageInfos, bufferInfos);
		else
			m_SSAOInput->Update(imageInfos, bufferInfos);
	}

	void Renderer::InitShadows()
	{
		Helpers::CreateSampler(m_Shadows.sampler, VK_FILTER_NEAREST);

		// Point
		Helpers::CreateImage(m_Shadows.point.sharedImage, m_Shadows.point.allocation, VkExtent2D{ m_Shadows.point.resolution, m_Shadows.point.resolution }, VK_FORMAT_R32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 6 * MAX_POINT_LIGHT_SHADOWS, 1U, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);

		m_Shadows.point.singleViews.resize(MAX_POINT_LIGHT_SHADOWS);

		for (int i = 0; i < m_Shadows.point.singleViews.size(); i++)
			for (int j = 0; j < 6; j++)
				Helpers::CreateImageView(m_Shadows.point.sharedImage, m_Shadows.point.singleViews[i][j], VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, i * 6 + j, 1U);

		Helpers::CreateImageView(m_Shadows.point.sharedImage, m_Shadows.point.sharedView, VK_IMAGE_VIEW_TYPE_CUBE_ARRAY, VK_FORMAT_R32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 0U, 6 * MAX_POINT_LIGHT_SHADOWS);

		Helpers::CreateImage(m_Shadows.point.depthImage, m_Shadows.point.depthAllocation, VkExtent2D{ m_Shadows.point.resolution, m_Shadows.point.resolution }, m_Shadows.shadowFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		Helpers::CreateImageView(m_Shadows.point.depthImage, m_Shadows.point.depthView, VK_IMAGE_VIEW_TYPE_2D, m_Shadows.shadowFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

		// Spot
		Helpers::CreateImage(m_Shadows.spot.sharedImage, m_Shadows.spot.allocation, VkExtent2D{ m_Shadows.spot.resolution, m_Shadows.spot.resolution }, m_Shadows.shadowFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, MAX_SPOT_LIGHT_SHADOWS);

		m_Shadows.spot.singleViews.resize(MAX_SPOT_LIGHT_SHADOWS);

		for (uint32_t i = 0; auto & view : m_Shadows.spot.singleViews)
			Helpers::CreateImageView(m_Shadows.spot.sharedImage, view, VK_IMAGE_VIEW_TYPE_2D, m_Shadows.shadowFormat, VK_IMAGE_ASPECT_DEPTH_BIT, i++);

		Helpers::CreateImageView(m_Shadows.spot.sharedImage, m_Shadows.spot.sharedView, VK_IMAGE_VIEW_TYPE_2D_ARRAY, m_Shadows.shadowFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 0U, MAX_SPOT_LIGHT_SHADOWS);


		// Dir
		Helpers::CreateImage(m_Shadows.dir.sharedImage, m_Shadows.dir.allocation, VkExtent2D{ m_Shadows.dir.resolution, m_Shadows.dir.resolution }, m_Shadows.shadowFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, MAX_DIR_LIGHT_SHADOWS * SHADOW_CASCADES);
	
		m_Shadows.dir.singleViews.resize(MAX_DIR_LIGHT_SHADOWS * SHADOW_CASCADES);

		for (uint32_t i = 0; auto& view : m_Shadows.dir.singleViews)
			Helpers::CreateImageView(m_Shadows.dir.sharedImage, view, VK_IMAGE_VIEW_TYPE_2D, m_Shadows.shadowFormat, VK_IMAGE_ASPECT_DEPTH_BIT, i++);
		
		Helpers::CreateImageView(m_Shadows.dir.sharedImage, m_Shadows.dir.sharedView, VK_IMAGE_VIEW_TYPE_2D_ARRAY, m_Shadows.shadowFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 0U, MAX_DIR_LIGHT_SHADOWS * SHADOW_CASCADES);

		Helpers::TransitionImageLayout(m_Shadows.point.sharedImage, m_Shadows.shadowFormat, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0U, 6 * MAX_POINT_LIGHT_SHADOWS
		);

		Helpers::TransitionImageLayout(m_Shadows.point.depthImage, m_Shadows.shadowFormat, VK_IMAGE_ASPECT_DEPTH_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
		);

		Helpers::TransitionImageLayout(m_Shadows.spot.sharedImage, m_Shadows.shadowFormat, VK_IMAGE_ASPECT_DEPTH_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0U, MAX_SPOT_LIGHT_SHADOWS
		);
		Helpers::TransitionImageLayout(m_Shadows.dir.sharedImage, m_Shadows.shadowFormat, VK_IMAGE_ASPECT_DEPTH_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0U, MAX_DIR_LIGHT_SHADOWS * SHADOW_CASCADES
		);
	}
	void Renderer::DestroyShadows()
	{
		UseContext();

		vkDestroySampler(ctx.m_LogicalDevice, m_Shadows.sampler, nullptr);

		// Point
		vmaDestroyImage(ctx.m_Allocator, m_Shadows.point.sharedImage, m_Shadows.point.allocation);
		for (auto& cubeView : m_Shadows.point.singleViews)
			for (auto& view : cubeView)
				vkDestroyImageView(ctx.m_LogicalDevice, view, nullptr);

		vkDestroyImageView(ctx.m_LogicalDevice, m_Shadows.point.sharedView, nullptr);

		vmaDestroyImage(ctx.m_Allocator, m_Shadows.point.depthImage, m_Shadows.point.depthAllocation);
		vkDestroyImageView(ctx.m_LogicalDevice, m_Shadows.point.depthView, nullptr);

		// Spot
		vmaDestroyImage(ctx.m_Allocator, m_Shadows.spot.sharedImage, m_Shadows.spot.allocation);
		for (auto& view : m_Shadows.spot.singleViews)
			vkDestroyImageView(ctx.m_LogicalDevice, view, nullptr);

		vkDestroyImageView(ctx.m_LogicalDevice, m_Shadows.spot.sharedView, nullptr);

		// Dir
		vmaDestroyImage(ctx.m_Allocator, m_Shadows.dir.sharedImage, m_Shadows.dir.allocation);
		for (auto& view : m_Shadows.dir.singleViews)
			vkDestroyImageView(ctx.m_LogicalDevice, view, nullptr);

		vkDestroyImageView(ctx.m_LogicalDevice, m_Shadows.dir.sharedView, nullptr);
	}

	void Renderer::InitDepthPipeline()
	{
		constexpr VkPushConstantRange depthPushConstant {
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset		= 0U,
			.size		= sizeof(DepthStageInfo)
		};

		const GraphicsPipeline::CreateInfo pipelineInfo{
			.depthFormat		= m_DepthBuffer->m_Format,
			.vShader			= "Shaders/Depth.spv",
			.pushConstantRanges = { depthPushConstant },
			.useVertexBindings  = true,
			.enableDepthTest    = true
		};

		m_DepthPipeline = MakeHandle<GraphicsPipeline>(pipelineInfo);
	}
	void Renderer::InitShadowPipeline()
	{
		constexpr VkPushConstantRange depthPushConstant {
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 0U,
			.size = sizeof(DepthStageInfo)
		};

		const GraphicsPipeline::CreateInfo pipelineInfo {
			.depthFormat = m_Shadows.shadowFormat,
			.vShader = "Shaders/Depth.spv",
			.pushConstantRanges = { depthPushConstant },
			.useVertexBindings = true,
			.enableDepthTest = true
		};

		m_ShadowPipeline = MakeHandle<GraphicsPipeline>(pipelineInfo);
	}
	void Renderer::InitOmniShadowPipeline()
	{
		constexpr VkPushConstantRange depthPushConstant{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 0U,
			.size = sizeof(Shadows::Point::OmniShadowPushConstant)
		};

		const GraphicsPipeline::CreateInfo pipelineInfo{
			.colorFormats = { VK_FORMAT_R32_SFLOAT },
			.depthFormat = m_Shadows.shadowFormat,
			.vShader = "Shaders/OmniDepthVert.spv",
			.fShader = "Shaders/OmniDepthFrag.spv",
			.descriptorLayouts = {},
			.pushConstantRanges = { depthPushConstant },
			.useVertexBindings = true,
			.enableDepthTest = true
		};

		m_OmniShadowPipeline = MakeHandle<GraphicsPipeline>(pipelineInfo);
	}
	void Renderer::InitForwardClusteredPipeline()
	{
		constexpr VkPushConstantRange objectPushConstant {
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 0U,
			.size = sizeof(glm::mat4),
		};

		constexpr VkPushConstantRange materialPushConstant {
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.offset = sizeof(glm::mat4),
			.size = 24U,
		};

		const GraphicsPipeline::CreateInfo pipelineInfo{
			.colorFormats = { m_HDROffscreen->m_Format},
			.depthFormat = m_DepthBuffer->m_Format,
			.vShader = "Shaders/ForwardClusteredVert.spv",
			.fShader = "Shaders/ForwardClusteredFrag.spv",
			.descriptorLayouts = { 
				m_CameraBuffer->GetLayout(),
				Material::GetLayout(),
				m_ForwardClusteredDescriptor->GetLayout(),
				m_CSMBuffer->GetLayout()
			},
			.pushConstantRanges = { objectPushConstant, materialPushConstant },
			.useVertexBindings = true,
			.enableDepthTest = true,
			.enableDepthWrite = false,
			.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
		};

		m_ForwardClusteredPipeline = MakeHandle<GraphicsPipeline>(pipelineInfo);
	}
	void Renderer::InitSSAOPipeline()
	{
		constexpr VkPushConstantRange ssaoPushConstant { 
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.offset = 0U,
			.size = sizeof(PostProcessingParams::AmbientOcclusion),
		};
		
		const GraphicsPipeline::CreateInfo pipelineInfo{
			.colorFormats = { m_SSAOTarget->m_Format},
			.vShader = "Shaders/FullscreenTriVert.spv",
			.fShader = "Shaders/SSAO.spv",
			.descriptorLayouts = { m_CameraBuffer->GetLayout(), m_SSAOInput->GetLayout()},
			.pushConstantRanges = { ssaoPushConstant }
			
		};
		
		m_SSAOPipeline = MakeHandle<GraphicsPipeline>(pipelineInfo);
	}
	void Renderer::InitTonemappingPipeline()
	{
		constexpr VkPushConstantRange exposurePushConstant {
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.offset		= 0U,
			.size		= sizeof(PostProcessingParams::Exposure)
		};

		const GraphicsPipeline::CreateInfo pipelineInfo{
			.colorFormats		= { m_Swapchain->GetFormat() },
			.vShader			= "Shaders/FullscreenTriVert.spv",
			.fShader			= "Shaders/Tonemapping.spv",
			.descriptorLayouts  = { m_HDRInput->GetLayout() },
			.pushConstantRanges = { exposurePushConstant }
		};

		m_TonemappingPipeline = MakeHandle<GraphicsPipeline>(pipelineInfo);
	}
	void Renderer::InitAntialiasingPipeline()
	{
		if (m_PostProcessParams.antialiasingMode == AntialiasingMode::None) return;

		constexpr VkPushConstantRange antialiasingPushConstant{
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.offset = 0U,
			.size = sizeof(PostProcessingParams::Antialiasing)
		};

		const GraphicsPipeline::CreateInfo pipelineInfo{
			.colorFormats		= { m_Swapchain->GetFormat() },
			.vShader			= "Shaders/FullscreenTriVert.spv",
			.fShader			= "Shaders/FXAA.spv",
			.descriptorLayouts  = { m_SwapchainInputs[0]->GetLayout()},
			.pushConstantRanges = { antialiasingPushConstant },
		};

		m_AntialiasingPipeline = MakeHandle<GraphicsPipeline>(pipelineInfo);
	}

	void Renderer::CreateCommandBuffer()
	{
		const VkCommandBufferAllocateInfo allocInfo{
			.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool		= m_Ctx->m_GraphicsCommandPool,
			.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = FRAMES_IN_FLIGHT
		};

		if (vkAllocateCommandBuffers(m_Ctx->m_LogicalDevice, &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS)
			EN_ERROR("Renderer::GCreateCommandBuffer() - Failed to allocate command buffer!");
	}
	void Renderer::CreateSyncObjects()
	{
		constexpr VkFenceCreateInfo fenceInfo{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT
		};

		for(auto& fence : m_SubmitFences)
			if (vkCreateFence(m_Ctx->m_LogicalDevice, &fenceInfo, nullptr, &fence) != VK_SUCCESS)
				EN_ERROR("Renderer::CreateSyncObjects() - Failed to create submit fences!");
		
		constexpr VkSemaphoreCreateInfo semaphoreInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
		};
		
		for (auto& semaphore : m_MainSemaphores)
			if (vkCreateSemaphore(m_Ctx->m_LogicalDevice, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS)
				EN_ERROR("GraphicsPipeline::CreateSyncSemaphore - Failed to create main semaphores!");

		for (auto& semaphore : m_PresentSemaphores)
			if (vkCreateSemaphore(m_Ctx->m_LogicalDevice, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS)
				EN_ERROR("GraphicsPipeline::CreateSyncSemaphore - Failed to create present semaphores!");
	}

	void ImGuiCheckResult(VkResult err)
	{
		if (err == 0) return;

		std::cerr << err << '\n';
		EN_ERROR("ImGui has caused a problem!");
	}
	void Renderer::InitImGui()
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
			EN_ERROR("Renderer::InitImGui() - Failed to create ImGui's render pass!");

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
			EN_ERROR("Renderer::InitImGui() - Failed to create ImGui's descriptor pool!");

		ImGui_ImplGlfw_InitForVulkan(Window::Get().m_NativeHandle, true);

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

		VkFormat format = m_Swapchain->GetFormat();

		ImGui_ImplVulkanH_SelectSurfaceFormat(m_Ctx->m_PhysicalDevice, m_Ctx->m_WindowSurface, &format, 1, VK_COLORSPACE_SRGB_NONLINEAR_KHR);
	}
	
	void Renderer::RecalculateShadowMatrices(const DirectionalLight& light, glm::mat4* lightMatrices)
	{
		for (int i = 0; i < SHADOW_CASCADES; ++i)
		{
			float radius = m_Shadows.frustums[i].radius;
			glm::vec3 center = m_Shadows.frustums[i].center;

			glm::mat4 lightProj = glm::ortho(-radius, radius, -radius, radius, 0.0f, light.m_FarPlane * radius);
			glm::mat4 lightView = glm::lookAt(center - (light.m_FarPlane-1.0f) * -glm::normalize(light.m_Direction + glm::vec3(0.0000001f, 0, 0.0000001f)) * radius, center, glm::vec3(0, 1, 0));

			glm::mat4 shadowViewProj = lightProj * lightView;
			glm::vec4 shadowOrigin = shadowViewProj * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f) * float(m_Shadows.dir.resolution) / 2.0f;

			glm::vec4 shadowOffset = (glm::round(shadowOrigin) - shadowOrigin) * 2.0f / float(m_Shadows.dir.resolution) * glm::vec4(1, 1, 0, 0);

			glm::mat4 shadowProj = lightProj;
			shadowProj[3] += shadowOffset;

			lightMatrices[i] = shadowProj * lightView;
		}
	}
	void Renderer::UpdateShadowFrustums()
	{
		// CSM Implementation heavily inspired with:
		// Blaze engine by kidrigger - https://github.com/kidrigger/Blaze
		// Flex Engine by ajweeks - https://github.com/ajweeks/FlexEngine
		
		float lambda = m_Shadows.cascadeSplitWeight;

		float ratio = m_Shadows.cascadeFarPlane / m_MainCamera->m_NearPlane;
		
		for (uint32_t i = 1; i < SHADOW_CASCADES; i++) 
		{
			float si = i / float(SHADOW_CASCADES);
		
			float nearPlane = lambda * (m_MainCamera->m_NearPlane * powf(ratio, si)) + (1.0f - lambda) * (m_MainCamera->m_NearPlane + (m_Shadows.cascadeFarPlane - m_MainCamera->m_NearPlane) * si);
			float farPlane = nearPlane * 1.005f;
			m_Shadows.frustums[i-1].split = farPlane;
		}
		
		m_Shadows.frustums[SHADOW_CASCADES-1].split = m_Shadows.cascadeFarPlane;


		for (int i = 0; i < SHADOW_CASCADES; i++)
			m_CSMBuffer->m_CSMBOs[m_FrameIndex].cascadeSplitDistances[i].x = m_Shadows.frustums[i].split;

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

		glm::mat4 invViewProj = glm::inverse(m_CameraBuffer->m_CBOs[m_FrameIndex].proj * m_CameraBuffer->m_CBOs[m_FrameIndex].view);

		for (auto& vert : frustumClipSpace)
		{
			vert = invViewProj * vert;
			vert /= vert.w;
		}

		float cosine = glm::dot(glm::normalize(glm::vec3(frustumClipSpace[4]) - m_MainCamera->m_Position), m_MainCamera->GetFront());
		glm::vec3 cornerRay = glm::normalize(glm::vec3(frustumClipSpace[4] - frustumClipSpace[0]));

		float prevFarPlane = m_MainCamera->m_NearPlane;

		for (int i = 0; i < SHADOW_CASCADES; ++i)
		{
			float farPlane = m_Shadows.frustums[i].split;
			float secTheta = 1.0f / cosine;
			float cDist = 0.5f * (farPlane + prevFarPlane) * secTheta * secTheta;
			m_Shadows.frustums[i].center = m_MainCamera->GetFront() * cDist + m_MainCamera->m_Position;

			float nearRatio = prevFarPlane / m_Shadows.cascadeFarPlane;
			glm::vec3 corner = cornerRay * nearRatio + m_MainCamera->m_Position;

			m_Shadows.frustums[i].radius = glm::distance(m_Shadows.frustums[i].center, corner);

			prevFarPlane = farPlane;
		}

		m_Shadows.frustums[0].ratio = 1.0f;
		for (int i = 1; i < SHADOW_CASCADES; i++)
			m_Shadows.frustums[i].ratio = m_Shadows.frustums[i].radius / m_Shadows.frustums[0].radius;

		for (int i = 0; i < SHADOW_CASCADES; i++)
			m_CSMBuffer->m_CSMBOs[m_FrameIndex].cascadeFrustumSizeRatios[i].x = m_Shadows.frustums[i].ratio;
	}
}