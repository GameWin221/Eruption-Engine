#include "Core/EnPch.hpp"
#include "DynamicFramebuffer.hpp"

#include <Renderer/Context.hpp>
#include <Common/Helpers.hpp>

namespace en
{
	DynamicFramebuffer::DynamicFramebuffer(const std::initializer_list<AttachmentInfo>& attachmentInfos, const VkExtent2D& size, const VkFilter& filtering) : m_Size(size)
	{
		// Prevent reallocations
		m_Attachments.reserve(attachmentInfos.size());

		for (const auto& info : attachmentInfos)
			m_Attachments.emplace_back(m_Size, info.format, info.imageUsageFlags, info.imageAspectFlags, info.initialLayout, false);

		Helpers::CreateSampler(m_Sampler, filtering);
	}

	DynamicFramebuffer::~DynamicFramebuffer()
	{
		vkDestroySampler(Context::Get().m_LogicalDevice, m_Sampler, nullptr);

		m_Attachments.clear();
	}
}