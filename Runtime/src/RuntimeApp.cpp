#include "RuntimeApp.hpp"
#include <glad/glad.h>
#include <spdlog/spdlog.h>

RuntimeApp::RuntimeApp(std::string name) {
	m_SceneName = name;
	Init();
}

void RuntimeApp::Init() {
	spdlog::info("Initializing RuntimeApp");

	m_Window = std::make_shared<Window>(1920, 1080, "WTHR Runtime");

	// Create scene and pass the native GLFW window to it
	m_Scene = std::make_shared<Scene>(m_Window->GetNativeWindow());

	//m_Scene->CreateCubeGrid(2, 2, 2, glm::vec3(0.f));

	m_Scene->Load("Default.sce");

	m_Renderer = std::make_shared<Renderer>();
	spdlog::info("Scene initialized with test cubes");
}

void RuntimeApp::Shutdown() {
	spdlog::info("Shutting down RuntimeApp");
	m_Scene.reset();
	m_Window.reset();
}

static auto lastFrame = std::chrono::high_resolution_clock::now();
void RuntimeApp::Run() {
	spdlog::info("Starting main game loop");
	auto ptrShdr = std::make_shared<Shader>("shaders/default.vert", "shaders/default.frag");

	while (!m_Window->ShouldClose()) {
		m_Window->PollEvents();

		// Clear screen
		m_Renderer->Clear();

		auto currentFrame = std::chrono::high_resolution_clock::now();
		float deltaTime = std::chrono::duration<float>(currentFrame - lastFrame).count();
		lastFrame = currentFrame;

		m_Scene->script.update(1);

		if (glfwGetKey(m_Window->GetNativeWindow(), GLFW_KEY_W) == GLFW_PRESS)
			m_Scene->GetCamera().ProcessKeyboard(FORWARD, deltaTime);
		if (glfwGetKey(m_Window->GetNativeWindow(), GLFW_KEY_S) == GLFW_PRESS)
			m_Scene->GetCamera().ProcessKeyboard(BACKWARD, deltaTime);
		if (glfwGetKey(m_Window->GetNativeWindow(), GLFW_KEY_A) == GLFW_PRESS)
			m_Scene->GetCamera().ProcessKeyboard(LEFT, deltaTime);
		if (glfwGetKey(m_Window->GetNativeWindow(), GLFW_KEY_D) == GLFW_PRESS)
			m_Scene->GetCamera().ProcessKeyboard(RIGHT, deltaTime);


		// TODO: call your Scene's render functions here
		//TODO: automate scene loading and input scene into binary
		m_Renderer->RenderScene(*m_Scene.get(), *ptrShdr);

		// Swap buffers
		m_Window->SwapBuffers();
	}

	Shutdown();
}
