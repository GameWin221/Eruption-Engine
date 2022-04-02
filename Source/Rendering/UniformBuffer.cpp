#include "EnPch.hpp"
#include "Rendering/UniformBuffer.hpp"

namespace en
{
	UniformBuffer::UniformBuffer()
	{
		m_BufferSize = sizeof(UniformBufferObject);

		en::Helpers::CreateBuffer(m_BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_Buffer, m_BufferMemory);
	}
	UniformBuffer::~UniformBuffer()
	{
		en::Helpers::DestroyBuffer(m_Buffer, m_BufferMemory);
	}

	void UniformBuffer::Bind()
	{
		en::Helpers::MapBuffer(m_BufferMemory, &m_UBO, m_BufferSize);
	}
}