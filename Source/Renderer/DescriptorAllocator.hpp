#pragma once
#ifndef EN_DESCRIPTORALLOCATOR_HPP
#define EN_DESCRIPTORALLOCATOR_HPP

#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <functional>
#include <unordered_map>

namespace en
{
	struct DescriptorInfo 
	{
		struct ImageInfo
		{
			uint32_t      index = 0U;
			VkImageView   imageView = VK_NULL_HANDLE;
			VkSampler     imageSampler = VK_NULL_HANDLE;
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkDescriptorType   type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			VkShaderStageFlags stage = VK_SHADER_STAGE_FRAGMENT_BIT;

			uint32_t count = 1U;

			bool operator==(const ImageInfo& other) const
			{
				return index == other.index && imageView == other.imageView && imageSampler == other.imageSampler && imageLayout == other.imageLayout && type == other.type && stage == other.stage && count == other.count;
			}
		};
		struct BufferInfo
		{
			uint32_t	 index = 0U;
			VkBuffer	 buffer = VK_NULL_HANDLE;
			VkDeviceSize size = 0U;

			VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

			VkShaderStageFlags stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		
			bool operator==(const BufferInfo& other) const
			{
				return index == other.index && buffer == other.buffer && size == other.size && type == other.type && stage == other.stage;
			}
		};

		struct Hash
		{
			std::size_t operator()(en::DescriptorInfo const& info) const noexcept
			{
				size_t resultBuffer = std::hash<size_t>()(info.bufferInfos.size());
				for (const en::DescriptorInfo::BufferInfo& b : info.bufferInfos)
					resultBuffer ^= std::hash<size_t>()(b.index | b.type << 8 | b.size << 16 | b.stage << 24);

				size_t resultImage = std::hash<size_t>()(info.bufferInfos.size());
				for (const en::DescriptorInfo::ImageInfo& i : info.imageInfos)
					resultImage ^= std::hash<size_t>()(i.index | i.type << 8 | i.imageLayout << 16 | i.stage << 24);

				return resultBuffer ^ (resultImage << 1);
			}
		};

		bool operator==(const DescriptorInfo& other) const
		{
			if (imageInfos.size() != other.imageInfos.size() || bufferInfos.size() != other.bufferInfos.size())
				return false;

			for (uint32_t i = 0U; i < imageInfos.size(); i++)
				if (imageInfos[i] != other.imageInfos[i])
					return false;

			for (uint32_t i = 0U; i < bufferInfos.size(); i++)
				if (bufferInfos[i] != other.bufferInfos[i])
					return false;

			return true;
		}

		std::vector<ImageInfo> imageInfos;
		std::vector<BufferInfo> bufferInfos;
	};

	class DescriptorAllocator
	{
	public:
		DescriptorAllocator(VkDevice logicalDevice);
		~DescriptorAllocator();

		VkDescriptorSetLayout MakeLayout(const DescriptorInfo& descriptorInfo);
		VkDescriptorSet		  MakeSet(const DescriptorInfo& descriptorInfo);

		const VkDescriptorPool GetPool() const { return m_DescriptorPool; }

	private:
		VkDescriptorPool m_DescriptorPool;
		std::unordered_map<DescriptorInfo, VkDescriptorSetLayout, DescriptorInfo::Hash> m_LayoutMap;

		VkDevice m_LogicalDevice;
	};
}


#endif