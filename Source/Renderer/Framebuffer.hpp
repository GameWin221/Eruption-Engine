#pragma once
#ifndef EN_FRAMEBUFFER_HPP
#define EN_FRAMEBUFFER_HPP

#include <Renderer/Context.hpp>
#include <Renderer/RenderPass.hpp>

namespace en
{
	class Framebuffer
	{
	public:
		Framebuffer(
			Handle<RenderPass> renderPass,
			const VkExtent2D extent,
			const RenderPassAttachment& colorAttachment = {},
			const RenderPassAttachment& depthAttachment = {}
		);
		~Framebuffer();
		
		const VkFramebuffer GetHandle() const { return m_Handle; }

		const VkExtent2D m_Extent{};
		const RenderPassAttachment m_ColorAttachment;
		const RenderPassAttachment m_DepthAttachment;

	private:
		VkFramebuffer m_Handle{};
	};
}

#endif