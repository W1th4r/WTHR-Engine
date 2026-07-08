#pragma once
#include "Scripts.hpp"
#include <Scene.hpp>
#include <string>
#include <vector>
#include <entt/entt.hpp>
#include "TextEditor.hpp"

struct ScriptEditor {
    ScriptEditor();
    Script* scriptManager = nullptr;
    std::string currentScript;
    std::vector<char> editableBuffer; // mutable buffer for ImGui
    std::string editorBuffer;

    ScriptEditor(Script* manager) : scriptManager(manager) {}

    void draw(entt::registry& registry, Scene& p_Scene);
    void set(Script* manager);

    void drawDebugPanel(entt::registry& registry);
private:
    void loadCurrentScript(); // helper to copy std::string to editableBuffer
    TextEditor editing;
};
