#include <Core/EnPch.hpp>
#include "Eruption.hpp"

bool modelSpawned;

void Eruption::Init()
{
	EN_LOG("Eruption::Init() - Started");

	en::WindowInfo windowInfo{};
	windowInfo.title	  = "Eruption Engine v0.4.3";
	windowInfo.resizable  = true;
	windowInfo.fullscreen = false;
	windowInfo.size		  = glm::ivec2(1920, 1080);

	m_Window = new en::Window(windowInfo);

	m_Context = new en::Context;

	en::RendererInfo rendererInfo{};
	rendererInfo.clearColor = { 0.01f, 0.01f, 0.01f, 1.0f };
	rendererInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rendererInfo.cullMode = VK_CULL_MODE_BACK_BIT;

	m_Renderer = new en::Renderer(rendererInfo);

	m_UILayer = new en::UILayer;
	m_UILayer->AttachTo(m_Renderer, &m_DeltaTime);

	m_Input = new en::InputManager;
	m_Input->m_MouseSensitivity = 0.1f;

	m_AssetManager = new en::AssetManager;

	m_AssetManager->LoadMesh("BackpackModel"  , "Models/Backpack/Backpack.obj");
	m_AssetManager->LoadMesh("AdditionalModel", "Models/Skull/skull.obj"	  );
	m_AssetManager->LoadMesh("FloorModel"     , "Models/Plane.obj"			  );

	m_AssetManager->LoadTexture("BackpackTexture"  , "Models/Backpack/backpack_albedo.jpg");
	m_AssetManager->LoadTexture("AdditionalTexture", "Models/Skull/skull_albedo.jpg"	  );
	m_AssetManager->LoadTexture("FloorTexture"     , "Models/floor.png"					  );

	m_AssetManager->LoadTexture("BackpackSpecularTexture"  , "Models/Backpack/backpack_specular.jpg");
	m_AssetManager->LoadTexture("AdditionalSpecularTexture", "Models/Skull/skull_specular.jpg"		);
	m_AssetManager->LoadTexture("FloorSpecularTexture"     , "Models/floor_specular.png"			);

	m_AssetManager->CreateMaterial("BackpackMaterial"  , glm::vec3(1.0f), 30.0f , m_AssetManager->GetTexture("BackpackTexture"  ), m_AssetManager->GetTexture("BackpackSpecularTexture"  ));
	m_AssetManager->CreateMaterial("AdditionalMaterial", glm::vec3(1.0f), 100.0f , m_AssetManager->GetTexture("AdditionalTexture"), m_AssetManager->GetTexture("AdditionalSpecularTexture"));
	m_AssetManager->CreateMaterial("FloorMaterial"	   , glm::vec3(1.0f), 75.0f, m_AssetManager->GetTexture("FloorTexture"     ), m_AssetManager->GetTexture("FloorSpecularTexture"     ));

	m_AssetManager->GetMesh("BackpackModel"  )->m_SubMeshes[0].m_Material = m_AssetManager->GetMaterial("BackpackMaterial"  );
	m_AssetManager->GetMesh("AdditionalModel")->m_SubMeshes[0].m_Material = m_AssetManager->GetMaterial("AdditionalMaterial");
	m_AssetManager->GetMesh("FloorModel"     )->m_SubMeshes[0].m_Material = m_AssetManager->GetMaterial("FloorMaterial"     );

	m_Skull = new en::SceneObject(m_AssetManager->GetMesh("AdditionalModel"));
	m_Skull->m_Position = glm::vec3(0, 0.5f, 0);
	m_Skull->m_Rotation = glm::vec3(-45.0f, 0.0f, 0.0f);

	m_Backpack = new en::SceneObject(m_AssetManager->GetMesh("BackpackModel"));
	m_Floor	   = new en::SceneObject(m_AssetManager->GetMesh("FloorModel"));

	m_Backpack->m_Position = glm::vec3(2, 0.5f, 0);
	m_Backpack->m_Scale = glm::vec3(0.2f);

	en::CameraInfo cameraInfo{};
	cameraInfo.dynamicallyScaled = true;
	cameraInfo.fov = 70.0f;
	cameraInfo.position = glm::vec3(0, 1, 0);

	m_Camera = new en::Camera(cameraInfo);

	m_Renderer->SetMainCamera(m_Camera);

	EN_LOG("Eruption::Init() - Finished");
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

	m_Window->PollEvents();
	m_Input->UpdateMouse();

	// Locking / Freeing the mouse
	if (m_Input->IsMouseButton(en::Button::Right, en::InputState::Pressed))
		m_Input->SetCursorMode(en::CursorMode::Locked);
	else if(m_Input->IsMouseButton(en::Button::Right, en::InputState::Released))
		m_Input->SetCursorMode(en::CursorMode::Free);

	// Spawning / Despawning the additional model
	if (m_Input->IsKey(en::Key::R, en::InputState::Pressed))
		modelSpawned = true;

	else if (m_Input->IsKey(en::Key::T, en::InputState::Pressed))
		modelSpawned = false;
	
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
		m_Renderer->EnqueueSceneObject(m_Skull);

	m_Renderer->EnqueueSceneObject(m_Backpack);
	m_Renderer->EnqueueSceneObject(m_Floor);

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