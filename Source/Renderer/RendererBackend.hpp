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
#include <Renderer/Camera.hpp>
#include <Assets/Model.hpp>

namespace en
{
	struct RendererInfo
	{
		VkClearColorValue clearColor  = { 0.0f, 0.0f, 0.0f, 1.0f };
		VkPolygonMode     polygonMode = VK_POLYGON_MODE_FILL;
		VkCullModeFlags   cullMode    = VK_CULL_MODE_BACK_BIT;
	};

	class VulkanRendererBackend
	{
	public:
		VulkanRendererBackend(){};
		~VulkanRendererBackend();

		void Init(RendererInfo& rendererInfo);

		void PrepareModel(Model* model);
		void RemoveModel(Model* model);
		void EnqueueModel(Model* model);

		void BeginRender();

		void GeometryPass();
		void LightingPass();
		void ImGuiPass(std::function<void()>& imGuiUIDrawFunction);

		void EndRender();

		void ReloadBackend();

		void SetMainCamera(Camera* camera);
		Camera* GetMainCamera();

	private:
		void PrepareMesh(Mesh* mesh, Model* parent);
		void RemoveMesh (Mesh* mesh);
		void EnqueueMesh(Mesh* mesh);

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

		};

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

		};

		struct Pipeline
		{
			VkDescriptorPool	  descriptorPool;
			VkDescriptorSetLayout descriptorSetLayout;

			VkRenderPass	 renderPass;
			VkPipelineLayout layout;
			VkPipeline		 pipeline;

			VkSemaphore passFinished;

			void Destroy(VkDevice& device)
			{
				vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
				vkDestroyDescriptorPool(device, descriptorPool, nullptr);

				vkDestroySemaphore(device, passFinished, nullptr);

				vkDestroyPipeline(device, pipeline, nullptr);
				vkDestroyPipelineLayout(device, layout, nullptr);
				vkDestroyRenderPass(device, renderPass, nullptr);
			}
		};

		struct Lights
		{
			struct LightsBufferObject
			{
				PointLight::Buffer Lights[MAX_LIGHTS];
				glm::vec3 ViewPos;
			} LBO;

			PointLight PointLights[MAX_LIGHTS];

			VkBuffer       Buffer;
			VkDeviceMemory BufferMemory;
			VkDeviceSize   BufferSize;

		} m_Lights;


		GBuffer m_GBuffer;

		Swapchain m_Swapchain;

		Pipeline m_GeometryPipeline;
		Pipeline m_LightingPipeline;

		VkDescriptorSet m_LightingDescriptorSet;

		struct PreparedMesh
		{
			VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
			Model* parent = nullptr;
		};

		std::vector<en::Mesh*> m_MeshQueue;

		std::unordered_map<en::Mesh*, PreparedMesh> m_PreparedMeshes;

		VkCommandBuffer m_CommandBuffer;

		struct ImGuiAPI
		{
			VkDescriptorPool DescriptorPool;
			VkRenderPass	 RenderPass;

			VkCommandPool CommandPool;

			std::vector<VkCommandBuffer> CommandBuffers;
		} m_ImGui;
		

		VkFence	m_SubmitFence;

		RendererInfo m_RendererInfo;

		bool m_IsRendering;
		bool m_SkipFrame = false;

		// References to existing objects
		Context* m_Ctx;
		Window*  m_Window;
		Camera*  m_MainCamera;

		uint32_t m_ImageIndex;

		bool m_FramebufferResized = false;


		static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
		void RecreateFramebuffer();

		void RecordCommandBuffer(VkCommandBuffer& commandBuffer, uint32_t imageIndex);

		void CreateSwapchain();
		void CreateGBufferAttachments();
		void CreateGBuffer();
		void CreateAttachment(Attachment& attachment, VkFormat format, VkImageLayout imageLayout, VkImageAspectFlags imageAspectFlags, VkImageUsageFlags imageUsageFlags);
		
		void InitGeometryPipeline();
		void GCreateDescriptorSetLayout();
		void GCreateDescriptorPool();
		void GCreateCommandBuffer();
		void GCreateRenderPass();
		void GCreatePipeline();

		void InitLightingPipeline();
		void LCreateLightsBuffer();
		void LCreateDescriptorSetLayout();
		void LCreateDescriptorPool();
		void LCreateDescriptorSet();
		void LCreateCommandBuffer();
		void LRecordCommandBuffer();
		void LCreateRenderPass();
		void LCreatePipeline();

		void CreateSwapchainFramebuffers();

		void CreateSyncObjects();

		void InitImGui();

		SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice& device);
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

		VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
		VkFormat FindDepthFormat();
		bool HasStencilComponent(const VkFormat format);

	};
}

#endif