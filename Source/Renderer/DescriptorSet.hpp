#pragma once

#ifndef EN_DESCRIPTORSET_HPP
#define EN_DESCRIPTORSET_HPP

#include <Renderer/Context.hpp>
#include <vector>

namespace en
{ 
	class DescriptorSet
	{
	public:
		struct ImageInfo
		{
			uint32_t      index		   = 0U;
			VkImageView   imageView	   = VK_NULL_HANDLE;
			VkSampler     imageSampler = VK_NULL_HANDLE;
			VkImageLayout imageLayout  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkDescriptorType	  type  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			VkShaderStageFlags stage = VK_SHADER_STAGE_FRAGMENT_BIT;

			uint32_t count = 1U;
		};
		struct BufferInfo
		{
			uint32_t	 index  = 0U;
			VkBuffer	 buffer = VK_NULL_HANDLE;
			VkDeviceSize size	= 0U;

			VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

			VkShaderStageFlags stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		};

		DescriptorSet(const std::vector<ImageInfo>& imageInfos, const std::vector<BufferInfo>& bufferInfos);
		~DescriptorSet();

		const VkDescriptorSet GetHandle() const { return m_DescriptorSet; }

		void Update(const std::vector<ImageInfo>& imageInfos, const std::vector<BufferInfo>& bufferInfos);

		const VkDescriptorSetLayout GetLayout() const { return m_DescriptorLayout; }

	private:
		void CreateDescriptorPool(const std::vector<ImageInfo>& imageInfos, const std::vector<BufferInfo>& bufferInfos);
		void CreateDescriptorSet();

		VkDescriptorSetLayout m_DescriptorLayout;
		VkDescriptorSet  m_DescriptorSet;
		VkDescriptorPool m_DescriptorPool;
	};
}

#endif