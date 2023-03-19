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
		struct ImageInfoContent
		{
			VkImageView   imageView    = VK_NULL_HANDLE;
			VkSampler     imageSampler = VK_NULL_HANDLE;
		};
		struct ImageInfo
		{
			uint32_t index = 0U;
			uint32_t count = 1U;

			std::vector<ImageInfoContent> contents{};

			VkDescriptorType   type		   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			VkShaderStageFlags stage	   = VK_SHADER_STAGE_FRAGMENT_BIT;
			VkImageLayout	   imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			bool operator==(const ImageInfo& other) const
			{
				if (contents.size() != other.contents.size())
					return false;
				
				return imageLayout == other.imageLayout && index == other.index && count == other.count && type == other.type && stage == other.stage;
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
				return index == other.index && type == other.type && stage == other.stage;
			}
		};

		struct Hash
		{
			std::size_t operator()(DescriptorInfo const& info) const noexcept
			{
				size_t resultBuffer = std::hash<size_t>()(info.bufferInfos.size());
				for (const DescriptorInfo::BufferInfo& b : info.bufferInfos)
					resultBuffer ^= std::hash<size_t>()(b.index | b.type << 16 | b.stage << 24);

				size_t resultImage = std::hash<size_t>()(info.bufferInfos.size());
				for (const DescriptorInfo::ImageInfo& i : info.imageInfos)
					resultImage ^= std::hash<size_t>()(i.index | i.type << 8 | i.imageLayout << 16 | i.stage << 24 | i.count << 36);

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

		static DescriptorAllocator& Get();

	private:
		VkDescriptorPool m_DescriptorPool;
		std::unordered_map<DescriptorInfo, VkDescriptorSetLayout, DescriptorInfo::Hash> m_LayoutMap;

		VkDevice m_LogicalDevice;
	};
}


#endif