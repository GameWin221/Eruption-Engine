#pragma once

#ifndef EN_COMPUTE_PIPELINE_HPP
#define EN_COMPUTE_PIPELINE_HPP

#include <Renderer/Pipelines/Pipeline.hpp>

namespace en
{
	class ComputePipeline : public Pipeline
	{
	public:
		struct CreateInfo
		{
			std::string sourcePath{};
			std::vector<VkDescriptorSetLayout> descriptorLayouts{};
			std::vector<VkPushConstantRange> pushConstantRanges{};
		};

		ComputePipeline(const CreateInfo& createInfo);

		void Bind(const VkCommandBuffer cmd);

		void Dispatch(const uint32_t x = 1U, const uint32_t y = 1U, const uint32_t z = 1U);
	};
}
#endif