#pragma once

#ifndef EN_GRAPHICS_PIPELINE_HPP
#define EN_GRAPHICS_PIPELINE_HPP

#include <Renderer/Pipelines/Pipeline.hpp>

#include <Renderer/DescriptorSet.hpp>
#include <Renderer/RenderPass.hpp>
#include <Renderer/Buffers/MemoryBuffer.hpp>

namespace en
{
	class GraphicsPipeline : public Pipeline
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
		struct CreateInfo
		{
			Handle<RenderPass> renderPass{};

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

		GraphicsPipeline(const CreateInfo& pipeline);

		void Bind(VkCommandBuffer commandBuffer, VkExtent2D extent, VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT);

		void BindVertexBuffer(Handle<MemoryBuffer> buffer, VkDeviceSize offset = 0U);
		void BindIndexBuffer(Handle<MemoryBuffer> buffer);
		
		void DrawIndexedIndirect(Handle<MemoryBuffer> buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride = sizeof(VkDrawIndexedIndirectCommand));
		void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1U, uint32_t firstIndex = 0U, uint32_t firstInstance = 0U);
		void Draw(uint32_t vertexCount, uint32_t instanceCount = 1U, uint32_t firstVertex = 0U, uint32_t firstInstance = 0U);
	};
}

#endif