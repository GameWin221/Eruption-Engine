#include <Core/EnPch.hpp>
#include "Eruption.hpp"

bool modelSpawned = true;

void Eruption::Init()
{
	EN_LOG("Eruption::Init() - Started");

	en::WindowInfo windowInfo{};
	windowInfo.title	  = "Eruption Engine v0.4.6";
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

	m_AssetManager->LoadMesh("BackpackModel", "Models/Backpack/Backpack.fbx");
	m_AssetManager->LoadMesh("SkullModel"   , "Models/Skull/Skull.gltf"	    );
	m_AssetManager->LoadMesh("FloorModel"   , "Models/Floor/Floor.obj"   	);

	m_AssetManager->LoadTexture("BackpackAlbedo", "Models/Backpack/BackpackAlbedo.jpg");
	m_AssetManager->LoadTexture("SkullAlbedo"	, "Models/Skull/SkullAlbedo.jpg"	   );
	m_AssetManager->LoadTexture("FloorAlbedo"   , "Models/Floor/FloorAlbedo.png"	   );

	m_AssetManager->LoadTexture("BackpackSpecular", "Models/Backpack/BackpackSpecular.jpg");
	m_AssetManager->LoadTexture("SkullSpecular"	  , "Models/Skull/SkullSpecular.jpg"	  );
	m_AssetManager->LoadTexture("FloorSpecular"   , "Models/Floor/FloorSpecular.png"	  );

	m_AssetManager->LoadTexture("BackpackNormal", "Models/Backpack/BackpackNormal.jpg", VK_FORMAT_R8G8B8A8_UNORM);
	m_AssetManager->LoadTexture("SkullNormal"   , "Models/Skull/SkullNormal.jpg"      , VK_FORMAT_R8G8B8A8_UNORM);
	m_AssetManager->LoadTexture("FloorNormal"   , "Models/Floor/FloorNormal.png"      , VK_FORMAT_R8G8B8A8_UNORM);

	m_AssetManager->CreateMaterial("BackpackMaterial", glm::vec3(1.0f), 50.0f , 0.3f, m_AssetManager->GetTexture("BackpackAlbedo"), m_AssetManager->GetTexture("BackpackSpecular"), m_AssetManager->GetTexture("BackpackNormal"));
	m_AssetManager->CreateMaterial("SkullMaterial"   , glm::vec3(1.0f), 40.0f, 1.0f, m_AssetManager->GetTexture("SkullAlbedo"   ), m_AssetManager->GetTexture("SkullSpecular")   , m_AssetManager->GetTexture("SkullNormal")   );
	m_AssetManager->CreateMaterial("FloorMaterial"	 , glm::vec3(1.0f), 200.0f , 1.0f, m_AssetManager->GetTexture("FloorAlbedo"   ), m_AssetManager->GetTexture("FloorSpecular")   , m_AssetManager->GetTexture("FloorNormal")   );

	m_AssetManager->GetMesh("BackpackModel")->m_SubMeshes[0].m_Material = m_AssetManager->GetMaterial("BackpackMaterial");
	m_AssetManager->GetMesh("SkullModel"   )->m_SubMeshes[0].m_Material = m_AssetManager->GetMaterial("SkullMaterial"   );
	m_AssetManager->GetMesh("FloorModel"   )->m_SubMeshes[0].m_Material = m_AssetManager->GetMaterial("FloorMaterial"   );

	m_Skull = new en::SceneObject(m_AssetManager->GetMesh("SkullModel"));
	m_Skull->m_Position = glm::vec3(0, 0.5f, 0);
	m_Skull->m_Rotation = glm::vec3(45.0f, 0.0f, 0.0f);

	m_Backpack = new en::SceneObject(m_AssetManager->GetMesh("BackpackModel"));
	m_Backpack->m_Position = glm::vec3(2, 0.5f, 0);
	m_Backpack->m_Scale = glm::vec3(0.2f);

	m_Floor	   = new en::SceneObject(m_AssetManager->GetMesh("FloorModel"));

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