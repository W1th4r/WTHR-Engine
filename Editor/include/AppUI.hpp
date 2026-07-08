#pragma once
#include <imgui.h>
#include "Editor.hpp"
#include <ScriptEditor.hpp>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <shobjidl.h> 
#elif defined(__linux__)
#include <X11/Xlib.h>
// Undefine the most annoying macros
#undef None
#undef Status
#undef Bool
#endif

// Forward declare your app state/settings to keep includes light
struct AppSettings;
class Renderer;
class Scene;
class InputManager;
class WindowManager;
class ScriptEditor;

class FileDialog
{
public:
	static std::filesystem::path OpenFile();
	static std::filesystem::path OpenFolder();
};


class AppUI {
public:
	void Initialize(Scene& p_ActiveScene, Renderer& p_Renderer, WindowManager& p_WindowManagar);
	void NewFrame();

	// This is where your 70% of code goes!
	void Update();

	void Render();
	void Shutdown();

private:
	// Helper methods to break up the 700 lines into chunks
	void RenderEcs();
	void RenderInspectors();
	void RenderScripts();
	void RenderObjectInspector();
	void DrawMenuBar();
	Scene* m_ActiveScene;
	Renderer* m_Renderer;
	InputManager* m_Input; 
	WindowManager* m_WindowManager;
	Editor m_Editor;
	ScriptEditor m_Scripts;

};