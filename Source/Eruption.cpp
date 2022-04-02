#include <EnPch.hpp>
#include <Eruption.hpp>

void Eruption::Init()
{
	en::WindowInfo windowInfo{};
	windowInfo.title	  = "Eruption Engine v0.1.0";
	windowInfo.resizable  = true;
	windowInfo.fullscreen = false;
	windowInfo.size		  = glm::ivec2(2000, 1200);

	m_Window = new en::Window(windowInfo);

	m_Context = new en::Context;

	m_AssetImporter = new en::AssetImporter;
	m_AssetImporter->LoadTexture("BackpackTexture", "Models/Backpack/backpack_albedo.jpg");
	m_AssetImporter->LoadModel  ("BackpackModel"  , "Models/Backpack/backpack.obj", "BackpackTexture");
	
	m_AssetImporter->LoadTexture("ChaletTexture", "Models/chalet.jpg");
	m_AssetImporter->LoadModel  ("ChaletModel"  , "Models/chalet.obj", "ChaletTexture");
	
	m_AssetImporter->GetModel("ChaletModel")->m_UniformBuffer->m_UBO.model = glm::rotate(glm::mat4(1), glm::radians(-90.0f), glm::vec3(1, 0, 0));

	en::RendererInfo rendererInfo{};
	rendererInfo.clearColor  = { 0.01f, 0.01f, 0.01f, 1.0f };
	rendererInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rendererInfo.cullMode    = VK_CULL_MODE_BACK_BIT;

	m_Renderer = new en::Renderer(rendererInfo);

	m_Renderer->PrepareModel(m_AssetImporter->GetModel("BackpackModel"));
	m_Renderer->PrepareModel(m_AssetImporter->GetModel("ChaletModel"));

	en::CameraInfo cameraInfo{};
	cameraInfo.dynamicallyScaled = true;
	cameraInfo.fov = 70.0f;

	m_Camera = new en::Camera(cameraInfo);

	m_Renderer->SetMainCamera(m_Camera);
}
void Eruption::Update()
{
	static int counter = 0;

	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - m_LastFrame);

	if (counter == 6)
	{
		std::cout << "fps: " << 1.0 / ((double)duration.count()/1000000.0) << '\n';
		counter = 0;
	}
	else
		counter++;

	m_LastFrame = std::chrono::high_resolution_clock::now();

	m_Window->PollEvents();

	if (glfwGetKey(m_Window->m_GLFWWindow, GLFW_KEY_ESCAPE))
		m_Window->Close();

	if (glfwGetKey(m_Window->m_GLFWWindow, GLFW_KEY_W))
		m_Camera->m_Position += m_Camera->GetFront() * 0.002f;

	else if (glfwGetKey(m_Window->m_GLFWWindow, GLFW_KEY_S))
		m_Camera->m_Position -= m_Camera->GetFront() * 0.002f;

	if (glfwGetKey(m_Window->m_GLFWWindow, GLFW_KEY_A))
		m_Camera->m_Position -= m_Camera->GetRight() * 0.002f;

	else if (glfwGetKey(m_Window->m_GLFWWindow, GLFW_KEY_D))
		m_Camera->m_Position += m_Camera->GetRight() * 0.002f;

	if (glfwGetKey(m_Window->m_GLFWWindow, GLFW_KEY_Z))
		m_Camera->m_Position += m_Camera->GetUp() * 0.002f;

	else if (glfwGetKey(m_Window->m_GLFWWindow, GLFW_KEY_X))
		m_Camera->m_Position -= m_Camera->GetUp() * 0.002f;

	if (glfwGetKey(m_Window->m_GLFWWindow, GLFW_KEY_Q))
		m_Camera->m_Yaw -= 0.03f;

	else if (glfwGetKey(m_Window->m_GLFWWindow, GLFW_KEY_E))
		m_Camera->m_Yaw += 0.03f;

	if (glfwGetKey(m_Window->m_GLFWWindow, GLFW_KEY_F))
		m_Camera->m_Pitch -= 0.03f;

	else if (glfwGetKey(m_Window->m_GLFWWindow, GLFW_KEY_R))
		m_Camera->m_Pitch += 0.03f;
}
void Eruption::Render()
{
	m_Renderer->EnqueueModel(m_AssetImporter->GetModel("ChaletModel"));

	m_Renderer->Render();
}

void Eruption::Run()
{
	Init();

	while (m_Window->IsOpen())
	{
		Update();
		Render();
	}
}