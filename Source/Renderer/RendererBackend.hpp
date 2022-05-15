#pragma once

#ifndef EN_RENDERERBACKEND_HPP
#define EN_RENDERERBACKEND_HPP

#include "../../EruptionEngine.ini"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <Common/Helpers.hpp>

#include <Renderer/Window.hpp>
#include <Renderer/Context.hpp>
#include <Renderer/Shader.hpp>
#include <Renderer/Lights/PointLight.hpp>
#include <Renderer/Camera/Camera.hpp>
#include <Renderer/PipelineInput.hpp>

#include <Scene/Scene.hpp>

#include <Renderer/Pipeline.hpp>

namespace en
{
	struct RendererInfo
	{
		VkPolygonMode     polygonMode = VK_POLYGON_MODE_FILL;
		VkCullModeFlags   cullMode    = VK_CULL_MODE_BACK_BIT;
	};

	class VulkanRendererBackend
	{
	public:
		~VulkanRendererBackend();

		void Init(RendererInfo& rendererInfo);

		void BindScene(Scene* scene);
		void UnbindScene();
		
		void WaitForGPUIdle();

		void BeginRender();

		void GeometryPass();
		void LightingPass();
		void PostProcessPass();
		void ImGuiPass();

		void EndRender();

		void ReloadBackend();

		void SetMainCamera(Camera* camera);
		Camera* GetMainCamera() { return m_MainCamera; };

		Scene* GetScene() { return m_Scene; };

		std::array<PointLight, MAX_LIGHTS>& GetPointLights();

		int m_DebugMode = 0;
		std::function<void()> m_ImGuiRenderCallback;

	private:
		struct Attachment
		{
			VkImage		   image;
			VkDeviceMemory imageMemory;
			VkImageView    imageView;
			VkFormat	   format;

			void Destroy(VkDevice& device)
			{
				vkFreeMemory(device, imageMemory, nullptr);
				vkDestroyImageView(device, imageView, nullptr);
				vkDestroyImage(device, image, nullptr);
			}
		};

		struct GBuffer
		{
			Attachment albedo, position, normal;
			Attachment depth;

			VkFramebuffer framebuffer;
			VkSampler	  sampler;

			void Destroy(VkDevice& device)
			{
				albedo.Destroy(device);
				position.Destroy(device);
				normal.Destroy(device);
				depth.Destroy(device);

				vkDestroySampler(device, sampler, nullptr);
				vkDestroyFramebuffer(device, framebuffer, nullptr);
			}

		} m_GBuffer;

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
				PointLight::Buffer lights[MAX_LIGHTS];
				glm::vec3 ambientLight;
			} LBO;

			struct LightsCameraInfo
			{
				glm::vec3 viewPos;
				int debugMode;
			} camera;

			std::array<PointLight, MAX_LIGHTS> pointLights;

			VkBuffer       buffer;
			VkDeviceMemory bufferMemory;
			VkDeviceSize   bufferSize;

		} m_Lights;

		struct PostProcessingParams
		{
			struct Exposure 
			{ 
				float value = 1.0f;
			} exposure;
		} m_PostProcessParams;
		
		struct ImGuiVK
		{
			VkDescriptorPool descriptorPool;
			VkRenderPass	 renderPass;

			VkCommandPool commandPool;

			std::vector<VkCommandBuffer> commandBuffers;

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

		//Pipeline m_DepthPipeline;
		std::unique_ptr<Pipeline> m_GeometryPipeline;

		std::unique_ptr<Pipeline> m_TonemappingPipeline;
		std::unique_ptr<PipelineInput> m_TonemappingInput;

		std::unique_ptr<Pipeline> m_LightingPipeline;
		std::unique_ptr<PipelineInput> m_LightingInput;

		Scene* m_Scene = nullptr;

		std::unique_ptr<CameraMatricesBuffer> m_CameraMatrices;

		VkFramebuffer   m_LightingHDRFramebuffer;
		Attachment      m_LightingHDRColorBuffer;
		
		VkCommandBuffer m_CommandBuffer;
		
		VkFence	m_SubmitFence;

		const VkClearValue m_BlackClearValue{};

		RendererInfo m_RendererInfo;

		// References to existing objects
		Context* m_Ctx;
		Window*  m_Window;
		Camera*  m_MainCamera;

		uint32_t m_ImageIndex;

		bool m_FramebufferResized = false;
		bool m_SkipFrame = false;

		static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
		void RecreateFramebuffer();

		void CreateSwapchain();
		void CreateGBufferAttachments();
		void CreateGBuffer();
		void CreateAttachment(Attachment& attachment, VkFormat format, VkImageLayout imageLayout, VkImageAspectFlags imageAspectFlags, VkImageUsageFlags imageUsageFlags);
		
		void InitGeometryPipeline();
		void CreateCommandBuffer();

		void UpdateLightingInput();
		void InitLightingPipeline();
		void LCreateHDRFramebuffer();

		void UpdateTonemappingInput();
		void InitTonemappingPipeline();

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