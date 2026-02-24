#pragma once
#include "pch.hpp"

#include <WindowManager.hpp>
#include <InputManager.hpp>
#include <Renderer.hpp>
#include <PhysicsWorld.hpp>
#include <AppUI.hpp>

class Application
{
public:
	Application();
	virtual ~Application();

	bool Init();
	void Run();
	Scene& GetScene() { return m_ActiveScene; }
	Renderer& GetRenderer() { return m_Renderer; }

	inline static bool isFocused = false;
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
	AppUI m_UI;
	Scene m_ActiveScene;
};
