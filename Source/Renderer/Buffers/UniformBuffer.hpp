#pragma once

#ifndef EN_UNIFORMBUFFER_HPP
#define EN_UNIFORMBUFFER_HPP

namespace en
{ 
	class UniformBuffer
	{
	public:
		struct ImageInfo
		{
			uint32_t index = 0U;
			VkImageView imageView = VK_NULL_HANDLE;
			VkSampler imageSampler = VK_NULL_HANDLE;
		};
		struct BufferInfo
		{
			uint32_t index = 0U;
			VkBuffer buffer = VK_NULL_HANDLE;
			VkDeviceSize size = 0U;
		};

		UniformBuffer(std::vector<ImageInfo>& imageInfos, BufferInfo bufferInfo = BufferInfo{});
		~UniformBuffer();

		void Bind(VkCommandBuffer& cmd, VkPipelineLayout& layout, uint32_t index = 0U);

		VkDescriptorSetLayout m_DescriptorLayout;

	private:
		void CreateDescriptorPool(std::vector<ImageInfo>& imageInfos, BufferInfo& bufferInfo);
		void CreateDescriptorSet(std::vector<ImageInfo>& imageInfos, BufferInfo& bufferInfo);

		VkDescriptorPool m_DescriptorPool;
		VkDescriptorSet  m_DescriptorSet;
	};
}

#endif