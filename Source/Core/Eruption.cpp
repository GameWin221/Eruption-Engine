#include "Eruption.hpp"

void Eruption::Init()
{
	EN_LOG("Eruption::Init() - Started");

	m_Window = en::MakeScope<en::Window>(
		"Eruption Engine v0.8.0",
		glm::ivec2(1920, 1080),
		false, 
		true
	);

	m_Context = en::MakeScope<en::Context>();

	m_Renderer = en::MakeScope<en::Renderer>();

	m_Input = en::MakeScope<en::InputManager>();
	m_Input->m_MouseSensitivity = 0.1f;

	m_AssetManager = en::MakeScope<en::AssetManager>();

	m_Editor = en::MakeScope<en::EditorLayer>();
	m_Editor->AttachTo(m_Renderer.get(), m_AssetManager.get());

	m_Camera = en::MakeHandle<en::Camera>(60.0f, 0.1f, 200.0f, glm::vec3(6.570f, 2.214f, 0.809f));
	m_Camera->m_Yaw = -165.6f;
	m_Camera->m_Pitch = -8.3f;

	EN_LOG("Eruption::Init() - Finished");

	CreateExampleScene();

	m_ExampleScene->m_MainCamera = m_Camera;
}
void Eruption::Update()
{
	m_Window->PollEvents();
	
	m_Input->UpdateMouse();

	// Locking / Freeing the mouse
	if (m_Input->IsMouseButton(en::Button::Right, en::InputState::Pressed))
		m_Input->SetCursorMode(en::CursorMode::Locked);
	else if(m_Input->IsMouseButton(en::Button::Right, en::InputState::Released))
		m_Input->SetCursorMode(en::CursorMode::Free);

	if (m_Input->IsKey(en::Key::LShift) && m_Input->IsKey(en::Key::I, en::InputState::Pressed))
		m_Editor->SetVisibility(!m_Editor->GetVisibility());

	if (m_Input->IsKey(en::Key::LShift) && m_Input->IsKey(en::Key::R, en::InputState::Pressed))
		m_Renderer->ReloadBackend();

	double deltaTime = m_Renderer->GetFrameTime();

	static float targetYaw = m_Camera->m_Yaw;
	static float targetPitch = m_Camera->m_Pitch;

	if (m_Input->GetCursorMode() == en::CursorMode::Locked)
	{
		if (m_Input->IsKey(en::Key::W) || m_Input->IsKey(en::Key::Up))
			m_Camera->m_Position += m_Camera->GetFront() * (float)deltaTime;

		else if (m_Input->IsKey(en::Key::S) || m_Input->IsKey(en::Key::Down))
			m_Camera->m_Position -= m_Camera->GetFront() * (float)deltaTime;

		if (m_Input->IsKey(en::Key::A) || m_Input->IsKey(en::Key::Left))
			m_Camera->m_Position -= m_Camera->GetRight() * (float)deltaTime;

		else if (m_Input->IsKey(en::Key::D) || m_Input->IsKey(en::Key::Right))
			m_Camera->m_Position += m_Camera->GetRight() * (float)deltaTime;

		if (m_Input->IsKey(en::Key::Space) || m_Input->IsKey(en::Key::RShift))
			m_Camera->m_Position += m_Camera->GetUp() * (float)deltaTime;

		else if (m_Input->IsKey(en::Key::LCtrl) || m_Input->IsKey(en::Key::RCtrl))
			m_Camera->m_Position -= m_Camera->GetUp() * (float)deltaTime;

		targetYaw += m_Input->GetMouseVelocity().x;
		targetPitch -= m_Input->GetMouseVelocity().y;

		// Clamping Pitch
		if (targetPitch > 89.999f) targetPitch = 89.999f;
		else if (targetPitch < -89.999f) targetPitch = -89.999f;
	}
	
	m_Camera->m_Yaw = glm::mix(m_Camera->m_Yaw, targetYaw, std::fmin(30.0 * deltaTime, 1.0));
	m_Camera->m_Pitch = glm::mix(m_Camera->m_Pitch, targetPitch, std::fmin(30.0 * deltaTime, 1.0));

	m_Input->UpdateInput();
	
	m_Renderer->Update();
}
void Eruption::Render()
{
	m_Renderer->PreRender();
	m_Renderer->Render();
}

void Eruption::CreateExampleScene()
{
	/*
	m_AssetManager->LoadMesh("SkullModel", "Models/Skull/Skull.gltf");
	
	m_AssetManager->LoadTexture("SkullAlbedo", "Models/Skull/SkullAlbedo.jpg");
	m_AssetManager->LoadTexture("SkullRoughness", "Models/Skull/SkullRoughness.jpg", { en::TextureFormat::NonColor });
	m_AssetManager->LoadTexture("SkullNormal", "Models/Skull/SkullNormal.jpg", { en::TextureFormat::NonColor });
	
	m_AssetManager->CreateMaterial("SkullMaterial", glm::vec3(1.0f), 0.0f, 0.65f, 1.0f, m_AssetManager->GetTexture("SkullAlbedo"),m_AssetManager->GetTexture("SkullRoughness"),m_AssetManager->GetTexture("SkullNormal"));
	
	m_AssetManager->GetMesh("SkullModel")->m_SubMeshes[0].SetMaterial(m_AssetManager->GetMaterial("SkullMaterial"));
	
	m_AssetManager->LoadMesh("Sponza", "Models/Sponza/Sponza.gltf");
	*/
	m_ExampleScene = en::MakeHandle<en::Scene>();
	/*
	auto m_Sponza = m_ExampleScene->CreateSceneObject("Sponza", m_AssetManager->GetMesh("Sponza"));
	m_Sponza->SetScale(glm::vec3(1.2f));

	m_AssetManager->GetMaterial("BlueCurtains")->SetNormalStrength(0.6f);
	m_AssetManager->GetMaterial("RedCurtains")->SetNormalStrength(0.6f);
	m_AssetManager->GetMaterial("GreenCurtains")->SetNormalStrength(0.6f);

	auto m_Skull = m_ExampleScene->CreateSceneObject("SkullModel", m_AssetManager->GetMesh("SkullModel"));
	m_Skull->SetPosition(glm::vec3(0, 0.5f, -0.3));
	m_Skull->SetRotation(glm::vec3(45.0f, 90.0f, 0.0f));
	*/
	


	//for (uint32_t i = 0; i < MAX_POINT_LIGHTS; i++)
	//{
	//	glm::vec3 spawnPoint(
	//		float(rand() % 2400 - 1200) / 100.0f,
	//		0.2f + float(rand() % 1200) / 100.0f,
	//		float(rand() % 1100 - 550 ) / 100.0f
	//	);
	//
	//	float intensity = 2.0f + float(rand() % 30) / 2.0f;
	//
	//	float radius = 1.0f + float(rand() % 100) / 45.0f;
	//
	//	glm::vec3 color(
	//		float(rand() % 1000) / 1000.0f,
	//		float(rand() % 1000) / 1000.0f,
	//		float(rand() % 1000) / 1000.0f
	//	);
	//
	//	m_ExampleScene->CreatePointLight(spawnPoint, color, intensity, radius);
	//}

	m_ExampleScene->m_AmbientColor = glm::vec3(0.042f, 0.045f, 0.074f);

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

	m_ExampleScene.reset();
	m_Editor.reset();
	m_AssetManager.reset();
	m_Renderer.reset();
	m_Input.reset();
	m_Window.reset();
	m_Context.reset();
}