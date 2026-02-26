#include "pch.hpp"      // precompiled header
#include "Application.hpp"
#include <PrimitiveShape.hpp>
#include <filesystem>
#include <Components.hpp>
#include <Scene.hpp>


Application::Application()
{

}

Application::~Application()
{
	Shutdown();
}
void WindowSizeCallback(GLFWwindow* window, int width, int height)
{
	Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

	// 2. Safety check the app and its renderer
	if (app)
	{
		app->GetRenderer().setSize(width, height);
	}
}

void drop_callback(GLFWwindow* window, int count, const char** paths)
{
	// 1. Get the "this" pointer back
	auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
	if (!app) return;

	// 2. Get the reference to the textures map
	auto& m_Textures = app->GetScene().GetTextures();
	auto& l_ActiveScene = app->GetScene();


	for (int i = 0; i < count; i++)
	{
		std::filesystem::path droppedPath = paths[i];
		std::string extension = droppedPath.extension().string();

		if (extension == ".obj" || extension == ".fbx") {
			// Call your model loading logic
			l_ActiveScene.CreateModel(droppedPath.string(), glm::vec4(1.0f));
		}


		if (extension == ".png" || extension == ".jpg") {
			spdlog::debug("Dropped texture: {}", droppedPath.string());

			// Create and insert
			Texture texture(droppedPath.string(), "texture_diffuse");

			m_Textures.insert({ droppedPath.string(), texture });
		}
	}
}

bool Application::Init()
{
	m_WindowManager.Init();

	InitImGui();
	InitEngineSystems();
	m_Renderer.Init();
	// Setup
	glfwSetWindowUserPointer(m_WindowManager.GetWindow(), this);
	glfwSetWindowSizeCallback(m_WindowManager.GetWindow(), WindowSizeCallback);
	glfwSetDropCallback(m_WindowManager.GetWindow(), drop_callback);

	m_Input.setWindow(m_WindowManager.GetWindow());
	if (m_WindowManager.GetWindow() == nullptr) __debugbreak();

	m_World.SetScene(&m_ActiveScene);
	m_Renderer.m_Editor.set(&m_ActiveScene.m_Script);

	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	GLFWwindow* workerWindow = glfwCreateWindow(1, 1, "GPU Worker", nullptr, m_WindowManager.GetWindow());


	GLFWwindow* contextA = m_WindowManager.GetWindow();

	m_UI.Initialize(m_ActiveScene, m_Renderer, m_Input, m_WindowManager);
	m_ActiveScene.SetWorker(m_WindowManager.GetWindow());

	return true;
}


bool CheckSharedContextThreads(GLFWwindow* contextA, GLFWwindow* contextB, std::thread& threadB)
{
	if (!contextA || !contextB)
	{
		spdlog::error("Invalid GLFW window pointer(s) provided.");
		return false;
	}

	std::atomic<bool> result{ false };
	std::atomic<bool> done{ false };

	// Launch a thread to test contextA against contextB
	std::thread testThread([&]() {
		// Make contextA current on this thread
		glfwMakeContextCurrent(contextA);

		// Try to query some OpenGL state that should exist if contextB is shared
		GLint maxTexUnits = 0;
		glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTexUnits);

		// If we got a valid value (>0), assume contexts are shared
		result = (maxTexUnits > 0);

		done = true;
		});

	// Wait for the test to finish (timeout optional)
	while (!done)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

	testThread.join();

	return result;
}
std::string load_file(const std::string& path) {
	std::ifstream f(path);
	std::stringstream ss;
	ss << f.rdbuf();
	return ss.str();
}

auto lastFrame = std::chrono::high_resolution_clock::now();
void Application::Run()
{
	if (!m_WindowManager.isOpen())
		return;

	spdlog::info("Running WTHR Application...");


	ImGuiIO& io = ImGui::GetIO();



	auto ptrShdr = std::make_shared<Shader>("shaders/default.vert", "shaders/default.frag");
	//m_Compiler.CookModel(std::string(),std::string("cooked"));
	while (m_WindowManager.isOpen())
	{
		auto currentFrame = std::chrono::high_resolution_clock::now();
		float deltaTime = std::chrono::duration<float>(currentFrame - lastFrame).count();
		lastFrame = currentFrame;

		io.DeltaTime = deltaTime;
		m_ActiveScene.m_Script.update(1);

		m_World.stepSimulation(.016f);

		m_WindowManager.PollEvents();
		m_Input.Update();
		m_WindowManager.BeginFrame();


		m_UI.Update();
		m_UI.Render();


		m_Renderer.Clear();

		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y,
			ImGui::GetWindowSize().x, ImGui::GetWindowSize().y);
		m_Renderer.RenderScene(m_ActiveScene, *ptrShdr);
		ImGui::End();


		auto& registry = m_ActiveScene.GetRegistry();

		m_Renderer.m_Editor.draw(registry);
		m_Renderer.m_Editor.drawDebugPanel(registry);

		m_WindowManager.EndFrame();

		m_WindowManager.SwapBuffers();

	}
}


void Application::Update()
{
	// ECS, input, editor logic
}

void Application::Render()
{
	// Rendering is handled by Renderer library
}

void Application::InitEngineSystems()
{
	spdlog::info("Initializing ECS, asset loaders, physics, etc.");
	// Example: EnTT registry, JSON parser, Assimp importer setup
}

void Application::InitImGui()
{
	spdlog::info("Initializing ImGui...");
	// Setup ImGui context here
	// ImGui::CreateContext();
	// ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
	// ImGui_ImplOpenGL3_Init("#version 460");
}

void Application::Shutdown()
{
	spdlog::info("Shutting down WTHR Application...");

	// Cleanup ImGui
	// ImGui_ImplOpenGL3_Shutdown();
	// ImGui_ImplGlfw_Shutdown();
	// ImGui::DestroyContext();


}
