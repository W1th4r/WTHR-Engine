// Tests/MainTest.cpp
#include <catch2/catch_session.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

int main(int argc, char* argv[])
{
    // 1. Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW for testing!\n";
        return -1;
    }

    // 2. Configure hidden/offscreen window context
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hide window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Test Context", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create OpenGL window context for tests!\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // 3. ?? Initialize GLAD!
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD for tests!\n";
        return -1;
    }

    // 4. Run all Catch2 test cases
    int result = Catch::Session().run(argc, argv);

    // 5. Clean up
    glfwDestroyWindow(window);
    glfwTerminate();

    return result;
}