// scripts.hpp
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <sol/sol.hpp>
#include <Components.hpp>
#include <entt/entt.hpp>

struct ScriptInstance {
    ScriptInstance() { isActive = false; }
    std::string name;
    std::string code;
    sol::function onUpdate;       // called every frame
    sol::function onInit;         // called on attach
    sol::function onDestroy;      // called on detach
    bool isActive;
};

class Script {
public:
    Script();
    ~Script();

    // Load / save scripts from file
    bool loadScriptFile(const std::string& path, const std::string& name);
    bool saveScriptFile(const std::string& path, const std::string& name);

    // Add / remove scripts dynamically
    void addScript(const std::string& name, const std::string& code);
    void removeScript(const std::string& name);


    // Attach scripts to objects
    void attachScript(const std::string& name, entt::entity entity);
    void detachScript(const std::string& name, entt::entity entity, entt::registry& registry);

    // Run all scripts per-frame
    void update(float dt);

    // Bind engine API / objects
    void bindTransforms(entt::registry& registry);

    // Optional: redirect Lua print
    void setPrintCallback(std::function<void(const std::string&)> callback);

    sol::state lua;
    std::function<void(const std::string&)> printCallback;

    // Store all script sources, filepath, script
    std::unordered_map<std::string, ScriptInstance> scripts;

    // Track which scripts are attached to which objects
    std::unordered_map<entt::entity, std::vector<std::string>> objectScripts;
};
