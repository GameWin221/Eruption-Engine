#include <Core/EnPch.hpp>
#include "Eruption.hpp"

void Eruption::Init()
{
	EN_LOG("Eruption::Init() - Started");

	en::WindowInfo windowInfo{};
	windowInfo.title	  = "Eruption Engine v0.6.0";
	windowInfo.resizable  = true;
	windowInfo.fullscreen = false;
	windowInfo.size		  = glm::ivec2(1920, 1080);

	m_Window = new en::Window(windowInfo);

	m_Context = new en::Context;

	en::RendererInfo rendererInfo{};
	rendererInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rendererInfo.cullMode = VK_CULL_MODE_BACK_BIT;

	m_Renderer = new en::Renderer(rendererInfo);

	m_Input = new en::InputManager;
	m_Input->m_MouseSensitivity = 0.1f;

	m_AssetManager = new en::AssetManager;

	m_Editor = new en::EditorLayer;
	m_Editor->AttachTo(m_Renderer, m_AssetManager, &m_DeltaTime);

	en::CameraInfo cameraInfo{};
	cameraInfo.dynamicallyScaled = true;
	cameraInfo.fov = 70.0f;
	cameraInfo.position = glm::vec3(0, 1, 0);

	m_Camera = new en::Camera(cameraInfo);

	m_Renderer->SetMainCamera(m_Camera);

	EN_LOG("Eruption::Init() - Finished");

	CreateExampleScene();
}
void Eruption::Update()
{
	static std::chrono::high_resolution_clock::time_point lastFrame;

	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - lastFrame);

	lastFrame = std::chrono::high_resolution_clock::now();

	m_DeltaTime = duration.count() / 1000000.0;

	m_Window->PollEvents();
	m_Input->UpdateMouse();

	// Locking / Freeing the mouse
	if (m_Input->IsMouseButton(en::Button::Right, en::InputState::Pressed))
		m_Input->SetCursorMode(en::CursorMode::Locked);
	else if(m_Input->IsMouseButton(en::Button::Right, en::InputState::Released))
		m_Input->SetCursorMode(en::CursorMode::Free);

	if (m_Input->GetCursorMode() == en::CursorMode::Locked)
	{
		if (m_Input->IsKey(en::Key::W))
			m_Camera->m_Position += m_Camera->GetFront() * (float)m_DeltaTime;

		else if (m_Input->IsKey(en::Key::S))
			m_Camera->m_Position -= m_Camera->GetFront() * (float)m_DeltaTime;

		if (m_Input->IsKey(en::Key::A))
			m_Camera->m_Position -= m_Camera->GetRight() * (float)m_DeltaTime;

		else if (m_Input->IsKey(en::Key::D))
			m_Camera->m_Position += m_Camera->GetRight() * (float)m_DeltaTime;

		if (m_Input->IsKey(en::Key::Space))
			m_Camera->m_Position += m_Camera->GetUp() * (float)m_DeltaTime;

		else if (m_Input->IsKey(en::Key::Ctrl))
			m_Camera->m_Position -= m_Camera->GetUp() * (float)m_DeltaTime;

		m_Camera->m_Yaw   += m_Input->GetMouseVelocity().x;
		m_Camera->m_Pitch -= m_Input->GetMouseVelocity().y;
	}

	m_Input->UpdateInput();

	m_Renderer->WaitForGPUIdle();

	m_AssetManager->UpdateAssets();
}
void Eruption::Render()
{
	m_Renderer->Render();
}

void Eruption::CreateExampleScene()
{
	m_AssetManager->LoadMesh("SkullModel", "Models/Skull/Skull.gltf"  );

	m_AssetManager->LoadTexture("SkullAlbedo", "Models/Skull/SkullAlbedo.jpg");
	m_AssetManager->LoadTexture("SkullRoughness", "Models/Skull/SkullRoughness.jpg", { en::TextureFormat::NonColor });
	m_AssetManager->LoadTexture("SkullNormal", "Models/Skull/SkullNormal.jpg", { en::TextureFormat::NonColor });

	m_AssetManager->CreateMaterial("SkullMaterial", glm::vec3(1.0f), 0.0f, 1.0f, m_AssetManager->GetTexture("SkullAlbedo"), m_AssetManager->GetTexture("SkullRoughness"), m_AssetManager->GetTexture("SkullNormal"));

	m_AssetManager->GetMesh("SkullModel")->m_SubMeshes[0].m_Material = m_AssetManager->GetMaterial("SkullMaterial");

	m_AssetManager->LoadMesh("Sponza", "Models/Sponza/Sponza.gltf");

	m_ExampleScene = new en::Scene;

	en::SceneObject* m_Skull = m_ExampleScene->CreateSceneObject("SkullModel", m_AssetManager->GetMesh("SkullModel"));
	m_Skull->m_Position = glm::vec3(0, 0.5f, -0.3);
	m_Skull->m_Rotation = glm::vec3(45.0f, 90.0f, 0.0f);

	en::SceneObject* m_Sponza = m_ExampleScene->CreateSceneObject("Sponza", m_AssetManager->GetMesh("Sponza"));
	m_Sponza->m_Scale = glm::vec3(1.2f);

	m_ExampleScene->CreatePointLight(glm::vec3(10.9, 1.4, 0.5), glm::vec3(0.4, 1.0, 0.4));
	m_ExampleScene->CreatePointLight(glm::vec3(-0.4, 6.2, 2.5), glm::vec3(1.0, 0.4, 0.4));
	m_ExampleScene->CreatePointLight(glm::vec3(-6.8, 1.0,-1.0), glm::vec3(0.2, 0.2, 1.0));
	m_ExampleScene->CreatePointLight(glm::vec3( 1.2, 2.0,-1.0), glm::vec3(1.0)		    );

	m_ExampleScene->m_AmbientColor = glm::vec3(0.027f, 0.027f, 0.055f);

	m_Renderer->BindScene(m_ExampleScene);
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