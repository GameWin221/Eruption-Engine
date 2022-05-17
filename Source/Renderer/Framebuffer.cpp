#include "Core/EnPch.hpp"
#include "Framebuffer.hpp"

#include <Renderer/Context.hpp>

namespace en
{
	void Framebuffer::Attachment::Destroy()
	{
		UseContext();

		vkFreeMemory(ctx.m_LogicalDevice, imageMemory, nullptr);
		vkDestroyImageView(ctx.m_LogicalDevice, imageView, nullptr);
		vkDestroyImage(ctx.m_LogicalDevice, image, nullptr);
	}
}