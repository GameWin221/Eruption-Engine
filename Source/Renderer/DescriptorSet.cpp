#include "DescriptorSet.hpp"

#include <Common/Helpers.hpp>

namespace en
{

	DescriptorSet::DescriptorSet(const DescriptorInfo& info)
	{
		UseContext();

		m_DescriptorLayout = ctx.m_DescriptorAllocator->MakeLayout(info);
		m_DescriptorSet = ctx.m_DescriptorAllocator->MakeSet(info);

		Update(info);
	}
	DescriptorSet::~DescriptorSet()
	{
		UseContext();
		vkFreeDescriptorSets(ctx.m_LogicalDevice, ctx.m_DescriptorAllocator->GetPool(), 1U, &m_DescriptorSet);
	}
	
	void DescriptorSet::Update(const DescriptorInfo& info)
	{
		UseContext();

		std::vector<VkDescriptorImageInfo> descriptorImageInfos;
		for (const auto& image : info.imageInfos)
		{
			VkDescriptorImageInfo info{
				.sampler	 = image.imageSampler,
				.imageView   = image.imageView,
				.imageLayout = image.imageLayout
			};

			descriptorImageInfos.emplace_back(info);
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
				.descriptorCount = 1U,
				.descriptorType  = image.type,
				.pImageInfo		 = &descriptorImageInfos[i++]
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