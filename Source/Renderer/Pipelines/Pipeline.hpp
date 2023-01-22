#pragma once

#ifndef EN_PIPELINE_HPP
#define EN_PIPELINE_HPP

#include <Common/Helpers.hpp>

#include <Renderer/DescriptorSet.hpp>

#include <Core/Types.hpp>

#include <fstream>

namespace en
{
	class Pipeline
	{
	public:
		~Pipeline();

		void PushConstants(const void* data, uint32_t size, uint32_t offset, VkShaderStageFlags shaderStage);

		void BindDescriptorSet(Handle<DescriptorSet> descriptor, uint32_t index = 0U, VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS);
		void BindDescriptorSet(VkDescriptorSet descriptor, uint32_t index = 0U, VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS);

		const VkPipelineLayout GetLayout() const { return m_Layout; }

	protected:
		VkPipelineShaderStageCreateInfo CreateShaderInfo(VkShaderModule shaderModule, VkShaderStageFlagBits stage);

		VkShaderModule CreateShaderModule(const std::string& path);
		void DestroyShaderModule(VkShaderModule shaderModule);

		std::vector<char> ReadShaderFile(const std::string& path);

		VkPipelineLayout m_Layout = VK_NULL_HANDLE;
		VkPipeline		 m_Pipeline = VK_NULL_HANDLE;

		VkCommandBuffer m_BoundCommandBuffer = VK_NULL_HANDLE;
	};
}

#endif