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

		void Dispatch(VkCommandBuffer& cmd, const uint32_t& x = 1U, const uint32_t& y = 1U, const uint32_t& z = 1U);

		const std::string m_SourcePath;

		VkPipelineLayout m_PipelineLayout;

	private:
		VkPipeline m_Pipeline;
	};
}
#endif