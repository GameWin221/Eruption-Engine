#include <Core/EnPch.hpp>
#include "Eruption.hpp"

bool modelSpawned;

void Eruption::Init()
{
	en::WindowInfo windowInfo{};
	windowInfo.title	  = "Eruption Engine v0.3.6";
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

	// The Cerberus file is not in the repo because it's too heavy (100mb+)
	//m_AssetManager->LoadTexture("CerberusAlbedo", "Models/Cerberus/CerberusAlbedo.png", false);
	//m_AssetManager->LoadModel("Cerberus", "Models/Cerberus/Cerberus.obj", "CerberusAlbedo");

	m_AssetManager->LoadTexture("FloorTexture", "Models/floor.png");
	m_AssetManager->LoadModel("FloorModel", "Models/Plane.obj", "FloorTexture");

	en::UniformBufferObject& ubo = m_AssetManager->GetModel("BackpackModel")->m_UniformBuffer->m_UBO;
	ubo.model = glm::translate(ubo.model, glm::vec3(2, 0.5f, 0));
	ubo.model = glm::scale	  (ubo.model, glm::vec3(0.2f));

	en::UniformBufferObject& uboSkull = m_AssetManager->GetModel("AdditionalModel")->m_UniformBuffer->m_UBO;
	uboSkull.model = glm::translate(uboSkull.model, glm::vec3(0, 0.5f, 0));
	uboSkull.model = glm::rotate   (uboSkull.model, glm::radians(45.0f), glm::vec3(-1, 0, 0));

	//en::UniformBufferObject& uboCerberus = m_AssetManager->GetModel("Cerberus")->m_UniformBuffer->m_UBO;
	//uboCerberus.model = glm::translate(uboCerberus.model, glm::vec3(0, 1.4f, 2.5f));
	//uboCerberus.model = glm::rotate(uboCerberus.model, glm::radians(8.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
	//uboCerberus.model = glm::scale(uboCerberus.model, glm::vec3(0.3f));

	en::RendererInfo rendererInfo{};
	rendererInfo.clearColor  = { 0.01f, 0.01f, 0.01f, 1.0f };
	rendererInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rendererInfo.cullMode    = VK_CULL_MODE_BACK_BIT;

	m_Renderer = new en::Renderer(rendererInfo);
	m_Renderer->PrepareImGuiUI(std::bind(&Eruption::DrawImGuiUI, this));
	m_Renderer->PrepareModel(m_AssetManager->GetModel("BackpackModel"));
	m_Renderer->PrepareModel(m_AssetManager->GetModel("FloorModel"));
	m_Renderer->PrepareModel(m_AssetManager->GetModel("Cerberus"));

	m_Input = new en::InputManager;
	m_Input->m_MouseSensitivity = 0.1f;

	en::CameraInfo cameraInfo{};
	cameraInfo.dynamicallyScaled = true;
	cameraInfo.fov = 70.0f;
	cameraInfo.position = glm::vec3(0, 1, 0);

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
	m_Input->UpdateMouse();

	// Locking / Freeing the mouse
	if (m_Input->IsMouseButton(en::Button::Right, en::InputState::Pressed))
		m_Input->SetCursorMode(en::CursorMode::Locked);
	else if(m_Input->IsMouseButton(en::Button::Right, en::InputState::Released))
		m_Input->SetCursorMode(en::CursorMode::Free);

	// Spawning / Despawning the additional model
	if (m_Input->IsKey(en::Key::R, en::InputState::Pressed) && !modelSpawned)
	{
		m_Renderer->PrepareModel(m_AssetManager->GetModel("AdditionalModel"));
		modelSpawned = true;
	}
	else if (m_Input->IsKey(en::Key::T, en::InputState::Pressed) && modelSpawned)
	{
		m_Renderer->RemoveModel(m_AssetManager->GetModel("AdditionalModel"));
		modelSpawned = false;
	}
	
	if (m_Input->GetCursorMode() == en::CursorMode::Locked)
	{
		if (m_Input->IsKey(en::Key::W))
			m_Camera->m_Position += m_Camera->GetFront() * m_fDeltaTime;

		else if (m_Input->IsKey(en::Key::S))
			m_Camera->m_Position -= m_Camera->GetFront() * m_fDeltaTime;

		if (m_Input->IsKey(en::Key::A))
			m_Camera->m_Position -= m_Camera->GetRight() * m_fDeltaTime;

		else if (m_Input->IsKey(en::Key::D))
			m_Camera->m_Position += m_Camera->GetRight() * m_fDeltaTime;

		if (m_Input->IsKey(en::Key::Space))
			m_Camera->m_Position += m_Camera->GetUp() * m_fDeltaTime;

		else if (m_Input->IsKey(en::Key::Ctrl))
			m_Camera->m_Position -= m_Camera->GetUp() * m_fDeltaTime;

		m_Camera->m_Yaw   += m_Input->GetMouseVelocity().x;
		m_Camera->m_Pitch -= m_Input->GetMouseVelocity().y;
	}

	m_Input->UpdateInput();
}
void Eruption::Render()
{
	if (modelSpawned)
		m_Renderer->EnqueueModel(m_AssetManager->GetModel("AdditionalModel"));

	m_Renderer->EnqueueModel(m_AssetManager->GetModel("BackpackModel"));
	m_Renderer->EnqueueModel(m_AssetManager->GetModel("FloorModel"));
	//m_Renderer->EnqueueModel(m_AssetManager->GetModel("Cerberus"));

	m_Renderer->Render();
}

void Eruption::DrawImGuiUI()
{
	ImGui::Begin("Lights");

	

	ImGui::End();
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