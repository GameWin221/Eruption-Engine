#pragma once

#ifndef EN_RENDERER_HPP
#define EN_RENDERER_HPP

#include "Context.hpp"

#include "Shader.hpp"
#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"
#include "UniformBuffer.hpp"
#include "Texture.hpp"
#include "Mesh.hpp"
#include "Model.hpp"

#include "Utility/Helpers.hpp"

#include "Camera.hpp"

namespace en
{
	struct RendererInfo 
	{
		VkClearColorValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
		VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
		VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
	};
	struct MeshData
	{
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		Model* parent = nullptr;
	};

	class Renderer
	{
	public:
		Renderer(RendererInfo& rendererInfo);
		~Renderer();

		void PrepareModel(Model* model);
		void RemoveModel (Model* model);
		void EnqueueModel(Model* model);

		void PrepareMesh(Mesh* mesh, Model* parent);
		void RemoveMesh (Mesh* mesh);
		void EnqueueMesh(Mesh* mesh);

		void Render();

		void SetMainCamera(Camera* camera);
		Camera* GetMainCamera() { return m_MainCamera; };

		static const int m_MaxMeshes = 64;

		Renderer& GetRenderer();

	private:
		VkRenderPass	 m_RenderPass;
		VkPipelineLayout m_PipelineLayout;
		VkPipeline		 m_GraphicsPipeline;

		VkSwapchainKHR			   m_SwapChain;
		std::vector<VkImage>	   m_SwapChainImages;
		VkFormat				   m_SwapChainImageFormat;
		VkExtent2D				   m_SwapChainExtent;
		std::vector<VkImageView>   m_SwapChainImageViews;
		std::vector<VkFramebuffer> m_Framebuffers;

		VkCommandBuffer m_CommandBuffer;

		VkDescriptorPool	  m_DescriptorPool;
		VkDescriptorSetLayout m_DescriptorSetLayout;

		VkImage		   m_DepthImage;
		VkDeviceMemory m_DepthImageMemory;
		VkImageView    m_DepthImageView;

		VkSemaphore m_ImageAvailableSemaphore;
		VkSemaphore m_RenderFinishedSemaphore;
		VkFence		m_InFlightFence;

		// References to existing objects
		Context* m_Ctx;
		Window*  m_Window;
		Camera*  m_MainCamera;

		std::vector<en::Mesh*> m_MeshQueue;
		std::unordered_map<en::Mesh*, MeshData> m_MeshData;

		bool m_FramebufferResized = false;

		RendererInfo m_RendererInfo;

		void RecreateFramebuffer();
		static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

		void RecordCommandBuffer(VkCommandBuffer& commandBuffer, uint32_t imageIndex);

		SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice& device);
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

		VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
		VkFormat FindDepthFormat();
		bool HasStencilComponent(const VkFormat format);

		void VKCreateCommandBuffer();
		void VKCreateSwapChain();
		void VKCreateImageViews();
		void VKCreateRenderPass();
		void VKCreateGraphicsPipeline();
		void VKCreateFramebuffers();
		void VKCreateDepthBuffer();
		void VKCreateSyncObjects();
		void VKCreateDescriptorSetLayout();
		void VKCreateDescriptorPool();
		void VKCreateDescriptorSets();
		void VKCreateBuffers();
	};
}

#endif