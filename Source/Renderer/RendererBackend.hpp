#pragma once

#ifndef EN_RENDERERBACKEND_HPP
#define EN_RENDERERBACKEND_HPP

#include "../../EruptionEngine.ini"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <Common/Helpers.hpp>

#include <Scene/Scene.hpp>

#include <Renderer/Window.hpp>
#include <Renderer/Context.hpp>
#include <Renderer/Shader.hpp>
#include <Renderer/ComputeShader.hpp>

#include <Renderer/Lights/PointLight.hpp>
#include <Renderer/Lights/DirectionalLight.hpp>
#include <Renderer/Lights/SpotLight.hpp>

#include <Renderer/Camera/Camera.hpp>
#include <Renderer/Camera/CameraMatricesBuffer.hpp>

#include <Renderer/Pipeline.hpp>
#include <Renderer/DescriptorSet.hpp>
#include <Renderer/DynamicFramebuffer.hpp>

#include <Renderer/Swapchain.hpp>

namespace en
{
	class RendererBackend
	{
	public:
		~RendererBackend();

		void Init();

		void BindScene(Scene* scene);
		void UnbindScene();

		void SetVSync(bool vSync);

		void WaitForGPUIdle();

		void UpdateLights();

		void BeginRender();

		void ReloadBackend();

		void DepthPass();
		void SSAOPass();
		void ShadowPass();
		void ClusteredForwardPass();
		void TonemappingPass();
		void AntialiasPass();
		void ImGuiPass();

		void EndRender();

		void SetMainCamera(Camera* camera);
		Camera* GetMainCamera() { return m_MainCamera; };


		void SetShadowCascadesWeight(float weight);
		const float GetShadowCascadesWeight() const { return m_Shadows.cascadeSplitWeight; }

		void SetShadowCascadesFarPlane(float farPlane);
		const float GetShadowCascadesFarPlane() const { return m_Shadows.cascadeFarPlane; }



		void SetPointShadowResolution(uint32_t resolution);
		const float GetPointShadowResolution() const { return m_Shadows.point.resolution; }

		void SetSpotShadowResolution(uint32_t resolution);
		const float GetSpotShadowResolution() const { return m_Shadows.spot.resolution; }

		void SetDirShadowResolution(uint32_t resolution);
		const float GetDirShadowResolution() const { return m_Shadows.dir.resolution; }


		void SetShadowFormat(VkFormat format);
		const VkFormat GetShadowFormat() const { return m_Shadows.shadowFormat; }

		Scene* GetScene() { return m_Scene; };

		int m_DebugMode = 0;

		std::function<void()> m_ImGuiRenderCallback;

		enum struct AntialiasingMode
		{
			FXAA = 0,
			None = 1
		};

		enum struct AmbientOcclusionMode
		{
			SSAO = 0,
			None = 1
		};

		struct PostProcessingParams
		{
			struct Exposure
			{
				float value = 1.0f;
			} exposure;

			AntialiasingMode antialiasingMode = AntialiasingMode::FXAA;
			AmbientOcclusionMode ambientOcclusionMode = AmbientOcclusionMode::None;

			struct Antialiasing
			{
				float fxaaSpanMax = 4.0f;
				float fxaaReduceMin = 0.001f;
				float fxaaReduceMult = 0.050f;

				float fxaaPower = 1.0f;

				float texelSizeX = 1.0f / 1920.0f;
				float texelSizeY = 1.0f / 1080.0f;
			} antialiasing;

			struct AmbientOcclusion
			{
				float screenWidth = 1920.0f;
				float screenHeight = 1080.0f;
				
				float radius = 0.5f;
				float bias = 0.025f;
				float multiplier = 1.0f;
			} ambientOcclusion;

		} m_PostProcessParams;

	private:
		struct Lights
		{
			struct LightsBufferObject
			{
				PointLight::Buffer		 pointLights[MAX_POINT_LIGHTS];
				SpotLight::Buffer		 spotLights[MAX_SPOT_LIGHTS];
				DirectionalLight::Buffer dirLights[MAX_DIR_LIGHTS];

				uint32_t activePointLights = 0U;
				uint32_t activeSpotLights = 0U;
				uint32_t activeDirLights = 0U;
				float dummy0 = 0.0f;

				glm::vec3 ambientLight = glm::vec3(0.0f);
				float dummy1 = 0.0f;

				glm::vec4 cascadeSplitDistances[SHADOW_CASCADES]{};
				glm::vec4 frustumSizeRatios[SHADOW_CASCADES]{};

				glm::vec3 viewPos = glm::vec3(0.0f);
				int debugMode = 0;

				glm::uvec4 tileCount;

				glm::uvec4 tileSizes;

				float scale;
				float bias;

			} LBO;

			std::array<std::unique_ptr<MemoryBuffer>, FRAMES_IN_FLIGHT> stagingBuffers;
			std::unique_ptr<MemoryBuffer> buffer;

		} m_Lights;

		struct Shadows
		{
			VkSampler sampler;
			
			VkFormat shadowFormat = VK_FORMAT_D32_SFLOAT;

			struct Point 
			{
				VkImage sharedImage;
				VkDeviceMemory memory;

				VkImageView sharedView;
				std::vector<std::array<VkImageView, 6>> singleViews;

				std::array<std::array<glm::mat4, 6>, MAX_POINT_LIGHT_SHADOWS> shadowMatrices;
				std::array<glm::vec3, MAX_POINT_LIGHT_SHADOWS> lightPositions;
				std::array<float, MAX_POINT_LIGHT_SHADOWS> farPlanes;

				VkImage depthImage;
				VkImageView depthView;
				VkDeviceMemory depthMemory;

				uint32_t resolution = 1024;

				struct OmniShadowPushConstant
				{
					glm::mat4x4 viewProj;
					glm::mat4x4 model;
					// last row is (lightPos.x, lightPos.y, lightPos.z, farPlane)
				};

			} point;
			struct Spot
			{
				VkImage sharedImage;
				VkDeviceMemory memory;

				VkImageView sharedView;
				std::vector<VkImageView> singleViews;

				uint32_t resolution = 2048;

			} spot;
			struct Dir
			{
				VkImage sharedImage;
				VkDeviceMemory memory;

				VkImageView sharedView;
				std::vector<VkImageView> singleViews;

				uint32_t resolution = 4096;
			} dir;

			struct Cascade
			{
				float split, radius, ratio;
				
				glm::vec3 center;
			};

			std::array<Cascade, SHADOW_CASCADES> frustums;
			
			float cascadeFarPlane = 140.0f;
			float cascadeSplitWeight = 0.87f;

		} m_Shadows;

		struct DepthStageInfo
		{
			glm::mat4 modelMatrix;
			glm::mat4 viewProjMatrix;
		};

		struct ImGuiVK
		{
			VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
			VkRenderPass	 renderPass = VK_NULL_HANDLE;

			VkCommandPool commandPool = VK_NULL_HANDLE;

			std::vector<VkCommandBuffer> commandBuffers{};

			void Destroy()
			{
				UseContext();

				ImGui_ImplVulkan_Shutdown();
				ImGui_ImplGlfw_Shutdown();
				ImGui::DestroyContext();

				vkDestroyCommandPool(ctx.m_LogicalDevice, commandPool, nullptr);

				vkDestroyDescriptorPool(ctx.m_LogicalDevice, descriptorPool, nullptr);

				vkDestroyRenderPass(ctx.m_LogicalDevice, renderPass, nullptr);
			}

		} m_ImGui;

		struct ClusterSSBOs
		{
			std::unique_ptr<MemoryBuffer> aabbClusters;
			std::unique_ptr<DescriptorSet> aabbClustersDescriptor;

			std::unique_ptr<MemoryBuffer> pointLightGrid;
			std::unique_ptr<MemoryBuffer> pointLightIndices;

			std::unique_ptr<MemoryBuffer> pointLightGlobalIndexOffset;

			std::unique_ptr<DescriptorSet> clusterLightCullingDescriptor;

			const glm::uvec3 clusterCount = glm::uvec3(CLUSTERED_TILES_X, CLUSTERED_TILES_Y, CLUSTERED_TILES_Z);
		} m_ClusterSSBOs;

		std::unique_ptr<ComputeShader> m_ClusterAABBCompute;
		std::unique_ptr<ComputeShader> m_ClusterLightCullingCompute;

		std::unique_ptr<Swapchain> m_Swapchain;

		std::unique_ptr<Pipeline> m_DepthPipeline;
		std::unique_ptr<Pipeline> m_ShadowPipeline;
		std::unique_ptr<Pipeline> m_OmniShadowPipeline;

		std::unique_ptr<Pipeline> m_ForwardClusteredPipeline;

		std::unique_ptr<Pipeline> m_SSAOPipeline;

		std::unique_ptr<Pipeline> m_TonemappingPipeline;
		std::unique_ptr<Pipeline> m_AntialiasingPipeline;

		std::unique_ptr<CameraMatricesBuffer> m_CameraMatrices;

		std::unique_ptr<DescriptorSet> m_ForwardClusteredDescriptor;

		std::unique_ptr<DescriptorSet> m_HDRInput;
		std::vector<std::unique_ptr<DescriptorSet>> m_SwapchainInputs;

		std::unique_ptr<Image> m_HDROffscreen;
		std::unique_ptr<Image> m_DepthBuffer;

		VkSampler m_MainSampler;

		std::unique_ptr<Image> m_SSAOTarget;
		std::unique_ptr<MemoryBuffer> m_SSAOBuffer;
		std::unique_ptr<DescriptorSet> m_SSAOInput;

		std::array<VkCommandBuffer, FRAMES_IN_FLIGHT> m_CommandBuffers;

		std::array<VkFence, FRAMES_IN_FLIGHT> m_SubmitFences;

		std::array<VkSemaphore, FRAMES_IN_FLIGHT> m_MainSemaphores;
		std::array<VkSemaphore, FRAMES_IN_FLIGHT> m_PresentSemaphores;
		
		const VkClearValue m_BlackClearValue{};

		// References to existing objects
		Context* m_Ctx		  = nullptr;
		Camera*  m_MainCamera = nullptr;
		Scene*	 m_Scene	  = nullptr;

		uint32_t m_SwapchainImageIndex = 0U; // Current swapchain index
		uint32_t m_FrameIndex = 0U;			 // Frame in flight index
		
		bool m_ReloadQueued = false;
		bool m_FramebufferResized = false;
		bool m_SkipFrame = false;
		bool m_VSync = true;

		static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
		void RecreateFramebuffer();
		void ReloadBackendImpl();

		void CreateCommandBuffer();
		void CreateLightsBuffer();
		void CreateSSAOBuffer();
		void CreateClusterSSBOs();
		void CreateClusterComputePipelines();

		//void UpdateClusterAABBs();

		void CreateDepthBuffer();
		void CreateHDROffscreen();
		void CreateSSAOTarget();

		void UpdateGBufferInput();
		void UpdateHDRInput();
		void UpdateSwapchainInputs();
		void UpdateSSAOInput();

		void InitShadows();
		void RecalculateShadowMatrices(const DirectionalLight& light, DirectionalLight::Buffer& lightBuffer);
		void UpdateShadowFrustums();
		void DestroyShadows();

		void InitDepthPipeline();
		void InitShadowPipeline();
		void InitOmniShadowPipeline();
		void InitForwardClusteredPipeline();
		void InitSSAOPipeline();
		void InitTonemappingPipeline();
		void InitAntialiasingPipeline();

		void CreateSyncObjects();

		void InitImGui();
	};
}

#endif