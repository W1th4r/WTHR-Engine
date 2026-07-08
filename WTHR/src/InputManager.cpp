#include <pch.hpp>
#include "InputManager.hpp"

// Initialize the static variables
GLFWwindow* InputManager::m_Window = nullptr;
std::array<bool, 350> InputManager::m_Keys = { false };
std::array<bool, 8> InputManager::m_MouseButtons = { false };
double InputManager::m_MouseX = 0.0;
double InputManager::m_MouseY = 0.0;

void InputManager::Update()
{
    // Safety check: Don't poll if the window hasn't been set yet
    if (!m_Window) return;

    // Update keyboard state safely using the array boundaries
    for (int key = GLFW_KEY_SPACE; key < m_Keys.size(); ++key)
    {
        m_Keys[key] = (glfwGetKey(m_Window, key) == GLFW_PRESS);
    }

    // Update mouse button state safely using the array boundaries
    for (int button = GLFW_MOUSE_BUTTON_1; button < m_MouseButtons.size(); ++button)
    {
        m_MouseButtons[button] = (glfwGetMouseButton(m_Window, button) == GLFW_PRESS);
    }

    // Update mouse position
    glfwGetCursorPos(m_Window, &m_MouseX, &m_MouseY);
}

bool InputManager::IsKeyPressed(int key)
{
    if (key >= 0 && key < m_Keys.size())
    {
        return m_Keys[key];
    }
    return false;
}

bool InputManager::IsMouseButtonPressed(int button)
{
    if (button >= 0 && button < m_MouseButtons.size())
    {
        return m_MouseButtons[button];
    }
    return false;
}

void InputManager::GetMousePosition(double& x, double& y)
{
    x = m_MouseX;
    y = m_MouseY;
}