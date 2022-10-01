#pragma once

#ifndef EN_PIPELINE_HPP
#define EN_PIPELINE_HPP

#include <Renderer/Shader.hpp>
#include <Renderer/DescriptorSet.hpp>
#include <Renderer/Buffers/VertexBuffer.hpp>
#include <Renderer/Buffers/IndexBuffer.hpp>
#include <Assets/Material.hpp>

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

			std::string vShader = "";
			std::string fShader = "";

			std::vector<VkDescriptorSetLayout> descriptorLayouts{};
			std::vector<VkPushConstantRange> pushConstantRanges{};

			bool useVertexBindings = false;
			bool enableDepthTest   = false;
			bool enableDepthWrite  = true;
			bool blendEnable	   = false;

			VkCompareOp	  compareOp	  = VK_COMPARE_OP_LESS;
			VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
		};

		Pipeline(const CreateInfo& pipeline);
		~Pipeline();

		void BeginRendering(VkCommandBuffer commandBuffer, const BindInfo& info);

		void PushConstants(const void* data, uint32_t size, uint32_t offset, VkShaderStageFlags shaderStage);

		void BindDescriptorSet(DescriptorSet* descriptor, uint32_t index = 0U);
		void BindDescriptorSet(VkDescriptorSet descriptor, uint32_t index = 0U);
		void BindVertexBuffer(VertexBuffer* buffer);
		void BindIndexBuffer(IndexBuffer* buffer);

		void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1U, uint32_t firstIndex = 0U, uint32_t firstInstance = 0U);
		void Draw(uint32_t vertexCount, uint32_t instanceCount = 1U, uint32_t firstVertex = 0U, uint32_t firstInstance = 0U);

		void EndRendering();

		VkPipelineLayout m_Layout;
		VkPipeline		 m_Pipeline;

	private:
		
		VkCommandBuffer m_RenderingCommandBuffer;
	};
}

#endif