#pragma once

#ifndef EN_DESCRIPTORSET_HPP
#define EN_DESCRIPTORSET_HPP

#include <Renderer/DescriptorAllocator.hpp>
#include <vector>

namespace en
{ 
	class DescriptorSet
	{
	public:
		DescriptorSet(const DescriptorInfo& info);
		~DescriptorSet();

		void Update(const DescriptorInfo& info);

		const VkDescriptorSet	    GetHandle() const { return m_DescriptorSet;    }
		const VkDescriptorSetLayout GetLayout() const { return m_DescriptorLayout; }

	private:
		VkDescriptorSetLayout m_DescriptorLayout;
		VkDescriptorSet		  m_DescriptorSet;
	};
}

#endif