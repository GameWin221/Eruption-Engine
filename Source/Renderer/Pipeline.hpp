#pragma once

#ifndef EN_PIPELINE_HPP
#define EN_PIPELINE_HPP

#include <Renderer/Shader.hpp>
#include <Renderer/RenderPass.hpp>

namespace en
{

	class Pipeline
	{
	public:
		struct PipelineInfo
		{
			Shader* vShader = nullptr;
			Shader* fShader = nullptr;

			RenderPass* renderPass = nullptr;

			uint32_t subpassIndex = 0U;

			VkExtent2D extent;

			std::vector<VkDescriptorSetLayout> descriptorLayouts;
			std::vector<VkPushConstantRange> pushConstantRanges;

			bool useVertexBindings = false;
			bool enableDepthTest  = false;
			bool enableDepthWrite = true;
			bool blendEnable = false;

			VkCompareOp compareOp = VK_COMPARE_OP_LESS;

			VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
			VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
		};

		void CreatePipeline(PipelineInfo& pipeline);

		~Pipeline();

		void Bind(VkCommandBuffer& commandBuffer);

		void Resize(VkExtent2D& extent);
		
		void Destroy();

		VkPipelineLayout m_Layout;
		VkPipeline		 m_Pipeline;

	private:
		std::string m_VShaderPath;
		std::string m_FShaderPath;

		PipelineInfo m_PipelineInfo;
	};
}

#endif