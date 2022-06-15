#pragma once

#ifndef EN_PIPELINEINPUT_HPP
#define EN_PIPELINEINPUT_HPP

namespace en
{ 
	// It is possible that the buffer always has to be the last index

	class PipelineInput
	{
	public:
		struct ImageInfo
		{
			uint32_t    index		 = 0U;
			VkImageView imageView	 = VK_NULL_HANDLE;
			VkSampler   imageSampler = VK_NULL_HANDLE;
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		};
		struct BufferInfo
		{
			uint32_t	 index  = 0U;
			VkBuffer	 buffer = VK_NULL_HANDLE;
			VkDeviceSize size	= 0U;
		};

		PipelineInput(std::vector<ImageInfo> imageInfos, BufferInfo bufferInfo = BufferInfo{});
		~PipelineInput();

		void Bind(VkCommandBuffer& cmd, VkPipelineLayout& layout, uint32_t index = 0U);

		void UpdateDescriptorSet(std::vector<ImageInfo> imageInfos, BufferInfo bufferInfo = BufferInfo{});

		VkDescriptorSetLayout m_DescriptorLayout;

	private:
		void CreateDescriptorPool(std::vector<ImageInfo>& imageInfos, BufferInfo& bufferInfo);
		void CreateDescriptorSet();

		VkDescriptorPool m_DescriptorPool;
		VkDescriptorSet  m_DescriptorSet;
	};
}

#endif