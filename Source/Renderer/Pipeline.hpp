#pragma once

#ifndef EN_PIPELINE_HPP
#define EN_PIPELINE_HPP

#include <Renderer/Shader.hpp>

namespace en
{
	static std::vector<VkClearValue> defaultBlackClearValue{ {0.0f, 0.0f, 0.0f, 1.0f} };

	class Pipeline
	{
	public:
		struct Attachment
		{
			VkFormat format = VK_FORMAT_UNDEFINED;

			VkAttachmentLoadOp  loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;

			VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			VkImageLayout finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			VkImageLayout refLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			uint32_t index = 0U;
		};
		struct RenderPassInfo
		{
			std::vector<Attachment> colorAttachments{};
			Attachment depthAttachment{};
		};
		struct PipelineInfo
		{
			Shader* vShader;
			Shader* fShader;

			VkExtent2D extent;

			std::vector<VkDescriptorSetLayout> descriptorLayouts;
			std::vector<VkPushConstantRange> pushConstantRanges;

			bool useVertexBindings = false;
			bool enableDepthTest = false;
			bool enableDepthWrite = true;
			bool blendEnable = false;

			VkCompareOp compareOp = VK_COMPARE_OP_LESS;

			VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
			VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
		};

		void CreateRenderPass(RenderPassInfo& renderPass);
		void CreatePipeline(PipelineInfo& pipeline);
		void CreateSyncSemaphore();

		~Pipeline();

		void Bind(VkCommandBuffer& commandBuffer, VkFramebuffer& framebuffer, VkExtent2D& extent, std::vector<VkClearValue>& clearValues = defaultBlackClearValue);

		void Unbind(VkCommandBuffer& commandBuffer);

		void Resize(VkExtent2D& extent);

		void Destroy();

		VkRenderPass	 m_RenderPass;
		VkPipelineLayout m_Layout;
		VkPipeline		 m_Pipeline;

		VkSemaphore m_PassFinished;

	private:
		// Cached values
		std::vector<Attachment> m_ColorAttachments;
		Attachment m_DepthAttachment;

		std::string m_VShaderPath;
		std::string m_FShaderPath;

		std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
		std::vector<VkPushConstantRange> m_PushConstantRanges;

		bool m_UseVertexBindings;
		bool m_EnableDepthTest;
		bool m_EnableDepthWrite;
		bool m_BlendEnable;

		VkCompareOp m_CompareOp;

		VkCullModeFlags m_CullMode;
		VkPolygonMode m_PolygonMode;
	};
}

#endif