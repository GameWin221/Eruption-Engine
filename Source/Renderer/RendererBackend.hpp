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

#define BENCHMARK_START cl::BenchmarkBegin("bxbxb");
#define BENCHMARK_RESULT std::cout << "Time ms: " << std::setprecision(12) << std::fixed << cl::BenchmarkStop("bxbxb") * 1000.0 << '\n';

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

		Scene* GetScene() { return m_Scene; };

		int m_DebugMode = 0;

		std::function<void()> m_ImGuiRenderCallback;

		enum struct AntialiasingMode : int
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

				alignas(4) uint32_t activePointLights = 0U;
				alignas(4) uint32_t activeSpotLights = 0U;
				alignas(4) uint32_t activeDirLights = 0U;

				alignas(16) glm::vec3 ambientLight = glm::vec3(0.0f);
			} LBO;

			struct LightsCameraInfo
			{
				glm::vec3 viewPos = glm::vec3(0.0f);
				int debugMode = 0;
			} camera;



			std::unique_ptr<MemoryBuffer> buffer;

			uint32_t lastPointLightsSize = 0U;
			uint32_t lastSpotLightsSize = 0U;
			uint32_t lastDirLightsSize = 0U;

			bool changed = false;

		} m_Lights;

		struct Shadows
		{
			VkImage image;
			VkDeviceMemory memory;

			VkSampler sampler;

			std::vector<VkImageView> pointShadowmaps;
			std::vector<VkImageView> spotShadowmaps;
			std::vector<VkImageView> dirShadowmaps;

			VkImageView pointShadowmapView;
			VkImageView spotShadowmapView;
			VkImageView dirShadowmapView;

			const VkFormat shadowFormat = VK_FORMAT_D32_SFLOAT;

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
		std::unique_ptr<Pipeline> m_GeometryPipeline;
		std::unique_ptr<Pipeline> m_LightingPipeline;

		std::unique_ptr<Pipeline> m_TonemappingPipeline;
		std::unique_ptr<Pipeline> m_AntialiasingPipeline;

		std::unique_ptr<CameraMatricesBuffer> m_CameraMatrices;

		std::unique_ptr<DynamicFramebuffer> m_GBuffer;

		std::unique_ptr<DescriptorSet> m_GBufferInput;
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

		uint32_t m_SwapchainImageIndex = 0U;
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

		void InitDepthPipeline();
		void InitGeometryPipeline();
		void InitLightingPipeline();
		void InitTonemappingPipeline();
		void InitAntialiasingPipeline();

		void CreateSyncObjects();

		void InitImGui();
	};
}

#endif