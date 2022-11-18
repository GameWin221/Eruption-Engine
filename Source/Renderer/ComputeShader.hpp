#pragma once

#ifndef EN_COMPUTESHADER_HPP
#define EN_COMPUTESHADER_HPP

#include <Renderer/Shader.hpp>

namespace en
{
	class ComputeShader
	{
	public:
		struct CreateInfo
		{
			std::string sourcePath{};
			std::vector<VkDescriptorSetLayout> descriptorLayouts{};
			std::vector<VkPushConstantRange> pushConstantRanges{};
		};

		ComputeShader(const CreateInfo& createInfo);
		~ComputeShader();

		void Bind(VkCommandBuffer& cmd);

		void Dispatch(const uint32_t x = 1U, const uint32_t y = 1U, const uint32_t z = 1U);

		const std::string m_SourcePath;

		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

	private:
		VkPipeline m_Pipeline = VK_NULL_HANDLE;

		VkCommandBuffer m_BoundCommandBuffer = VK_NULL_HANDLE;
	};
}
#endif