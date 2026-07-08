#include "ScriptEditor.hpp"
#include <imgui.h>
#include <spdlog/spdlog.h>
#include <cstring>


ScriptEditor::ScriptEditor()
{
	auto lang = editing.GetLanguageDefinition().Lua();
	editing.SetLanguageDefinition(lang);
}
// Resize callback for ImGui text input
static int InputTextCallback(ImGuiInputTextCallbackData* data) {
	if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
		// Resize the vector to fit the new text
		std::vector<char>* buffer = (std::vector<char>*)data->UserData;
		buffer->resize(data->BufTextLen + 1); // +1 for null terminator
		data->Buf = buffer->data();
	}
	return 0;
}

void ScriptEditor::set(Script* sc) {
	scriptManager = sc;
	if (scriptManager && !scriptManager->scripts.empty()) {
		currentScript = scriptManager->scripts.begin()->first;
		loadCurrentScript();
	}
}

void ScriptEditor::loadCurrentScript() {
	if (scriptManager && !currentScript.empty()) {
		auto it = scriptManager->scripts.find(currentScript);
		if (it != scriptManager->scripts.end()) {
			const auto& code = it->second.code;
			// Resize buffer to fit code + null terminator
			editableBuffer.resize(code.size() + 1);
			std::memcpy(editableBuffer.data(), code.c_str(), code.size() + 1);
		}
		else {
			// Script not found, clear buffer
			editableBuffer.resize(1);
			editableBuffer[0] = '\0';
		}
	}
}

void ScriptEditor::draw(entt::registry& registry,Scene& p_Scene) {
	static char newScriptName[128] = "";
	static char loadScriptName[128] = "";
	static uint32_t entityID = 0;

	if (!scriptManager) {
		ImGui::Text("No script manager set!");
		return;
	}

	ImGui::Begin("Lua Script Editor");

	// --- Create new script ---
	ImGui::InputText("New Script Name", newScriptName, sizeof(newScriptName));
	ImGui::SameLine();
	if (ImGui::Button("Create")) {
		std::string nameStr = newScriptName;
		if (!nameStr.empty() && scriptManager->scripts.find(nameStr) == scriptManager->scripts.end()) {
			scriptManager->addScript(nameStr, "-- New script\n");
			currentScript = nameStr;
			loadCurrentScript();
			newScriptName[0] = '\0';
			p_Scene.registerLua(scriptManager->lua);
		}
	}

	ImGui::Separator();

	// --- Load existing script by filename ---
	ImGui::InputText("Load Script (no .lua)", loadScriptName, sizeof(loadScriptName));
	ImGui::SameLine();
	if (ImGui::Button("Load")) {
		std::string filename(loadScriptName);
		if (!filename.empty()) {
			std::string path = "scripts/" + filename + ".lua";
			if (scriptManager->loadScriptFile(path, filename)) {
				currentScript = filename;
				loadCurrentScript();
				p_Scene.registerLua(scriptManager->lua);
				spdlog::info("Loaded script '{}'", filename);
			}
			else {
				spdlog::error("Failed to load script '{}'", filename);
			}
		}
	}

	ImGui::Separator();


	ImGui::Begin("Scripts");

	// 1️⃣ Render the Tab Bar for scripts
	if (!scriptManager->scripts.empty()) {
		if (ImGui::BeginTabBar("ScriptEditorTabs", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs)) {

			// Loop through all loaded scripts to create tabs
			for (auto it = scriptManager->scripts.begin(); it != scriptManager->scripts.end(); ) {
				const std::string& scriptName = it->first;
				bool open = true; // Setting this allows the small "X" close button on the tab

				// Pass &open so the user can close tabs by clicking the 'X'
				if (ImGui::BeginTabItem(scriptName.c_str(), &open)) {

					// If the user clicked a different tab, switch our current selection and load it
					if (currentScript != scriptName) {
						currentScript = scriptName;
						loadCurrentScript(); // Make sure this fills editableBuffer with currentScript's code
						editorBuffer = std::string(editableBuffer.begin(), editableBuffer.end());
						editing.SetText(editorBuffer);
					}

					// --- Action Buttons ---
					if (ImGui::Button("Run Script")) {
						std::string codeStr = editing.GetText();
						codeStr.erase(std::remove(codeStr.begin(), codeStr.end(), '\r'), codeStr.end());

						// 2️⃣ Ensure there is no trailing null terminator embedded INSIDE the string data
						if (!codeStr.empty() && codeStr.back() == '\0') {
							codeStr.pop_back();
						}
						scriptManager->addScript(currentScript, codeStr);
						spdlog::info("Ran script '{}'", currentScript);
					}
					ImGui::SameLine();
					if (ImGui::Button("Save Script")) {
						std::string codeStr = editing.GetText();
						// 1. Strip Carriage Returns
						codeStr.erase(std::remove(codeStr.begin(), codeStr.end(), '\r'), codeStr.end());

						// 2. Clear out any true null-terminators or zero-width spaces leaking at the end
						while (!codeStr.empty() && (codeStr.back() == '\0' || codeStr.back() == ' ' || codeStr.back() == '\n')) {
							codeStr.pop_back();
						}
						scriptManager->addScript(currentScript, codeStr);
						std::string path = "scripts/" + currentScript + ".lua";
						if (scriptManager->saveScriptFile(path, currentScript)) {
							spdlog::info("Saved script '{}' to {}", currentScript, path);
						}
						else {
							spdlog::error("Failed to save script '{}'", currentScript);
						}
					}
					ImGui::SameLine();
					if (ImGui::Button("Remove Script")) {
						scriptManager->removeScript(currentScript);
					}
					ImGui::Separator();

					// Ensure buffer health
					if (editableBuffer.empty()) {
						editableBuffer.resize(1);
						editableBuffer[0] = '\0';
					}

					// --- Max Available Size Multiline Input ---
					ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CallbackResize;

					// ImVec2(-FLT_MIN, 0) stretches to the window edges, leaving zero wasted space at the bottom
					//if (ImGui::InputTextMultiline("##script", editableBuffer.data(), editableBuffer.size(),
					//	ImVec2(-FLT_MIN, -FLT_MIN), flags, InputTextCallback, (void*)&editableBuffer)) {
					//	// Text was modified - handled by resize callback
					//}
					editing.Render("script");

					ImGui::EndTabItem();
				}

				// Handle tab closing (if the user clicked the 'X' button on the tab)
				if (!open) {
					scriptManager->removeScript(scriptName);

					// Reset iterator safely and update current active script choice
					it = scriptManager->scripts.erase(it);
					if (!scriptManager->scripts.empty()) {
						currentScript = scriptManager->scripts.begin()->first;
						loadCurrentScript();
					}
					else {
						currentScript.clear();
						editableBuffer.clear();
					}
				}
				else {
					++it;
				}
			}
			ImGui::EndTabBar();
		}
	}
	else {
		// Fallback UI state when no scripts are open
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No scripts open. Create or load a script to begin.");
	}

	ImGui::End();
	ImGui::Separator();

	// --- Bind transforms (rebind) ---
	if (ImGui::Button("Rebind Transforms")) {
		scriptManager->bindTransforms(registry);
		spdlog::info("Rebound transforms to Lua");
	}

	// --- Attach script to entity ---
	ImGui::InputScalar("Entity ID", ImGuiDataType_U32, &entityID);
	ImGui::SameLine();
	if (ImGui::Button("Attach to Entity") && entityID != 0 && !currentScript.empty()) {
		entt::entity e = static_cast<entt::entity>(entityID);
		if (registry.valid(e)) {
			scriptManager->attachScript(currentScript, e);
			spdlog::info("Attached script '{}' to entity {}", currentScript, entityID);
		}
		else {
			spdlog::error("Entity {} is not valid", entityID);
		}
	}
	// 1. Fetch the fresh string data for this frame
	std::vector<std::string> transformLines = scriptManager->getTransformDebugList(registry);

	if (transformLines.empty()) {
		ImGui::TextColored(ImVec4(0.5, 0.5, 0.5, 1.0), "No entities with Transform components found.");
	}
	else {
		// 2. Render each string line as an ImGui text element
		for (const auto& line : transformLines) {
			ImGui::TextUnformatted(line.c_str());
		}
	}
	ImGui::End();
}

void ScriptEditor::drawDebugPanel(entt::registry& registry) {
	if (!scriptManager) return;

	ImGui::Begin("Script Debug");
	
	ImGui::Text("Loaded Scripts: %zu", scriptManager->scripts.size());
	for (auto& [name, instance] : scriptManager->scripts) {
		if (ImGui::TreeNode(name.c_str())) {
			ImGui::Checkbox("Active", &instance.isActive);
			ImGui::Text("Callbacks:");
			ImGui::Indent();
			ImGui::Text("onInit: %s", instance.onInit.valid() ? "Yes" : "No");
			ImGui::Text("onUpdate: %s", instance.onUpdate.valid() ? "Yes" : "No");
			ImGui::Text("onDestroy: %s", instance.onDestroy.valid() ? "Yes" : "No");
			ImGui::Unindent();

			// Show attached entities
			ImGui::Text("Attached to %zu entities:",
				std::count_if(scriptManager->objectScripts.begin(), scriptManager->objectScripts.end(),
					[&](const auto& pair) {
						return std::find(pair.second.begin(), pair.second.end(), name) != pair.second.end();
					}));

			ImGui::Indent();
			for (auto& [entity, scriptNames] : scriptManager->objectScripts) {
				if (std::find(scriptNames.begin(), scriptNames.end(), name) != scriptNames.end()) {
					ImGui::Text("Entity ID: %u", static_cast<uint32_t>(entity));

					// Show entity's Transform if it exists
					if (registry.all_of<Transform>(entity)) {
						auto& t = registry.get<Transform>(entity);
						ImGui::Text("  Pos: %.2f, %.2f, %.2f", t.position.x, t.position.y, t.position.z);
						ImGui::Text("  Rot: %.2f, %.2f, %.2f", t.rotation.x, t.rotation.y, t.rotation.z);
						ImGui::Text("  Scale: %.2f, %.2f, %.2f", t.scale.x, t.scale.y, t.scale.z);
					}
				}
			}
			ImGui::Unindent();

			ImGui::TreePop();
		}
	}

	ImGui::End();
}