#include <glad/glad.h>
#include "Window.hpp"
#include <spdlog/spdlog.h>
#include <stdexcept>

Window::Window(int width, int height, const std::string& title)
    : window_(nullptr), width_(width), height_(height), title_(title)
{
    if (!InitGLFW()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    // Create GLFW window
    window_ = glfwCreateWindow(width_, height_, title_.c_str(), nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        spdlog::critical("Failed to create GLFW window");
        throw std::runtime_error("GLFW window creation failed");
    }

    glfwMakeContextCurrent(window_);

    if (!InitGLAD()) {
        glfwDestroyWindow(window_);
        glfwTerminate();
        throw std::runtime_error("Failed to initialize GLAD");
    }

    spdlog::info("Window '{}' created ({}x{}) with OpenGL context", title_, width_, height_);
}

Window::~Window() {
    if (window_) glfwDestroyWindow(window_);
    glfwTerminate();
}

bool Window::InitGLFW() {
    if (!glfwInit()) {
        spdlog::critical("Failed to initialize GLFW");
        return false;
    }

    // Set OpenGL version (e.g., 4.6 core)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    return true;
}

bool Window::InitGLAD() {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        spdlog::critical("Failed to initialize GLAD");
        return false;
    }

    spdlog::info("OpenGL loaded: {}.{}", GLVersion.major, GLVersion.minor);
    return true;
}

void Window::PollEvents() {
    glfwPollEvents();
}

bool Window::ShouldClose() const {
    return glfwWindowShouldClose(window_);
}

void Window::SwapBuffers() {
    glfwSwapBuffers(window_);
}
