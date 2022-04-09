#include <EnPch.hpp>
#include <Eruption.hpp>

bool modelSpawned;

void Eruption::Init()
{
	en::WindowInfo windowInfo{};
	windowInfo.title	  = "Eruption Engine v0.2.2";
	windowInfo.resizable  = true;
	windowInfo.fullscreen = false;
	windowInfo.size		  = glm::ivec2(1920, 1080);

	m_Window = new en::Window(windowInfo);

	m_Context = new en::Context;

	m_AssetManager = new en::AssetManager;

	m_AssetManager->LoadTexture("BackpackTexture", "Models/Backpack/backpack_albedo.jpg");
	m_AssetManager->LoadModel("BackpackModel", "Models/Backpack/Backpack.obj", "BackpackTexture");

	m_AssetManager->LoadTexture("AdditionalTexture", "Models/Skull/skull_albedo.jpg");
	m_AssetManager->LoadModel("AdditionalModel", "Models/Skull/skull.obj", "AdditionalTexture");

	en::UniformBufferObject& ubo = m_AssetManager->GetModel("BackpackModel")->m_UniformBuffer->m_UBO;
	ubo.model = glm::translate(ubo.model, glm::vec3(2, 0.5f, 0));
	ubo.model = glm::scale(ubo.model, glm::vec3(0.2f));

	en::RendererInfo rendererInfo{};
	rendererInfo.clearColor  = { 0.01f, 0.01f, 0.01f, 1.0f };
	rendererInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rendererInfo.cullMode    = VK_CULL_MODE_BACK_BIT;

	m_Renderer = new en::Renderer(rendererInfo);

	m_Renderer->PrepareModel(m_AssetManager->GetModel("BackpackModel"));

	m_Input = new en::InputManager;
	m_Input->m_MouseSensitivity = 0.1f;
	m_Input->SetCursorMode(en::CursorMode::Locked);

	en::CameraInfo cameraInfo{};
	cameraInfo.dynamicallyScaled = true;
	cameraInfo.fov = 70.0f;

	m_Camera = new en::Camera(cameraInfo);

	m_Renderer->SetMainCamera(m_Camera);;
}
void Eruption::Update()
{
	static int counter = 0;

	const double m_TargetFrameTime = 1000000.0 / m_TargetFPS;

	static std::chrono::high_resolution_clock::time_point lastFrame;

	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - lastFrame);

	lastFrame = std::chrono::high_resolution_clock::now();

	m_DeltaTime = duration.count() / 1000000.0;
	m_fDeltaTime = duration.count() / 1000000.0f;

	if (counter == 6)
	{
		std::cout << "fps: " << 1.0 / ((double)duration.count()/1000000.0) << '\n';
		counter = 0;
	}
	else
		counter++;

	m_Window->PollEvents();
	
	m_Input->UpdateInput();

	if (m_Input->IsKeyDown(en::Key::Escape))
		m_Input->SetCursorMode(en::CursorMode::Free);

	
	if (m_Input->IsKeyDown(en::Key::R) && !modelSpawned)
	{
		m_Renderer->PrepareModel(m_AssetManager->GetModel("AdditionalModel"));
		modelSpawned = true;
	}
	else if (m_Input->IsKeyDown(en::Key::T) && modelSpawned)
	{
		m_Renderer->RemoveModel(m_AssetManager->GetModel("AdditionalModel"));
		modelSpawned = false;
	}
	

	if (m_Input->GetCursorMode() == en::CursorMode::Free && m_Input->IsMouseButtonDown(en::Button::Left))
		m_Input->SetCursorMode(en::CursorMode::Locked);

	if (m_Input->IsKeyDown(en::Key::W))
		m_Camera->m_Position += m_Camera->GetFront() * m_fDeltaTime;

	else if (m_Input->IsKeyDown(en::Key::S))
		m_Camera->m_Position -= m_Camera->GetFront() * m_fDeltaTime;

	if (m_Input->IsKeyDown(en::Key::A))
		m_Camera->m_Position -= m_Camera->GetRight() * m_fDeltaTime;

	else if (m_Input->IsKeyDown(en::Key::D))
		m_Camera->m_Position += m_Camera->GetRight() * m_fDeltaTime;

	if (m_Input->IsKeyDown(en::Key::Space))
		m_Camera->m_Position += m_Camera->GetUp() * m_fDeltaTime;

	else if (m_Input->IsKeyDown(en::Key::Ctrl))
		m_Camera->m_Position -= m_Camera->GetUp() * m_fDeltaTime;

	if (m_Input->GetCursorMode() == en::CursorMode::Locked)
	{
		m_Camera->m_Yaw   += m_Input->GetMouseVelocity().x;
		m_Camera->m_Pitch -= m_Input->GetMouseVelocity().y;
	}
	
}
void Eruption::Render()
{
	if (modelSpawned)
		m_Renderer->EnqueueModel(m_AssetManager->GetModel("AdditionalModel"));

	m_Renderer->EnqueueModel(m_AssetManager->GetModel("BackpackModel"));

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