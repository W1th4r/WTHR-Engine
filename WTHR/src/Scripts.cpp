// scripts.cpp
#include "scripts.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <entt/entt.hpp>
#include <InputManager.hpp>
#include <entt/core/hashed_string.hpp>
#include "../../vendor/entt/single_include/entt/entt.hpp"
#include <entt/meta/meta.hpp>
// This brings the _hs user-defined literal into scope
using namespace entt::literals;

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


	lua.set_function("isKeyPressed", [this](int key) {
		return InputManager::IsKeyPressed(key);
		});

	lua.set_function("isMouseButtonPressed", [this](int button) {
		return InputManager::IsMouseButtonPressed(button);
		});

	lua.set_function("getMousePosition", [this]() {
		double x, y;
		InputManager::GetMousePosition(x, y);
		return glm::vec2{ (float)x, (float)y };
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

void Script::restartScripts()
{

	//Todo swap hardcoded 1 with the enitity id
	for (auto& script : scripts)
	{
		try {
			if (script.second.onInit.valid())
			{
				std::cout << "Called onInit on script \"" << script.first << "\"\n";
				script.second.onInit(1);
			}
			else
			{
				std::cout << "Lua onInit error for script '" << script.first << "': " << " script is not valid" << "\n";

			}

		}
		catch (const sol::error& e) {
			std::cout << "Lua onInit error for script '" << script.first << "': " << e.what() << "\n";
		}
	}

}

//Todo fix onInit should be based on editor mode
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

  //  lua = sol::state();
  //  lua.open_libraries(sol::lib::base); // Re-open standard libs if needed
	//TODO Try if scripts don't work and bug when doind bindTransforms



	lua.new_usertype<glm::vec3>("Vec3",
		sol::constructors<glm::vec3(), glm::vec3(float, float, float)>(),
		"x", &glm::vec3::x,
		"y", &glm::vec3::y,
		"z", &glm::vec3::z
	);;

	// 2️⃣ Expose Transform type
	lua.new_usertype<Transform>("Transform",
		"position", &Transform::position,
		"rotation", &Transform::rotation,
		"scale", &Transform::scale
	);

	// 3️⃣ Helper function to get Transform from entity
	lua["getTransform"] = [&](uint32_t entityID) -> Transform* {
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
	lua["getTransform"] = [&registry, this](uint32_t entityID) -> Transform* {
		entt::entity e = static_cast<entt::entity>(entityID);
		if (registry.valid(e) && registry.all_of<Transform>(e)) {
			// Track that Lua now has access to this specific entity
			m_BoundEntities.insert(entityID);
			return &registry.get<Transform>(e);
		}
		return nullptr;
		};

	// 4️⃣ Helper function to get all entities with Transform
	lua["getEntitiesWithTransform"] = [&registry, this]() {
		sol::table t = lua.create_table();
		auto view = registry.view<Transform>();
		int i = 1;
		for (auto e : view) {
			uint32_t id = static_cast<uint32_t>(e);
			t[i++] = id;

			// Track that these were exposed to Lua via the view array
			m_BoundEntities.insert(id);
		}
		return t;
		};
}

// Assuming this is inside your Editor or Script class where the registry is accessible
std::vector<std::string> Script::getTransformDebugList(entt::registry& registry) {
	std::vector<std::string> debugList;

	for (uint32_t entityID : m_BoundEntities) {
		entt::entity e = static_cast<entt::entity>(entityID);

		// Verify the entity is still alive and has its transform
		if (registry.valid(e) && registry.all_of<Transform>(e)) {
			const auto& transform = registry.get<Transform>(e);

			std::string line = std::format(
				"ID: {:04d} | Pos: ({:.2f}, {:.2f}, {:.2f}) | Rot: ({:.2f}, {:.2f}, {:.2f}) | Scale: ({:.2f}, {:.2f}, {:.2f})",
				entityID,
				transform.position.x, transform.position.y, transform.position.z,
				transform.rotation.x, transform.rotation.y, transform.rotation.z,
				transform.scale.x, transform.scale.y, transform.scale.z
			);

			debugList.push_back(line);
		}
		else {
			// Optional: The entity was destroyed or component removed.
			// You could flag it here for your editor rebind UI:
			debugList.push_back(std::format("ID: {:04d} | [STALE/DISCONNECTED]", entityID));
		}
	}

	return debugList;
}
void Script::setPrintCallback(std::function<void(const std::string&)> callback) {
	printCallback = callback;
}