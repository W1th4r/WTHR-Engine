// scripts.cpp
#include "scripts.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <entt/entt.hpp>

Script::Script() {
    // Open standard Lua libraries
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string,
        sol::lib::table, sol::lib::package, sol::lib::os);

    // Redirect Lua print to C++ callback
    lua.set_function("print", [this](sol::variadic_args args) {
        std::string output;
        for (auto arg : args) {
            // Safe conversion using as() instead of get()
            if (arg.is<std::string>()) {
                output += arg.as<std::string>() + " ";
            }
            else {
                output += lua["tostring"](arg).get<std::string>() + " ";
            }
        }
        if (printCallback) printCallback(output);
        else std::cout << output << "\n";
        });
    lua.set_function("IsKeyPressed", [](int keycode) {
        return glfwGetKey(glfwGetCurrentContext(), keycode) == GLFW_PRESS;
        });
    lua.set("KEY_W", GLFW_KEY_W);
    lua.set("KEY_A", GLFW_KEY_A);
    lua.set("KEY_S", GLFW_KEY_S);
    lua.set("KEY_D", GLFW_KEY_D);
    lua.set("KEY_UP", GLFW_KEY_UP);
    lua.set("KEY_DOWN", GLFW_KEY_DOWN);
}

Script::~Script() {
    // Call onDestroy for all attached scripts
    for (auto& [obj, scriptNames] : objectScripts) {
        for (auto& name : scriptNames) {
            auto it = scripts.find(name);
            if (it != scripts.end()) {
                auto& instance = it->second;
                if (instance.onDestroy.valid()) {
                    try {
                        instance.onDestroy(static_cast<uint32_t>(obj));
                    }
                    catch (const sol::error& e) {
                        std::cout << "Lua onDestroy error in destructor: " << e.what() << "\n";
                    }
                }
            }
        }
    }
}

bool Script::loadScriptFile(const std::string& path, const std::string& name) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout << "Failed to open script file: " << path << "\n";
        return false;
    }

    std::string code((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    addScript(name, code);
    return true;
}

bool Script::saveScriptFile(const std::string& path, const std::string& name) {
    auto it = scripts.find(name);
    if (it == scripts.end()) return false;

    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << it->second.code;
    return true;
}

void Script::addScript(const std::string& name, const std::string& code) {
    ScriptInstance instance;
    instance.name = name;
    instance.code = code;

    try {
        // Load the script code
        lua.script(code);

        // Get the callbacks (they might not exist, which is OK)
        instance.onInit = lua[name + "_onInit"];
        instance.onUpdate = lua[name + "_onUpdate"];
        instance.onDestroy = lua[name + "_onDestroy"];

        // Store the script instance
        scripts[name] = instance;

        std::cout << "Successfully loaded script: " << name << "\n";

        // Debug: print available functions
        std::cout << "Available callbacks for " << name << ":\n";
        std::cout << "  onInit: " << (instance.onInit.valid() ? "YES" : "NO") << "\n";
        std::cout << "  onUpdate: " << (instance.onUpdate.valid() ? "YES" : "NO") << "\n";
        std::cout << "  onDestroy: " << (instance.onDestroy.valid() ? "YES" : "NO") << "\n";
    }
    catch (const sol::error& e) {
        std::cout << "Lua error in addScript '" << name << "': " << e.what() << "\n";
    }
}

void Script::removeScript(const std::string& name) {
    // Call onDestroy for all entities using this script
    for (auto& [entity, scriptNames] : objectScripts) {
        if (std::find(scriptNames.begin(), scriptNames.end(), name) != scriptNames.end()) {
            auto it = scripts.find(name);
            if (it != scripts.end() && it->second.onDestroy.valid()) {
                try {
                    it->second.onDestroy(static_cast<uint32_t>(entity));
                }
                catch (const sol::error& e) {
                    std::cout << "Lua onDestroy error in removeScript: " << e.what() << "\n";
                }
            }
        }
    }

    scripts.erase(name);

    // Remove from all attached objects
    for (auto& [obj, scriptNames] : objectScripts) {
        scriptNames.erase(
            std::remove(scriptNames.begin(), scriptNames.end(), name),
            scriptNames.end()
        );
    }
}

void Script::attachScript(const std::string& name, entt::entity entity) {
    auto it = scripts.find(name);
    if (it == scripts.end()) {
        std::cout << "Script '" << name << "' not found for attachment\n";
        return;
    }

    // Check if already attached
    auto& entityScripts = objectScripts[entity];
    if (std::find(entityScripts.begin(), entityScripts.end(), name) != entityScripts.end()) {
        std::cout << "Script '" << name << "' already attached to entity " << static_cast<uint32_t>(entity) << "\n";
        return;
    }

    entityScripts.push_back(name);

    auto& instance = it->second;
    if (instance.onInit.valid()) {
        try {
            instance.onInit(static_cast<uint32_t>(entity));
            std::cout << "Called onInit for script '" << name << "' on entity " << static_cast<uint32_t>(entity) << "\n";
        }
        catch (const sol::error& e) {
            std::cout << "Lua onInit error for script '" << name << "': " << e.what() << "\n";
        }
    }
}

void Script::detachScript(const std::string& name, entt::entity entity, entt::registry& registry) {
    auto objIt = objectScripts.find(entity);
    if (objIt == objectScripts.end()) return;

    auto& scriptNames = objIt->second;
    auto nameIt = std::find(scriptNames.begin(), scriptNames.end(), name);
    if (nameIt == scriptNames.end()) return;

    scriptNames.erase(nameIt);

    // If no scripts left for this entity, remove the entity entry
    if (scriptNames.empty()) {
        objectScripts.erase(objIt);
    }

    auto it = scripts.find(name);
    if (it != scripts.end() && it->second.onDestroy.valid()) {
        try {
            it->second.onDestroy(static_cast<uint32_t>(entity));
        }
        catch (const sol::error& e) {
            std::cout << "Lua onDestroy error: " << e.what() << "\n";
        }
    }
}

void Script::update(float dt) {
    for (auto& [entity, scriptNames] : objectScripts) {
        for (auto& name : scriptNames) {
            auto it = scripts.find(name);
            if (it == scripts.end()) continue;

            auto& instance = it->second;
            if (instance.onUpdate.valid() && instance.isActive) {
                try {
                    instance.onUpdate(static_cast<uint32_t>(entity), dt);
                }
                catch (const sol::error& e) {
                    std::cout << "Lua onUpdate error for script '" << name << "': " << e.what() << "\n";
                }
            }
        }
    }
}

void Script::bindTransforms(entt::registry& registry) {
    // 1️⃣ Expose glm::vec3 to Lua
    lua.new_usertype<glm::vec3>("Vec3",
        "x", &glm::vec3::x,
        "y", &glm::vec3::y,
        "z", &glm::vec3::z
    );

    // 2️⃣ Expose Transform type
    lua.new_usertype<Transform>("Transform",
        "position", &Transform::position,
        "rotation", &Transform::rotation,
        "scale", &Transform::scale
    );

    // 3️⃣ Helper function to get Transform from entity
    lua["getTransform"] = [&registry](uint32_t entityID) -> Transform* {
        entt::entity e = static_cast<entt::entity>(entityID);
        if (registry.valid(e) && registry.all_of<Transform>(e))
            return &registry.get<Transform>(e);
        return nullptr;
        };

    // 4️⃣ Helper function to get all entities with Transform
    lua["getEntitiesWithTransform"] = [&registry, this]() {
        sol::table t = lua.create_table();
        auto view = registry.view<Transform>();
        int i = 1;
        for (auto e : view) {
            t[i++] = static_cast<uint32_t>(e);
        }
        return t;
        };
}



void Script::setPrintCallback(std::function<void(const std::string&)> callback) {
    printCallback = callback;
}