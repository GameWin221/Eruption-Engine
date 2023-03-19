#include "DescriptorSet.hpp"

#include <Common/Helpers.hpp>

namespace en
{

	DescriptorSet::DescriptorSet(const DescriptorInfo& info)
	{
		m_DescriptorLayout = DescriptorAllocator::Get().MakeLayout(info);
		m_DescriptorSet = DescriptorAllocator::Get().MakeSet(info);

		Update(info);
	}
	DescriptorSet::~DescriptorSet()
	{
		UseContext();
		vkFreeDescriptorSets(ctx.m_LogicalDevice, DescriptorAllocator::Get().GetPool(), 1U, &m_DescriptorSet);
	}
	
	void DescriptorSet::Update(const DescriptorInfo& info)
	{
		UseContext();

		std::vector<std::vector<VkDescriptorImageInfo>> descriptorImageInfos;
		for (const auto& image : info.imageInfos)
		{
			std::vector<VkDescriptorImageInfo> infos{};

			for (const auto& content : image.contents)
			{
				infos.emplace_back(VkDescriptorImageInfo {
					.sampler     = content.imageSampler,
					.imageView   = content.imageView,
					.imageLayout = image.imageLayout
				});
			}

			descriptorImageInfos.emplace_back(infos);
		}

		std::vector<VkDescriptorBufferInfo> descriptorBufferInfos;
		for (const auto& buffer : info.bufferInfos)
		{
			VkDescriptorBufferInfo info{
				.buffer = buffer.buffer,
				.offset = 0U,
				.range  = buffer.size
			};

			descriptorBufferInfos.emplace_back(info);
		}

		std::vector<VkWriteDescriptorSet> descriptorWrites(info.imageInfos.size() + info.bufferInfos.size());
		for (uint32_t i = 0; const auto & image : info.imageInfos)
		{
			descriptorWrites[image.index] = VkWriteDescriptorSet {
				.sType			 = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet			 = m_DescriptorSet,
				.dstBinding		 = image.index,
				.dstArrayElement = 0U,
				.descriptorCount = image.count,
				.descriptorType  = image.type,
				.pImageInfo		 = descriptorImageInfos[i++].data()
			};
		}
		for (uint32_t i = 0; const auto & buffer : info.bufferInfos)
		{
			descriptorWrites[buffer.index] = VkWriteDescriptorSet {
				.sType			 = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet			 = m_DescriptorSet,
				.dstBinding		 = buffer.index,
				.dstArrayElement = 0U,
				.descriptorCount = 1U,
				.descriptorType  = buffer.type,
				.pBufferInfo	 = &descriptorBufferInfos[i++]
			};
		}

		vkUpdateDescriptorSets(ctx.m_LogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0U, nullptr);
	}
}