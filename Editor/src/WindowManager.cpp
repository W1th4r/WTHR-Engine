#include <pch.hpp>
#include "WindowManager.hpp"


WindowManager::WindowManager(int width, int height, const char* title) :  m_Title(title), m_Window(nullptr)
{
    m_Width = 1920;
    m_Height = 1080;
}

WindowManager::~WindowManager()
{
    Shutdown();
}

bool WindowManager::Init()
{
  

    spdlog::info("Initializing GLFW...");

    if (!glfwInit())
    {
        spdlog::error("Failed to initialize GLFW!");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    m_Window = glfwCreateWindow(m_Width, m_Height, m_Title, nullptr, nullptr);
    if (!m_Window)
    {
        spdlog::error("Failed to create GLFW window!");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_Window);
    glfwSwapInterval(1); // vsync

    spdlog::info("GLFW window created: {}x{}", m_Width, m_Height);


    //GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        spdlog::error("Failed to initialize GLAD!");
        return false;
    }

    //IMGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    ImGuiIO& io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // enable keyboard controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // enable docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;    // enable multi-viewport


    glfwSetFramebufferSizeCallback(m_Window, FramebufferSizeCallback);

    return true;
}
void WindowManager::BeginFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

}
void WindowManager::EndFrame()
{
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(m_Window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
   // glClearColor(0 * 0, 0 * 0, 0 * 0, 0);
   // glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


   ImGuiIO& io = ImGui::GetIO();
    
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }


}
void WindowManager::Shutdown()
{
    //ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();



    if (m_Window)
    {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }
    glfwTerminate();




    spdlog::info("GLFW terminated.");
}

void WindowManager::PollEvents()
{
    glfwPollEvents();
}

void WindowManager::SwapBuffers()
{
    if (m_Window)
        glfwSwapBuffers(m_Window);
}

bool WindowManager::ShouldClose() const
{
    return m_Window ? glfwWindowShouldClose(m_Window) : true;
}

void WindowManager::SetTitle(const char* title)
{
    m_Title = title;
    if (m_Window)
        glfwSetWindowTitle(m_Window, m_Title);
}

void WindowManager::SetSize(int width, int height)
{
 
    if (m_Window)
        glfwSetWindowSize(m_Window, m_Width, m_Height);
}

// Static callback
void WindowManager::FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    spdlog::info("Framebuffer resized: {}x{}", width, height);
}
