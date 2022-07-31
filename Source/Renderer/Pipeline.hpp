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
		struct BindInfo
		{
			std::vector<Attachment> colorAttachments{};
			Attachment depthAttachment{};

			VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;

			VkExtent2D extent{};
		};
		struct CreateInfo
		{
			std::vector<VkFormat> colorFormats{};
			VkFormat depthFormat{};

			Shader* vShader = nullptr;
			Shader* fShader = nullptr;

			std::vector<VkDescriptorSetLayout> descriptorLayouts{};
			std::vector<VkPushConstantRange> pushConstantRanges{};

			bool useVertexBindings = false;
			bool enableDepthTest = false;
			bool enableDepthWrite = true;
			bool blendEnable = false;

			VkCompareOp compareOp = VK_COMPARE_OP_LESS;

			VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
		};

		void CreatePipeline(const CreateInfo& pipeline);

		~Pipeline();

		void Bind(VkCommandBuffer& commandBuffer, const BindInfo& info);

		void Unbind(VkCommandBuffer& commandBuffer);

		void Destroy();

		VkPipelineLayout m_Layout;
		VkPipeline		 m_Pipeline;

	private:
		std::string m_VShaderPath;
		std::string m_FShaderPath;
	};
}

#endif