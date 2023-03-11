#pragma once

#ifndef EN_RENDERER_HPP
#define EN_RENDERER_HPP

#include "../../EruptionEngine.ini"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <Common/Helpers.hpp>

#include <Scene/Scene.hpp>

#include <Renderer/Window.hpp>
#include <Renderer/Context.hpp>
#include <Renderer/Pipelines/Pipeline.hpp>
#include <Renderer/Pipelines/GraphicsPipeline.hpp>
#include <Renderer/Pipelines/ComputePipeline.hpp>

#include <Renderer/RenderPass.hpp>

#include <Renderer/Lights/PointLight.hpp>
#include <Renderer/Lights/DirectionalLight.hpp>
#include <Renderer/Lights/SpotLight.hpp>

#include <Renderer/Camera/Camera.hpp>
#include <Renderer/Camera/CameraBuffer.hpp>

#include <Renderer/DescriptorSet.hpp>

#include <functional>
#include <random>

#include <Renderer/Swapchain.hpp>

namespace en
{
	class Renderer
	{
	public:
		Renderer();
		~Renderer();

		void BindScene(en::Handle<Scene> scene);
		void UnbindScene();

		void SetVSync(bool vSync);

		void WaitForGPUIdle();

		void ReloadBackend();

		void PreRender();
		void Render();

		en::Handle<Scene> GetScene() { return m_Scene; };

		const double GetFrameTime() const { return m_FrameTime; }

		int m_DebugMode = 0;

		std::function<void()> m_ImGuiRenderCallback;

	private:
		Handle<Swapchain> m_Swapchain;
		Handle<CameraBuffer> m_CameraBuffer;

		VkSampler m_MainSampler;

		Handle<RenderPass> m_ImGuiRenderPass;
		Handle<RenderPass> m_RenderPass;

		Handle<GraphicsPipeline> m_Pipeline;

		Handle<Scene> m_Scene;

		struct Frame {
			VkCommandBuffer commandBuffer;
			VkFence submitFence;

			VkSemaphore mainSemaphore;
			VkSemaphore presentSemaphore;
		} m_Frames[FRAMES_IN_FLIGHT];
	
		uint32_t m_FrameIndex = 0U;
			
		bool m_ReloadQueued		  = false;
		bool m_FramebufferResized = false;
		bool m_SkipFrame		  = false;
		bool m_VSync			  = true;

		double m_FrameTime{};

		void MeasureFrameTime();
		void BeginRender();
		void ForwardPass();
		void ImGuiPass();
		void EndRender();

		void CreateForwardPass();
		void CreateForwardPipeline();

		static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
		void RecreateFramebuffer();
		void ReloadBackendImpl();

		void CreateBackend();

		void CreatePerFrameData();
		void DestroyPerFrameData();

		void CreateImGuiContext();
		void DestroyImGuiContext();
	};
}

#endif