#pragma once

#ifndef EN_PIPELINE_HPP
#define EN_PIPELINE_HPP

#include <Renderer/Shader.hpp>

namespace en
{
	class Pipeline
	{
	public:
		struct Attachment
		{
			VkImageView   imageView   = VK_NULL_HANDLE;
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentLoadOp  loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;

			VkClearValue clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };
		};
		struct RenderingInfo
		{
			std::vector<Attachment> colorAttachments{};
			Attachment depthAttachment{};

			VkExtent2D extent{};
		};
		struct PipelineInfo
		{
			std::vector<VkFormat> colorFormats{};
			VkFormat depthFormat{};

			Shader* vShader;
			Shader* fShader;

			std::vector<VkDescriptorSetLayout> descriptorLayouts{};
			std::vector<VkPushConstantRange> pushConstantRanges{};

			bool useVertexBindings = false;
			bool enableDepthTest = false;
			bool enableDepthWrite = true;
			bool blendEnable = false;

			VkCompareOp compareOp = VK_COMPARE_OP_LESS;

			VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
			VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
		};

		void CreatePipeline(PipelineInfo& pipeline);
		void CreateSyncSemaphore();

		~Pipeline();

		void Bind(VkCommandBuffer& commandBuffer, RenderingInfo& info);

		void Unbind(VkCommandBuffer& commandBuffer);

		void Destroy();

		PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR;
		PFN_vkCmdEndRenderingKHR vkCmdEndRenderingKHR;

		VkPipelineLayout m_Layout;
		VkPipeline		 m_Pipeline;

		VkSemaphore m_PassFinished;

	private:
		std::string m_VShaderPath;
		std::string m_FShaderPath;

		bool m_Initialised = false;

		PipelineInfo m_LastInfo{};
	};
}

#endif