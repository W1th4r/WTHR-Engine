#include "Editor.hpp"

void Editor::SetScene(Scene* scene)
{
	m_ActiveScene = scene;
}

void Editor::Play()
{
	if (!m_ActiveScene)
	{
		spdlog::error("Scene not loaded!");
		return;
	}
	//Start scene normally

	if (m_SceneState == State::Edit)
	{
		m_BufferReg = m_ActiveScene->GetRegistrySnapshot();
		for (auto& instance : m_ActiveScene->m_Script.scripts)
		{
			instance.second.isActive = true;
		}
		m_SceneState == State::Playing;
	}
	if (m_SceneState == State::Paused)
	{
		for (auto& instance : m_ActiveScene->m_Script.scripts)
		{
			instance.second.isActive = true;
		}
	}

	m_SceneState = State::Playing;
	//TODO same for physics

}
void Editor::Stop()
{
	if (!m_ActiveScene)
	{
		spdlog::error("Scene not loaded!");
		return;
	}

	if (m_SceneState != State::Edit)
	{
		m_ActiveScene->SetRegistry(m_BufferReg);
		for (auto& instance : m_ActiveScene->m_Script.scripts)
		{
			instance.second.isActive = false;
		}
		m_SceneState = State::Edit;
	}

}void Editor::Pause()
{
	if (!m_ActiveScene)
	{
		spdlog::error("Scene not loaded!");
		return;
	}

	if (m_SceneState == State::Playing)
	{
		for (auto& instance : m_ActiveScene->m_Script.scripts)
		{
			instance.second.isActive = false;
		}
		m_SceneState = State::Paused;
	}

}