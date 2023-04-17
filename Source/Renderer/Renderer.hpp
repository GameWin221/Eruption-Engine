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
#include <Renderer/Framebuffer.hpp>
#include <Renderer/Sampler.hpp>

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

		void ReloadBackend();

		void Update();
		void PreRender();
		void Render();

		en::Handle<Scene> GetScene() { return m_Scene; };

		const double GetFrameTime() const { return m_FrameTime; }

		int m_DebugMode = 0;

		std::function<void()> m_ImGuiRenderCallback;

	private:
		Handle<Scene> m_Scene;

		Handle<Swapchain> m_Swapchain;
		Handle<CameraBuffer> m_CameraBuffer;

		Handle<RenderPass> m_ImGuiRenderPass;
		Handle<RenderPass> m_RenderPass;
		Handle<RenderPass> m_DepthRenderPass;

		std::vector<Handle<Framebuffer>> m_ImGuiFramebuffers;
		std::vector<Handle<Framebuffer>> m_RenderFramebuffers;

		Handle<Framebuffer> m_DepthFramebuffer;

		Handle<GraphicsPipeline> m_Pipeline;
		Handle<GraphicsPipeline> m_DepthPipeline;

		Handle<Image> m_DepthBuffer;

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

		void WaitForActiveFrame();
		void ResetAllFrames();

		void MeasureFrameTime();
		void BeginRender();
		void ShadowPass();
		void DepthPass();
		void ForwardPass();
		void ImGuiPass();
		void EndRender();

		void CreateDepthPass();
		void CreateDepthPipeline();

		void CreateForwardPass();
		void CreateForwardPipeline();

		static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
		void RecreateFramebuffer();
		void ReloadBackendImpl();

		void CreateBackend(bool newImGui = true);

		void CreateDepthBuffer();

		void CreatePerFrameData();
		void DestroyPerFrameData();

		void CreateImGuiRenderPass();
		void CreateImGuiContext();
		void DestroyImGuiContext();
	};
}

#endif