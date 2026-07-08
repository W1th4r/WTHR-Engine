#pragma once
#include <pch.hpp>
#include <array>

class InputManager
{
public:
    // Disallow creating instances of this class
    InputManager() = delete;

    // Static setup and per-frame update
    static void SetWindow(GLFWwindow* window) { m_Window = window; }
    static void Update();

    // Pure static query functions
    static bool IsKeyPressed(int key);
    static bool IsMouseButtonPressed(int button);
    static void GetMousePosition(double& x, double& y);

private:
    static GLFWwindow* m_Window;

    // Pure static storage arrays
    static std::array<bool, 350> m_Keys;
    static std::array<bool, 8> m_MouseButtons;

    static double m_MouseX;
    static double m_MouseY;
};