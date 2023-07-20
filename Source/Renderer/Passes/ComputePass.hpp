#pragma once

#ifndef EN_COMPUTE_PIPELINE_HPP
#define EN_COMPUTE_PIPELINE_HPP

#include <Renderer/Passes/Pass.hpp>

namespace en
{
	class ComputePass : public Pass
	{
	public:
		struct CreateInfo
		{
			std::string sourcePath{};
			std::vector<VkDescriptorSetLayout> descriptorLayouts{};
			std::vector<VkPushConstantRange> pushConstantRanges{};
		};

		ComputePass(const CreateInfo& createInfo);

		void Bind(const VkCommandBuffer cmd);

		void Dispatch(const uint32_t x = 1U, const uint32_t y = 1U, const uint32_t z = 1U);
	};
}
#endif