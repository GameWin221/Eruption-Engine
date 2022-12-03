#pragma once

#ifndef EN_COMPUTESHADER_HPP
#define EN_COMPUTESHADER_HPP

#include <Renderer/Shader.hpp>
#include <Renderer/DescriptorSet.hpp>

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

		void Bind(const VkCommandBuffer cmd);

		void PushConstants(const void* data, uint32_t size, uint32_t offset);

		void BindDescriptorSet(DescriptorSet* descriptor, uint32_t index = 0U);
		void BindDescriptorSet(VkDescriptorSet descriptor, uint32_t index = 0U);

		void Dispatch(const uint32_t x = 1U, const uint32_t y = 1U, const uint32_t z = 1U);

		const std::string m_SourcePath;

		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

	private:
		VkPipeline m_Pipeline = VK_NULL_HANDLE;

		VkCommandBuffer m_BoundCommandBuffer = VK_NULL_HANDLE;
	};
}
#endif