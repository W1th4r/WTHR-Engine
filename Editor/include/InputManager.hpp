#pragma once
#include <pch.hpp>

class InputManager
{
public:
	InputManager();
	~InputManager() = default;

	
	void Update();
	void setWindow(GLFWwindow* window) { m_Window = window; }
	
	bool IsKeyPressed(int key) const;
	bool IsMouseButtonPressed(int button) const;
	void GetMousePosition(double& x, double& y) const;

private:
	GLFWwindow* m_Window;

	// Key states
	std::unordered_map<int, bool> m_Keys;
	std::unordered_map<int, bool> m_MouseButtons;

	// Mouse position
	double m_MouseX = 0.0;
	double m_MouseY = 0.0;
};
