#pragma once
#include "pch.hpp"

#include <WindowManager.hpp>
#include <InputManager.hpp>
#include <Renderer.hpp>
#include <PhysicsWorld.hpp>

class Application
{
public:
	Application();
	virtual ~Application();

	bool Init();
	void Run();

	static bool isFocused;
protected:
	virtual void Update();  
	virtual void Render();  

	void InitEngineSystems();
	void InitImGui();
	void Shutdown();

protected:
	WindowManager m_WindowManager;
	InputManager m_Input;
	Renderer m_Renderer;
	PhysicsWorld m_World;
};
