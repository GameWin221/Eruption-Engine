#pragma once

#ifndef EN_VULKANBACKEND_HPP
#define EN_VULKANBACKEND_HPP

#include "Window.hpp"
#include "Context.hpp"
#include "Camera.hpp"
#include "Model.hpp"

namespace en
{
	struct RendererInfo
	{
		VkClearColorValue clearColor  = { 0.0f, 0.0f, 0.0f, 1.0f };
		VkPolygonMode     polygonMode = VK_POLYGON_MODE_FILL;
		VkCullModeFlags   cullMode    = VK_CULL_MODE_BACK_BIT;
		uint32_t		  maxMeshes   = 64;
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

		void EndRender();

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

			VkFramebuffer         framebuffer;
			VkSampler			  sampler;

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
			VkSwapchainKHR			   swapChain;
			std::vector<VkImage>	   images;
			std::vector<VkImageView>   imageViews;
			VkFormat				   imageFormat;
			std::vector<VkFramebuffer> framebuffers;
			VkExtent2D				   extent;

			void Destroy(VkDevice& device)
			{
				vkDestroySwapchainKHR(device, swapChain, nullptr);

				for (auto& imageView : imageViews)
					vkDestroyImageView(device, imageView, nullptr);

				for (auto& image : images)
					vkDestroyImage(device, image, nullptr);

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

		VkFence	m_InFlightFence;

		RendererInfo m_RendererInfo;

		// References to existing objects
		Context* m_Ctx;
		Window* m_Window;
		Camera* m_MainCamera;

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
		void LCreateDescriptorSetLayout();
		void LCreateDescriptorPool();
		void LCreateDescriptorSet();
		void LCreateCommandBuffer();
		void LRecordCommandBuffer();
		void LCreateRenderPass();
		void LCreatePipeline();

		void CreateSwapchainFramebuffers();

		void CreateSyncObjects();

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