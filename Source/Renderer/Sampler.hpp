#pragma once
#ifndef EN_SAMPLER_HPP
#define EN_SAMPLER_HPP

#include <Renderer/Context.hpp>

namespace en
{
	class Sampler
	{
	public:
		Sampler(VkFilter filter, uint32_t anisotropy = 0U, float maxLod = 0.0f, float mipLodBias = 0.0f);
		~Sampler();

		const VkSampler GetHandle() const { return m_Handle; }

	private:
		VkSampler m_Handle = VK_NULL_HANDLE;
	};
}


#endif