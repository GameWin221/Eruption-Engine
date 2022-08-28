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
//#include <Renderer/ComputeShader.hpp>

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

		void SetVSync(const bool& vSync);

		void WaitForGPUIdle();

		void UpdateLights();

		void BeginRender();

		void ReloadBackend();

		void DepthPass();
		void GeometryPass();
		void ShadowPass();
		void LightingPass();
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

		Scene* GetScene() { return m_Scene; };

		int m_DebugMode = 0;

		std::function<void()> m_ImGuiRenderCallback;

		enum struct AntialiasingMode
		{
			FXAA = 0,
			//SMAA = 1,
			None = 1
		};

		struct PostProcessingParams
		{
			struct Exposure
			{
				float value = 1.0f;
			} exposure;

			AntialiasingMode antialiasingMode = AntialiasingMode::FXAA;

			struct Antialiasing
			{
				float fxaaSpanMax = 4.0f;
				float fxaaReduceMin = 0.001f;
				float fxaaReduceMult = 0.050f;

				float fxaaPower = 1.0f;

				float texelSizeX = 1.0f / 1920.0f;
				float texelSizeY = 1.0f / 1080.0f;
			} antialiasing;

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

				glm::mat4 viewMat = glm::mat4(1.0f);
			} LBO;

			std::unique_ptr<MemoryBuffer> stagingBuffer;
			std::array<std::unique_ptr<MemoryBuffer>, FRAMES_IN_FLIGHT> buffers;

		} m_Lights;

		struct Shadows
		{
			VkSampler sampler;

			const VkFormat shadowFormat = VK_FORMAT_D32_SFLOAT;

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

			} spot;
			struct Dir
			{
				VkImage sharedImage;
				VkDeviceMemory memory;

				VkImageView sharedView;
				std::vector<VkImageView> singleViews;
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

		std::unique_ptr<Swapchain> m_Swapchain;

		std::unique_ptr<Pipeline> m_DepthPipeline;
		std::unique_ptr<Pipeline> m_OmniShadowPipeline;

		std::unique_ptr<Pipeline> m_GeometryPipeline;
		std::unique_ptr<Pipeline> m_LightingPipeline;

		std::unique_ptr<Pipeline> m_TonemappingPipeline;
		std::unique_ptr<Pipeline> m_AntialiasingPipeline;

		std::unique_ptr<CameraMatricesBuffer> m_CameraMatrices;

		std::unique_ptr<DynamicFramebuffer> m_GBuffer;

		std::array<std::unique_ptr<DescriptorSet>, FRAMES_IN_FLIGHT> m_GBufferInputs;
		std::unique_ptr<DescriptorSet> m_HDRInput;
		std::vector<std::unique_ptr<DescriptorSet>> m_SwapchainInputs;

		std::unique_ptr<Image> m_HDROffscreen;

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

		void CreateGBuffer();
		void CreateHDROffscreen();

		void UpdateGBufferInput();
		void UpdateHDRInput();
		void UpdateSwapchainInputs();

		void InitShadows();
		void RecalculateShadowMatrices(const DirectionalLight& light, DirectionalLight::Buffer& lightBuffer);
		void UpdateShadowFrustums();
		void DestroyShadows();

		void InitDepthPipeline();
		void InitOmniShadowPipeline();
		void InitGeometryPipeline();
		void InitLightingPipeline();
		void InitTonemappingPipeline();
		void InitAntialiasingPipeline();

		void CreateSyncObjects();

		void InitImGui();
	};
}

#endif