#pragma once
#include <entt/entt.hpp>
#include "Scene.hpp"

class Editor {
public:
    
    void SetScene(Scene*);

    void Play();
    void Stop();
    void Pause();


    void SelectEntity(entt::entity e) { m_SelectedEntity = e; }
    entt::entity GetSelectedEntity() const { return m_SelectedEntity; }

    bool IsPlaying() const { return m_ActiveScene->m_SceneState == State::Playing; }

private:
 
    std::string m_SnapshotBuffer; // Your RAM snapshot
    entt::entity m_SelectedEntity = entt::null;
    Scene* m_ActiveScene;
    entt::registry m_BufferReg;
};