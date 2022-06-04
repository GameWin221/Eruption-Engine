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

#include <Renderer/Lights/PointLight.hpp>
#include <Renderer/Lights/DirectionalLight.hpp>
#include <Renderer/Lights/Spotlight.hpp>

#include <Renderer/Camera/Camera.hpp>

#include <Renderer/Pipeline.hpp>
#include <Renderer/PipelineInput.hpp>
#include <Renderer/Framebuffer.hpp>

#define BENCHMARK_START cl::BenchmarkBegin("bxbxb");
#define BENCHMARK_RESULT std::cout << "Time ms: " << std::setprecision(12) << std::fixed << cl::BenchmarkStop("bxbxb") * 1000.0 << '\n';

namespace en
{
	class VulkanRendererBackend
	{
	public:
		~VulkanRendererBackend();

		void Init();

		void BindScene(Scene* scene);
		void UnbindScene();

		void SetVSync(bool& vSync);

		void WaitForGPUIdle();

		void UpdateLights();

		void BeginRender();

		void ReloadBackend();

		void DepthPass();
		void GeometryPass();
		void LightingPass();
		void PostProcessPass();
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
		struct Swapchain
		{
			VkSwapchainKHR			   swapchain;
			std::vector<VkImage>	   images;
			std::vector<VkImageView>   imageViews;
			VkFormat				   imageFormat;
			std::vector<VkFramebuffer> framebuffers;
			VkExtent2D				   extent;

			void Destroy(VkDevice& device)
			{
				vkDestroySwapchainKHR(device, swapchain, nullptr);

				for (auto& imageView : imageViews)
					vkDestroyImageView(device, imageView, nullptr);

				for (auto& framebuffer : framebuffers)
					vkDestroyFramebuffer(device, framebuffer, nullptr);
			}

		} m_Swapchain;

		struct Lights
		{
			struct LightsBufferObject
			{
				PointLight::Buffer		 pointLights[MAX_POINT_LIGHTS];
				Spotlight::Buffer		 spotLights[MAX_SPOT_LIGHTS];
				DirectionalLight::Buffer dirLights[MAX_DIR_LIGHTS];

				alignas(4) uint32_t activePointLights = 0U;
				alignas(4) uint32_t activeSpotlights = 0U;
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
			uint32_t lastSpotlightsSize = 0U;
			uint32_t lastDirLightsSize = 0U;

		} m_Lights;

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

		std::unique_ptr<Pipeline> m_DepthPipeline;

		std::unique_ptr<Pipeline> m_GeometryPipeline;

		std::unique_ptr<Pipeline> m_LightingPipeline;
		std::unique_ptr<PipelineInput> m_LightingInput;

		std::unique_ptr<Pipeline> m_TonemappingPipeline;
		std::unique_ptr<PipelineInput> m_TonemappingInput;

		std::unique_ptr<Pipeline> m_AntialiasingPipeline;
		std::unique_ptr<PipelineInput> m_AntialiasingInput;

		std::unique_ptr<CameraMatricesBuffer> m_CameraMatrices;

		std::unique_ptr<Framebuffer> m_DepthBuffer;
		std::unique_ptr<Framebuffer> m_GBuffer;
		std::unique_ptr<Framebuffer> m_HDRFramebuffer;
		std::unique_ptr<Framebuffer> m_AliasedFramebuffer;

		VkCommandBuffer m_CommandBuffer;

		VkFence	m_SubmitFence;

		const VkClearValue m_BlackClearValue{};

		// References to existing objects
		Context* m_Ctx;
		Camera* m_MainCamera;

		Scene* m_Scene = nullptr;

		uint32_t m_ImageIndex = 0U;

		bool m_ReloadQueued = false;
		bool m_FramebufferResized = false;
		bool m_SkipFrame = false;
		bool m_VSync = true;

		bool m_LightsChanged = false;

		static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
		void RecreateFramebuffer();

		void ReloadBackendImpl();

		void CreateSwapchain();

		void CreateCommandBuffer();

		void InitDepthPipeline();

		void CreateDepthBufferHandle();
		void CreateDepthBufferAttachments();

		void InitGeometryPipeline();

		void CreateGBufferHandle();
		void CreateGBufferAttachments();

		void UpdateLightingInput();
		void InitLightingPipeline();
		void CreateLightingHDRFramebuffer();

		void UpdateTonemappingInput();
		void InitTonemappingPipeline();

		void UpdateAntialiasingInput();
		void InitAntialiasingPipeline();
		void CreateAliasedFramebuffer();

		void CreateSwapchainFramebuffers();

		void CreateSyncObjects();

		void CreateCameraMatricesBuffer();

		void InitImGui();

		SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice& device);
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

		VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
		VkFormat FindDepthFormat();
		bool HasStencilComponent(const VkFormat format);

	};
}

#endif