#include "AppUI.hpp"
#include "Renderer.hpp"
#include "Scene.hpp"
#include "InputManager.hpp"
#include "WindowManager.hpp"
#include "Application.hpp"
#include <chrono>



std::filesystem::path FileDialog::OpenFile()
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr))
	{
		IFileOpenDialog* pFileOpen;
		std::filesystem::path path;
		// Create the FileOpenDialog object
		hr = CoCreateInstance(__uuidof(FileOpenDialog), NULL, CLSCTX_ALL,
			__uuidof(IFileOpenDialog), reinterpret_cast<void**>(&pFileOpen));

		if (SUCCEEDED(hr))
		{
			// Show the Open dialog box
			hr = pFileOpen->Show(NULL);

			if (SUCCEEDED(hr))
			{
				IShellItem* pItem;
				// Get the selection from the dialog
				hr = pFileOpen->GetResult(&pItem);

				if (SUCCEEDED(hr))
				{
					PWSTR pszFilePath;
					// Get the file path
					hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

					if (SUCCEEDED(hr))
					{
						// Use the file path (example: print to console)
						std::wcout << L"Selected File: " << pszFilePath << std::endl;
						path = std::filesystem::path(pszFilePath);
						CoTaskMemFree(pszFilePath); // Free the memory allocated for the path
					}
					pItem->Release(); // Release the item pointer
				}
			}
			pFileOpen->Release(); // Release the dialog pointer
		}
		CoUninitialize(); // Uninitialize COM
		return path;
	}
	return std::filesystem::path();
}

std::filesystem::path FileDialog::OpenFolder()
{
	std::filesystem::path resultPath;

	// 1. Initialize COM (Necessary for Shell objects)
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(hr)) return resultPath;

	IFileOpenDialog* pFileOpen;

	// 2. Create the FileOpenDialog object
	hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
		IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

	if (SUCCEEDED(hr))
	{
		// 3. Set the options to "Pick Folders" instead of files
		DWORD dwOptions;
		if (SUCCEEDED(pFileOpen->GetOptions(&dwOptions)))
		{
			pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS);
		}

		// 4. Show the Dialog
		hr = pFileOpen->Show(NULL);

		// 5. Process the result if the user didn't hit cancel
		if (SUCCEEDED(hr))
		{
			IShellItem* pItem;
			hr = pFileOpen->GetResult(&pItem);
			if (SUCCEEDED(hr))
			{
				PWSTR pszFilePath;
				hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

				if (SUCCEEDED(hr))
				{
					resultPath = std::filesystem::path(pszFilePath);
					CoTaskMemFree(pszFilePath); // Important: Free the COM string
				}
				pItem->Release();
			}
		}
		pFileOpen->Release();
	}

	CoUninitialize();
	return resultPath;
}



int nextGroupId = 0; // keep track somewhere in your editor state
static void DrawEntity(entt::registry& registry, entt::entity e, Scene& scene)
{

	ImGuiTreeNodeFlags flags =
		ImGuiTreeNodeFlags_Framed |
		ImGuiTreeNodeFlags_OpenOnArrow |
		ImGuiTreeNodeFlags_DrawLinesFull |
		ImGuiTreeNodeFlags_DefaultOpen;

	bool opened = ImGui::TreeNodeEx(
		(void*)(uint32_t)e,
		flags,
		"Entity %u##%u", // visible text | unique ID
		(uint32_t)e,     // visible number
		(uint32_t)e      // unique ID after ##
	);


	if (opened)
	{

		auto& all_Textures = scene.GetTextures();

		uint32_t entity = (uint32_t)e;
		MeshComponent& meshComp = registry.get<MeshComponent>(e);

		ImGui::Text("Textures");
		if (ImGui::Button(("+ Texture##" + std::to_string((int)entity)).c_str()))
		{
			ImGui::OpenPopup(("Set Texture##" + std::to_string((int)entity)).c_str());
		}
		if (ImGui::BeginPopupModal(("Set Texture##" + std::to_string((int)entity)).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			static char filepath[256] = "";
			ImGui::InputText("File Path", filepath, IM_ARRAYSIZE(filepath));

			if (ImGui::Button("Load"))
			{
				std::string targetFilename = std::filesystem::path(filepath).filename().string();
				bool found = false;

				// Iterate through the map to find a key whose filename matches our target
				for (auto const& [absPath, textureObj] : all_Textures)
				{
					if (std::filesystem::path(absPath).filename().string() == targetFilename)
					{

						meshComp.mesh->mesh.textures.push_back(textureObj);
						registry.emplace<Texture>(e);

						found = true;
						ImGui::CloseCurrentPopup();
						break;
					}
				}

				if (!found)
				{
					std::cout << "Texture filename not found: " << targetFilename << std::endl;
				}
			}

			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				filepath[0] = '\0';
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
			ImGui::SameLine();
		}
		if (ImGui::Button(("- Texture##" + std::to_string((int)entity)).c_str()))
		{
			auto& vecTextures = meshComp.mesh->mesh.textures;
			registry.remove<Texture>(e);
			vecTextures.clear();
		}
		for (auto& texture : meshComp.mesh->mesh.textures)
		{
			// Loop through the map to find the matching Texture
			for (auto& [name, tex] : all_Textures)
			{
				if (tex.id == texture.id) // assuming Texture has a unique 'id' or comparable field
				{
					ImGui::Bullet();
					ImGui::Text("%s", name.c_str());
					break; // stop after finding the first match
				}
			}
		}
		if (registry.any_of<Transform>(e))
		{

			ImGui::Text("Transforms");
			ImGui::DragFloat3("Position", &registry.get<Transform>(e).position.x, 0.1f);
			ImGui::DragFloat3("Scale", &registry.get<Transform>(e).scale.x, 0.1f);
			ImGui::DragFloat3("Rotation", &registry.get<Transform>(e).rotation.x, 0.1f);
		}
		if (registry.any_of<Color>(e))
		{
			ImGui::Text("Color");
			ImGui::ColorEdit4("RGB", &registry.get<Color>(e).value.r);
			if (ImGui::Button("Remove Color"))
			{
				registry.remove<Color>(e);
			}

		}
		else if (ImGui::Button("Add Color"))
		{
			registry.emplace<Color>(e, glm::vec4(0.f));
		}


		if (registry.any_of<Camera>(e))
		{
			ImGui::Text("Camera");
			ImGui::SliderFloat3("Camera", &registry.get<Camera>(e).Position.x, 0, 100);
			if (ImGui::Button("Remove Camera"))
			{
				registry.remove<Camera>(e);
			}
			ImGui::SameLine();
			if (ImGui::Button("Switch to Camera"))
			{
				scene.setCameraType(CameraType::Player);
			}

		}
		else if (ImGui::Button("Add Camera"))
		{
			registry.emplace<Camera>(e, glm::vec4(0.f));
		}





		if (registry.any_of<RigidBody>(e))
		{
			ImGui::Text("RigidBody");
			ImGui::DragFloat3("Position##rigidbody", &registry.get<RigidBody>(e).position.x, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat3("Velocity", &registry.get<RigidBody>(e).velocity.x, 0.01f, 0.0f, 1.0f);
			ImGui::Checkbox("IsKinematic", &registry.get<RigidBody>(e).isKinematic);
			ImGui::Checkbox("useGravity", &registry.get<RigidBody>(e).useGravity);

			if (ImGui::Button("Remove RigidBody"))
			{
				registry.remove<RigidBody>(e);
			}
		}
		else if (ImGui::Button("Add RigidBody"))
		{
			auto trans = registry.get<Transform>(e).position;

			registry.emplace<RigidBody>(e, RigidBodyDesc(trans));
		}




		if (registry.any_of<Collider>(e))
		{
			ImGui::Text("Collider");
			ImGui::DragFloat3("Position##collider", &registry.get<Collider>(e).offset.x, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat3("Size", &registry.get<Collider>(e).size.x, 0.01f, 0.0f, 1.0f);

			if (ImGui::Button("Remove Collider"))
			{
				registry.remove<Collider>(e);
			}
		}
		else if (ImGui::Button("Add Collider"))
		{
			registry.emplace<Collider>(e, ColliderDesc());
		}

		if (auto group = registry.try_get<GroupComponent>(e))
		{
			ImGui::InputInt("Group", &group->id);
			if (group->id == -1)
				registry.remove<GroupComponent>(e);
		}
		else if (ImGui::Button("Add group"))
		{
			registry.emplace<GroupComponent>(e, /* optional initial id */ 0);
		}

		ImGui::TreePop();
	}
}


void AppUI::Initialize(Scene& p_ActiveScene, Renderer& p_Renderer,
	InputManager& p_Input, WindowManager& p_WindowManagar)
{
	m_ActiveScene = &p_ActiveScene;
	m_Renderer = &p_Renderer;
	m_Input = &p_Input;
	m_WindowManager = &p_WindowManagar;

	m_Editor.SetScene(m_ActiveScene);
}
static entt::entity m_SelectedEntity;

void AppUI::Update()
{
	DrawMenuBar();


	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;

	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0)); // transparent background



	// Start full-screen dockspace host window
	ImGui::Begin("DockSpace Window", nullptr, window_flags);
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(3);

	// Create the actual DockSpace
	ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

	ImGui::PushID("ToolbarButtons");
	{
		float buttonWidth = 100.0f;
		float totalWidth = (buttonWidth * 3) + (ImGui::GetStyle().ItemSpacing.x * 2);
		ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - totalWidth) * 0.5f, 5.0f));

		if (ImGui::Button("Play", { buttonWidth, 20 })) { m_Editor.Play(); }
		ImGui::SameLine();
		if (ImGui::Button("Stop", { buttonWidth, 20 })) { m_Editor.Stop(); }
		ImGui::SameLine();
		if (ImGui::Button("Pause", { buttonWidth, 20 })) { m_Editor.Pause(); }
	}
	ImGui::PopID();
	extern std::chrono::steady_clock::time_point lastFrame;
	auto currentFrame = std::chrono::high_resolution_clock::now();
	float deltaTime = std::chrono::duration<float>(currentFrame - lastFrame).count();

	auto& registry = m_ActiveScene->GetRegistry();


	if (m_ActiveScene->GetCameraType() == CameraType::Editor)
	{
		float speedMultiplier = 1.0f;
		// Check if Left or Right Shift is held
		speedMultiplier = 10.f;
		if (glfwGetKey(m_WindowManager->GetWindow(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
			glfwGetKey(m_WindowManager->GetWindow(), GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
		{
			speedMultiplier *= 2.0f; // 100% base + 200% added = 300% total (3x speed)
		}
		float finalDelta = deltaTime * speedMultiplier;

		if (glfwGetKey(m_WindowManager->GetWindow(), GLFW_KEY_W) == GLFW_PRESS)
			m_ActiveScene->GetCamera().ProcessKeyboard(FORWARD, finalDelta);
		if (glfwGetKey(m_WindowManager->GetWindow(), GLFW_KEY_S) == GLFW_PRESS)
			m_ActiveScene->GetCamera().ProcessKeyboard(BACKWARD, finalDelta);
		if (glfwGetKey(m_WindowManager->GetWindow(), GLFW_KEY_A) == GLFW_PRESS)
			m_ActiveScene->GetCamera().ProcessKeyboard(LEFT, finalDelta);
		if (glfwGetKey(m_WindowManager->GetWindow(), GLFW_KEY_D) == GLFW_PRESS)
			m_ActiveScene->GetCamera().ProcessKeyboard(RIGHT, finalDelta);
	}


	if (m_ActiveScene->GetCameraType() == CameraType::Editor)
	{

		if (m_Input->IsMouseButtonPressed(GLFW_MOUSE_BUTTON_1))
		{
			double x, y;
			glfwGetCursorPos(m_WindowManager->GetWindow(), &x, &y);
			m_Renderer->RenderPicking(*m_ActiveScene, x, y);
		}

		if (m_Input->IsMouseButtonPressed(GLFW_MOUSE_BUTTON_1))
		{
			double x, y;
			glfwGetCursorPos(m_WindowManager->GetWindow(), &x, &y);
			m_Renderer->HandlePickingClick(*m_ActiveScene, x, y, m_SelectedEntity);
		}
	}
	else if (m_ActiveScene->GetCameraType() == CameraType::Player)
	{
		if (m_Input->IsMouseButtonPressed(GLFW_MOUSE_BUTTON_1))
		{
			//shoot bullet
		//	m_ActiveScene->CreateBullet();
		}
	}
	static bool firstMouse = false;
	static double lastX;
	static double lastY;
	if (m_Input->IsKeyPressed(GLFW_KEY_ESCAPE))
	{
		Application::isFocused = !Application::isFocused;

		if (Application::isFocused)
		{
			double xpos, ypos;
			glfwGetCursorPos(m_WindowManager->GetWindow(), &xpos, &ypos);
			lastX = static_cast<float>(xpos);
			lastY = static_cast<float>(ypos);
			firstMouse = true; // reset to avoid huge jump
		}
	}



	if (glfwGetKey(m_WindowManager->GetWindow(), GLFW_KEY_Q) == GLFW_PRESS)
	{
		m_Renderer->gizmoType = ImGuizmo::OPERATION::TRANSLATE;
	}
	else if (glfwGetKey(m_WindowManager->GetWindow(), GLFW_KEY_W) == GLFW_PRESS)
	{
		m_Renderer->gizmoType = ImGuizmo::OPERATION::ROTATE;
	}
	else if (glfwGetKey(m_WindowManager->GetWindow(), GLFW_KEY_E) == GLFW_PRESS)
	{
		m_Renderer->gizmoType = ImGuizmo::OPERATION::SCALE;
	}
	else if (glfwGetKey(m_WindowManager->GetWindow(), GLFW_KEY_R) == GLFW_PRESS)
	{
		m_Renderer->gizmoType = ImGuizmo::OPERATION::SCALEU; // uniform scale
	}


	if (glfwGetKey(m_WindowManager->GetWindow(), GLFW_KEY_1) == GLFW_PRESS)
	{
		m_ActiveScene->setCameraType(CameraType::Editor);
	}
	else if (glfwGetKey(m_WindowManager->GetWindow(), GLFW_KEY_2) == GLFW_PRESS)
	{
		m_ActiveScene->setCameraType(CameraType::Player);
	}

	if (Application::isFocused)
	{
		double xposIn, yposIn;
		glfwGetCursorPos(m_WindowManager->GetWindow(), &xposIn, &yposIn);
		float xpos = static_cast<float>(xposIn);
		float ypos = static_cast<float>(yposIn);

		if (firstMouse)
		{
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}

		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

		lastX = xpos;
		lastY = ypos;

		m_ActiveScene->GetCamera().ProcessMouseMovement(xoffset, yoffset);

		glfwSetInputMode(m_WindowManager->GetWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
	else
	{
		glfwSetInputMode(m_WindowManager->GetWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}




	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoInputs;

	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos);
	ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);

	ImGui::Begin("GizmoOverlay", nullptr, flags);

	if (ImGui::BeginPopupContextWindow("Context"))
	{
		if (ImGui::BeginMenu("Spawn"))
		{
			if (ImGui::BeginMenu("Shapes"))
			{
				if (ImGui::BeginMenu("Cube"))
				{
					if (ImGui::MenuItem("Single")) m_ActiveScene->CreateCube();
					if (ImGui::MenuItem("Grid 10x10x10")) m_ActiveScene->CreateCubeGrid(10, 10, 10);
					if (ImGui::MenuItem("Grid 10x10x1")) m_ActiveScene->CreateCubeGrid(10, 1, 10);

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Sphere"))
				{
					if (ImGui::MenuItem("Single")) m_ActiveScene->CreateSphere();
					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Entities"))
			{
				if (ImGui::MenuItem("Player")) m_ActiveScene->CreatePlayer();

				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}

		ImGui::EndPopup();
	}
}

void AppUI::Render()
{


	DrawEntPanel();

	DrawPropertiesPanel();

	ImGui::Separator();

	DrawECSPanel();
	DrawTextureInspector();

	DrawModelPanel();

	//ImGui::End(); // end dockspace
}
void AppUI::DrawEntPanel()
{
	auto& registry = m_ActiveScene->GetRegistry();
	std::filesystem::path path = std::filesystem::current_path().concat("\\Default.sce");

	// Modern panel with better organization
	ImGui::Begin("Entity Manager", nullptr, ImGuiWindowFlags_NoCollapse);

	// --- TOP TOOLBAR WITH ICONS ---
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));

	// Scene operations group
	ImGui::BeginGroup();
	ImGui::TextDisabled("SCENE");
	ImGui::SameLine(60);

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 4));

	if (ImGui::Button("󰆓 Save", ImVec2(70, 24))) {
		m_ActiveScene->Save(path);
	}
	ImGui::SameLine();
	if (ImGui::Button("󰁿 Load", ImVec2(70, 24))) {
		m_ActiveScene->Load(path);
	}
	ImGui::SameLine();
	if (ImGui::Button("󰧧 Clear", ImVec2(70, 24))) {
		registry.clear();
		registry = entt::registry();
	}
	ImGui::EndGroup();

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Entity operations group
	ImGui::BeginGroup();
	ImGui::TextDisabled("ENTITIES");
	ImGui::SameLine(60);

	// Create entity dropdown with multiple options
	if (ImGui::Button("󰩍 Create", ImVec2(80, 24))) {
		ImGui::OpenPopup("CreateEntityPopup");
	}
	ImGui::SameLine();

	if (ImGui::Button("󰆴 Delete", ImVec2(80, 24))) {
		if (registry.valid(m_SelectedEntity)) {
			registry.destroy(m_SelectedEntity);
			m_SelectedEntity = entt::null;
		}
	}
	ImGui::SameLine();

	// Duplicate button (new feature)
	if (ImGui::Button("󰆞 Duplicate", ImVec2(80, 24))) {
		if (registry.valid(m_SelectedEntity)) {
			// Duplicate entity logic here
			// This would copy all components to a new entity
		}
	}
	ImGui::EndGroup();

	ImGui::PopStyleVar();
	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar();

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// --- ENTITY CREATION POPUP (MODERN GRID) ---
	if (ImGui::BeginPopup("CreateEntityPopup"))
	{
		ImGui::TextDisabled("Create New Entity");
		ImGui::Separator();

		// Search bar for filtering
		static char searchBuffer[64] = "";
		ImGui::InputTextWithHint("##search", "󰍉 Search primitives...", searchBuffer, IM_ARRAYSIZE(searchBuffer));
		ImGui::Separator();

		// Grid of primitive buttons
		ImGui::BeginGroup();

		// Row 1
		if (ImGui::Button("󰊄 Cube", ImVec2(100, 40))) {
			m_ActiveScene->CreateCube();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("󰊄 Sphere", ImVec2(100, 40))) {
			m_ActiveScene->CreateSphere();
			ImGui::CloseCurrentPopup();
		}

		// Row 2
		if (ImGui::Button("󰊄 Plane", ImVec2(100, 40))) {
			// m_ActiveScene->CreatePlane();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("󰊄 Cylinder", ImVec2(100, 40))) {
			// m_ActiveScene->CreateCylinder();
			ImGui::CloseCurrentPopup();
		}

		// Row 3
		if (ImGui::Button("󰊄 Camera", ImVec2(100, 40))) {
			auto entity = registry.create();
			registry.emplace<Transform>(entity);
			registry.emplace<Camera>(entity, glm::vec4(0.f));
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("󰊄 Light", ImVec2(100, 40))) {
			// m_ActiveScene->CreateLight();
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndGroup();

		ImGui::Separator();

		// Advanced options
		if (ImGui::BeginMenu("Advanced Grids"))
		{
			if (ImGui::MenuItem("Cube Grid 10x10x10")) {
				m_ActiveScene->CreateCubeGrid(10, 10, 10);
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::MenuItem("Cube Grid 10x10x1")) {
				m_ActiveScene->CreateCubeGrid(10, 1, 10);
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::MenuItem("Sphere Grid 5x5")) {
				// m_ActiveScene->CreateSphereGrid(5, 5);
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndMenu();
		}

		ImGui::EndPopup();
	}

	// --- GROUP MANAGEMENT SECTION (COLLAPSIBLE) ---
	if (ImGui::CollapsingHeader("󰋃 Group Management", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent(8);

		// Group creation with range
		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled("Create Group from Range");
		ImGui::Spacing();
		int a, b;
		// Range inputs with better layout
		ImGui::PushItemWidth(80);
		ImGui::InputInt("##from", &a, 0);
		ImGui::SameLine();
		ImGui::Text("→");
		ImGui::SameLine();
		ImGui::InputInt("##to", &b, 0);
		ImGui::PopItemWidth();

		ImGui::SameLine();
		if (ImGui::Button("Create Group", ImVec2(100, 24))) {
			ImGui::OpenPopup("Group Creation Confirmation");
		}

		// Group creation confirmation modal
		if (ImGui::BeginPopupModal("Group Creation Confirmation", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Create group with entities %d through %d?", a, b);
			ImGui::Separator();

			if (ImGui::Button("✓ Confirm", ImVec2(120, 0))) {
				if (a > b) std::swap(a, b);
				int groupId = nextGroupId++;

				registry.view<MeshComponent>().each([&](auto e, MeshComponent& mesh) {
					uint32_t id = (uint32_t)e;
					if (id >= a && id <= b) {
						if (registry.any_of<GroupComponent>(e))
							registry.get<GroupComponent>(e).id = groupId;
						else
							registry.emplace<GroupComponent>(e, groupId);
					}
					});

				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button("✗ Cancel", ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::Spacing();

		// Existing groups list
		auto view = registry.view<GroupComponent>();
		if (!view.empty()) {
			ImGui::TextDisabled("Active Groups");

			ImGui::BeginChild("GroupsList", ImVec2(0, 100), true);

			std::unordered_map<int, int> groupCounts;
			view.each([&](auto e, auto& g) {
				groupCounts[g.id]++;
				});

			for (auto& [groupId, count] : groupCounts) {
				ImGui::Bullet();
				ImGui::Text("Group %d", groupId);
				ImGui::SameLine(100);
				ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%d entities", count);

				ImGui::SameLine(ImGui::GetWindowWidth() - 60);
				ImGui::PushID(groupId);
				if (ImGui::SmallButton("Select")) {
					// Select all entities in this group
				}
				ImGui::PopID();
			}

			ImGui::EndChild();
		}

		ImGui::Unindent(8);
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// --- ENTITY COUNTER AND STATS ---
	auto meshView = registry.view<MeshComponent>();
	auto totalEntities = registry.storage<entt::entity>().size();
	auto selectedCount = registry.valid(m_SelectedEntity) ? 1 : 0;

	ImGui::BeginGroup();
	ImGui::TextDisabled("Statistics");
	ImGui::SameLine(100);

	ImGui::Text("Total: %zu  |  Selected: %d  |  Meshes: %zu",
		totalEntities, selectedCount, meshView.size());
	ImGui::EndGroup();

	// --- QUICK SELECTION BUTTONS ---
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::TextDisabled("Quick Select");
	ImGui::SameLine(100);

	if (ImGui::SmallButton("All")) {
		// Select first entity
		auto view = registry.view<MeshComponent>();
		if (!view.empty()) {
			m_SelectedEntity = *view.begin();
		}
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("None")) {
		m_SelectedEntity = entt::null;
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("Invert")) {
		// Invert selection logic
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("By Type...")) {
		ImGui::OpenPopup("SelectByTypePopup");
	}

	// Select by type popup
	if (ImGui::BeginPopup("SelectByTypePopup"))
	{
		ImGui::Text("Select all entities with:");
		ImGui::Separator();

		if (ImGui::MenuItem("MeshComponent")) {
			auto view = registry.view<MeshComponent>();
			if (!view.empty()) m_SelectedEntity = *view.begin();
		}
		if (ImGui::MenuItem("RigidBody")) {
			auto view = registry.view<RigidBody>();
			if (!view.empty()) m_SelectedEntity = *view.begin();
		}
		if (ImGui::MenuItem("Camera")) {
			auto view = registry.view<Camera>();
			if (!view.empty()) m_SelectedEntity = *view.begin();
		}

		ImGui::EndPopup();
	}

}
void AppUI::DrawECSPanel()
{
	auto& registry = m_ActiveScene->GetRegistry();
	if (ImGui::TreeNode("Scene Hierarchy"))
	{
		std::unordered_map<int, std::vector<entt::entity>> groups;

		// Collect group members
		registry.view<GroupComponent>().each([&](auto e, auto& g) {
			groups[g.id].push_back(e);
			});

		// Draw groups
		for (auto& [groupId, members] : groups)
		{
			ImGuiTreeNodeFlags groupFlags =
				ImGuiTreeNodeFlags_DefaultOpen |
				ImGuiTreeNodeFlags_DrawLinesFull |
				ImGuiTreeNodeFlags_SpanAvailWidth |
				ImGuiTreeNodeFlags_OpenOnArrow |
				ImGuiTreeNodeFlags_OpenOnDoubleClick;

			bool groupOpened = ImGui::TreeNodeEx(
				(void*)(intptr_t)(groupId + 0x100000), // offset to differentiate groups
				groupFlags,
				"Group##%u",
				groupId
			);
			if (groupOpened)
			{

				// Ensure it has the components
				// Draw ImGui sliders for the master
				static RigidBody rb = (RigidBodyDesc());
				static Collider col = (ColliderDesc());
				static Color color = Color(glm::vec4(1.f));
				static Transform trans;

				// Check if any member is missing a component
				bool anyMissingRigidBody = false;
				bool anyMissingCollider = false;
				bool anyMissingColor = false;

				for (auto e : members)
				{
					if (!registry.any_of<RigidBody>(e)) anyMissingRigidBody = true;
					if (!registry.any_of<Collider>(e)) anyMissingCollider = true;
					if (!registry.any_of<Color>(e)) anyMissingColor = true;
				}

				// Render buttons if something is missing
				if (anyMissingRigidBody && ImGui::Button("Add RigidBody to missing members"))
				{
					for (auto e : members)
					{
						if (!registry.any_of<RigidBody>(e))
						{
							registry.emplace<RigidBody>(e, RigidBodyDesc(registry.get<Transform>(e).position)); // use current group master values

						}
					}
				}

				if (anyMissingCollider && ImGui::Button("Add Collider to missing members"))
				{
					for (auto e : members)
					{
						if (!registry.any_of<Collider>(e))
							registry.emplace<Collider>(e, col); // use current group master values
					}
				}
				if (anyMissingColor && ImGui::Button("Add Color to missing members"))
				{
					for (auto e : members)
					{
						if (!registry.any_of<Color>(e))
							registry.emplace<Color>(e, color); // use current group master values
					}
				}

				// Draw sliders for group master values
				ImGui::Checkbox("useGravity", &rb.useGravity);
				ImGui::Checkbox("isKinematic", &rb.isKinematic);
				ImGui::SliderFloat3("Position", &trans.position.x, -1.f, 1.f);
				ImGui::ColorEdit4("Color", &color.value.r);
				//	ImGui::DragFloat3("Group RigidBody", &rb.position.x, 0.1f);
				//	ImGui::DragFloat3("Group Collider", &col.size.x, 0.1f);

					// Propagate to all members that already have the components
				static Transform lastT;

				for (auto e : members)
				{
					if (registry.any_of<Transform>(e))
					{
						if (lastT.position != trans.position)
						{

							registry.get<Transform>(e).position = registry.get<Transform>(e).position + trans.position;

						}
					}
					if (registry.any_of<RigidBody>(e))
					{
						registry.get<RigidBody>(e).useGravity = rb.useGravity;
					}
					if (registry.any_of<Color>(e))
					{
						registry.get<Color>(e).value = color.value;
					}

					if (registry.any_of<Collider>(e))
					{

					}
				}
				lastT = trans;

				// Draw members WITHOUT indentation
				ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
				ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
				ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));


				for (auto e : members)
					DrawEntity(registry, e, *m_ActiveScene);

				ImGui::PopStyleColor(3);

				ImGui::TreePop();
			}
		}

		// Draw entities without a group (no indentation)
		registry.view<MeshComponent>(entt::exclude<GroupComponent>).each([&](auto e, MeshComponent& mesh) {
			DrawEntity(registry, e, *m_ActiveScene);
			});

		ImGui::TreePop();
	}

	//MODEL COMPONENTS
	registry.view<ModelComponent, Transform>().each([&](auto entity, auto& modelComp, auto& transform) {
		ImGui::Text("Current Components for entity %d", (int)entity); // optional display


		if (registry.any_of<Transform>(entity))
		{
			ImGui::Text("Transforms");
			// Encode entity into ID by using "##" suffix
	// Create unique labels using "##" + entity ID
			std::string posLabel = "Position##" + std::to_string((uint32_t)entity);
			std::string scaleLabel = "Scale##" + std::to_string((uint32_t)entity);
			std::string rotLabel = "Rotation##" + std::to_string((uint32_t)entity);

			// Position
			ImGui::SliderFloat3(posLabel.c_str(), &transform.position.x, -10.0f, 10.0f);

			// Scale
			ImGui::SliderFloat3(scaleLabel.c_str(), &transform.scale.x, -10.0f, 10.0f);

			// Rotation
			ImGui::SliderFloat3(rotLabel.c_str(), &transform.rotation.x, -360.0f, 360.0f);

		}

		if (registry.any_of<ModelComponent>(entity))
		{
			ImGui::Text("ModelComponent");
		}
		});

	ImGui::End();


}
void AppUI::DrawTextureInspector()
{

	ImGui::Begin("Texture Inspector");

	auto& m_Textures = m_ActiveScene->GetTextures();
	static std::string currentTextureKey;

	for (auto& [name, tex] : m_Textures)
	{
		ImGui::Text("Current Texture: %s", name.c_str());
		ImGui::Text("Current Texture Name: %s", std::filesystem::path(name).stem().string().c_str());

		;
		// Display thumbnail if you have OpenGL ID
		if (tex.id != 0) // assuming Texture struct has 'id' as GPU handle
		{
			ImVec2 size = ImVec2(128, 128);
			ImGui::Image((void*)(intptr_t)tex.id, size, ImVec2(0, 1), ImVec2(1, 0));
			ImGui::SameLine();
		}
	}



	// Button to open file path popup
	if (ImGui::Button("Load Texture"))
	{
		std::filesystem::path filepath = FileDialog::OpenFile();
		//	ImGui::OpenPopup("Load Texture");
		if (filepath.empty())
		{
			spdlog::error("Failed to load texture or canceled by usser");
		}
		else
		{

			Texture texture(filepath.string(), "texture_diffuse");

			m_Textures.insert({ filepath.string(), texture });


			currentTextureKey = filepath.string();

		}

	}

	ImGui::End();
}

void AppUI::DrawModelPanel()
{
	ImGui::Begin("Registry Debug");
	auto& registry = m_ActiveScene->GetRegistry();

	registry.view<Transform, ModelComponent>().each([&](auto entity, Transform& t, ModelComponent& mc) {
		// Use a unique ID for each entity header
		std::string label = "Entity: " + std::to_string(static_cast<uint32_t>(entity));

		if (ImGui::CollapsingHeader(label.c_str())) {
			ImGui::Indent();

			// --- SECTION: THUMBNAIL & BASIC INFO ---
			// Assuming your model has a GetThumbnail() returning an ImTextureID
			// If you don't have one yet, this is where the placeholder goes
			ImVec2 thumbSize(64, 64);
			if (mc.model->GetThumbnail()) {
				// Standard: ImVec2(0,0), ImVec2(1,1)
// Flipped:  ImVec2(0,1), ImVec2(1,0)
				ImGui::Image(
					(ImTextureID)(intptr_t)mc.model->GetThumbnail(),
					ImVec2(64, 64),
					ImVec2(0, 1), // Top-left UV
					ImVec2(1, 0)  // Bottom-right UV
				);
				ImGui::SameLine();
			}

			ImGui::BeginGroup();
			ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Model Data");
			ImGui::Text("File Size: %d Mb", mc.model->m_FileSize);

			// Status with color
			bool loaded = mc.model->IsLoaded();
			ImGui::Text("Status: "); ImGui::SameLine();
			ImGui::TextColored(loaded ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1),
				loaded ? "LOADED" : "PENDING");
			ImGui::EndGroup();

			ImGui::Separator();

			// --- SECTION: TRANSFORM (Using a Table for alignment) ---
			if (ImGui::BeginTable("TransformTable", 2)) {
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0); ImGui::Text("Position");
				ImGui::TableSetColumnIndex(1); ImGui::Text("%.2f, %.2f, %.2f", t.position.x, t.position.y, t.position.z);

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0); ImGui::Text("Scale");
				ImGui::TableSetColumnIndex(1); ImGui::Text("%.2f, %.2f, %.2f", t.scale.x, t.scale.y, t.scale.z);

				ImGui::EndTable();
			}

			// --- SECTION: PERFORMANCE/DEBUG ---
			ImGui::SeparatorText("Performance");
			ImGui::Text("Load Time: %.3f s", mc.model->m_LoadTime.count());
			ImGui::TextDisabled("Ptr: %p", mc.model.get());

			ImGui::Unindent();
			ImGui::Spacing();
		}
		});

	ImGui::End();
}
void AppUI::DrawMenuBar()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
				m_ActiveScene->GetRegistry().clear();
				// Reset scene defaults here
			}

			if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) {
				std::filesystem::path path = FileDialog::OpenFile(); // Assuming this opens a .sce
				if (!path.empty()) m_ActiveScene->Load(path);
			}

			if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
				// You previously used "Default.sce", better to use a path variable
				std::filesystem::path path = std::filesystem::current_path() / "Default.sce";
				m_ActiveScene->Save(path);
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Import Model...")) {
				std::filesystem::path filepath = FileDialog::OpenFile();
				if (!filepath.empty()) {
					m_ActiveScene->CreateModel(filepath.string(), glm::vec4(1.0f));
				}
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Exit", "Alt+F4")) {
				// Trigger application close (e.g., glfwSetWindowShouldClose)
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("Undo", "Ctrl+Z", false, false)) {} // Placeholder
			if (ImGui::MenuItem("Redo", "Ctrl+Y", false, false)) {} // Placeholder

			ImGui::Separator();

			if (ImGui::MenuItem("Clear Registry")) {
				m_ActiveScene->GetRegistry().clear();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			// Use these to toggle your panel visibility if you add boolean flags
			static bool showECS = true;
			ImGui::MenuItem("ECS Panel", nullptr, &showECS);

			static bool showPerf = true;
			ImGui::MenuItem("Performance Overlay", nullptr, &showPerf);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help"))
		{
			if (ImGui::MenuItem("About")) { /* Show a modal or version info */ }
			ImGui::EndMenu();
		}

		// Quick Stats on the far right
		float width = ImGui::GetWindowWidth();
		ImGui::SetCursorPosX(width - 150);
		ImGui::TextDisabled("%.1f FPS", ImGui::GetIO().Framerate);

		ImGui::EndMainMenuBar();
	}
}
void AppUI::DrawPropertiesPanel()
{
	ImGui::Begin("Properties Inspector", nullptr, ImGuiWindowFlags_NoCollapse);

	if (m_SelectedEntity == entt::null || !m_ActiveScene->GetRegistry().valid(m_SelectedEntity))
	{
		// Empty state with icon and message
		ImVec2 windowSize = ImGui::GetContentRegionAvail();
		ImGui::SetCursorPosY(windowSize.y * 0.4f);

		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
		ImGui::SetCursorPosX((windowSize.x - ImGui::CalcTextSize("󰉍 No Entity Selected").x) * 0.5f);
		ImGui::Text("󰉍 No Entity Selected");
		ImGui::SetCursorPosX((windowSize.x - ImGui::CalcTextSize("Select an entity in the Scene Hierarchy").x) * 0.5f);
		ImGui::TextDisabled("Select an entity in the Scene Hierarchy");
		ImGui::PopStyleColor();

		ImGui::End();
		return;
	}

	auto& registry = m_ActiveScene->GetRegistry();

	// --- ENTITY HEADER WITH ICON AND STATUS ---
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
	ImGui::BeginChild("EntityHeader", ImVec2(0, 48), true, ImGuiWindowFlags_NoScrollbar);

	ImGui::SetCursorPos(ImVec2(8, 8));
	ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Assuming you have a larger font loaded
	ImGui::Text("󰈸"); // Entity icon
	ImGui::PopFont();

	ImGui::SameLine(40);
	ImGui::BeginGroup();
	ImGui::Text("Entity %d", (uint32_t)m_SelectedEntity);

	// Tag/Badge for entity type
	if (registry.any_of<Camera>(m_SelectedEntity)) {
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.8f, 0.6f));
		ImGui::SmallButton("Camera");
		ImGui::PopStyleColor();
	}
	if (registry.any_of<RigidBody>(m_SelectedEntity)) {
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.2f, 0.6f));
		ImGui::SmallButton("Physics");
		ImGui::PopStyleColor();
	}

	ImGui::TextDisabled("Entity ID: %d | Active", (uint32_t)m_SelectedEntity);
	ImGui::EndGroup();

	ImGui::EndChild();
	ImGui::PopStyleColor();

	ImGui::Separator();
	ImGui::Spacing();

	// --- SCROLLABLE COMPONENTS AREA ---
	ImGui::BeginChild("ComponentsArea", ImVec2(0, -(ImGui::GetFrameHeightWithSpacing() + 8)), true);

	// --- 1. TRANSFORM COMPONENT (ALWAYS FIRST, ALWAYS VISIBLE) ---
	if (registry.any_of<Transform>(m_SelectedEntity))
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));

		// Collapsible header with component icon and context menu
		bool transformOpen = ImGui::CollapsingHeader("󰄤 Transform", ImGuiTreeNodeFlags_DefaultOpen);

		// Component toolbar (hamburger menu for remove/reset)
		ImGui::SameLine(ImGui::GetWindowWidth() - 40);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		if (ImGui::SmallButton("󰍎###TransformMenu")) {
			ImGui::OpenPopup("TransformContextMenu");
		}
		ImGui::PopStyleColor();

		if (ImGui::BeginPopup("TransformContextMenu")) {
			if (ImGui::MenuItem("Reset Position")) {
				auto& t = registry.get<Transform>(m_SelectedEntity);
				t.position = glm::vec3(0.0f);
			}
			if (ImGui::MenuItem("Reset Rotation")) {
				auto& t = registry.get<Transform>(m_SelectedEntity);
				t.rotation = glm::vec3(0.0f);
			}
			if (ImGui::MenuItem("Reset Scale")) {
				auto& t = registry.get<Transform>(m_SelectedEntity);
				t.scale = glm::vec3(1.0f);
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Remove Component", nullptr, false, false)) {
				// Can't remove transform - disabled
			}
			ImGui::EndPopup();
		}

		if (transformOpen)
		{
			ImGui::Indent(8);

			auto& t = registry.get<Transform>(m_SelectedEntity);

			// Position with better layout and reset buttons
			ImGui::PushID("TransformPos");
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Position");
			ImGui::SameLine(80);

			ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 60);
			float pos[3] = { t.position.x, t.position.y, t.position.z };
			if (ImGui::DragFloat3("##pos", pos, 0.1f, 0.0f, 0.0f, "%.2f")) {
				t.position = glm::vec3(pos[0], pos[1], pos[2]);
			}
			ImGui::PopItemWidth();

			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
			if (ImGui::SmallButton("R##resetPos")) {
				t.position = glm::vec3(0.0f);
			}
			ImGui::PopStyleColor();
			ImGui::PopID();

			// Rotation
			ImGui::PushID("TransformRot");
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Rotation");
			ImGui::SameLine(80);

			ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 60);
			float rot[3] = { t.rotation.x, t.rotation.y, t.rotation.z };
			if (ImGui::DragFloat3("##rot", rot, 1.0f, -360.0f, 360.0f, "%.1f°")) {
				t.rotation = glm::vec3(rot[0], rot[1], rot[2]);
			}
			ImGui::PopItemWidth();

			ImGui::SameLine();
			if (ImGui::SmallButton("R##resetRot")) {
				t.rotation = glm::vec3(0.0f);
			}
			ImGui::PopID();

			// Scale
			ImGui::PushID("TransformScale");
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Scale   ");
			ImGui::SameLine(80);

			ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 60);
			float scale[3] = { t.scale.x, t.scale.y, t.scale.z };
			if (ImGui::DragFloat3("##scale", scale, 0.1f, 0.01f, 100.0f, "%.2f")) {
				t.scale = glm::vec3(scale[0], scale[1], scale[2]);
			}
			ImGui::PopItemWidth();

			ImGui::SameLine();
			if (ImGui::SmallButton("R##resetScale")) {
				t.scale = glm::vec3(1.0f);
			}
			ImGui::PopID();

			ImGui::Unindent(8);
			ImGui::Spacing();
		}

		ImGui::PopStyleVar();
	}

	// --- 2. MATERIAL COMPONENT (MODERN CARD STYLE) ---
	if (registry.any_of<MeshComponent, ModelComponent>(m_SelectedEntity))
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));

		bool materialOpen = ImGui::CollapsingHeader("󰜫 Material", ImGuiTreeNodeFlags_DefaultOpen);

		// Component toolbar
		ImGui::SameLine(ImGui::GetWindowWidth() - 40);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		if (ImGui::SmallButton("󰍎###MaterialMenu")) {
			ImGui::OpenPopup("MaterialContextMenu");
		}
		ImGui::PopStyleColor();

		if (ImGui::BeginPopup("MaterialContextMenu")) {
			if (ImGui::MenuItem("Clear Textures")) {
				if (registry.any_of<MeshComponent>(m_SelectedEntity)) {
					auto& meshComp = registry.get<MeshComponent>(m_SelectedEntity);
					meshComp.mesh->mesh.textures.clear();
				}
			}
			if (ImGui::MenuItem("Remove Component", nullptr, false, false)) {
				registry.remove<MeshComponent>(m_SelectedEntity);
			}
			ImGui::EndPopup();
		}

		if (materialOpen)
		{
			ImGui::Indent(8);

			auto& textures = m_ActiveScene->GetTextures();

			// Material preview card
			ImGui::BeginChild("MaterialPreview", ImVec2(0, 120), true, ImGuiWindowFlags_NoScrollbar);

			// Left side - texture preview
			ImGui::BeginGroup();
			ImVec2 previewSize = ImVec2(100, 100);
			ImGui::InvisibleButton("##PreviewDrop", previewSize);

			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TEXTURE_PATH")) {
					const char* path = (const char*)payload->Data;
					// Handle texture drop
				}
				ImGui::EndDragDropTarget();
			}

			// Draw preview box with texture if available
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImVec2 previewPos = ImGui::GetItemRectMin();

			// Checkerboard background
			drawList->AddRectFilled(previewPos, ImVec2(previewPos.x + previewSize.x, previewPos.y + previewSize.y), IM_COL32(60, 60, 60, 255));
			for (int i = 0; i < 4; i++) {
				for (int j = 0; j < 4; j++) {
					if ((i + j) % 2 == 0) {
						ImVec2 p1(previewPos.x + i * 25, previewPos.y + j * 25);
						ImVec2 p2(p1.x + 25, p1.y + 25);
						drawList->AddRectFilled(p1, p2, IM_COL32(80, 80, 80, 255));
					}
				}
			}

			// Show texture if available
			if (registry.any_of<MeshComponent>(m_SelectedEntity)) {
				auto& meshComp = registry.get<MeshComponent>(m_SelectedEntity);
				if (!meshComp.mesh->mesh.textures.empty()) {
					auto& tex = meshComp.mesh->mesh.textures[0];
					// Draw texture here if you have the ID
					// drawList->AddImage((void*)(intptr_t)tex.id, previewPos, ImVec2(previewPos.x + previewSize.x, previewPos.y + previewSize.y));
				}
			}

			ImGui::SameLine(120);

			// Right side - material properties
			ImGui::BeginGroup();
			ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Diffuse Map");
			ImGui::Spacing();

			// Texture dropdown with preview
			static std::string currentTexture = "None";
			if (ImGui::BeginCombo("##TextureSelect", currentTexture.c_str(), ImGuiComboFlags_HeightLarge))
			{
				if (ImGui::Selectable("None", currentTexture == "None")) {
					currentTexture = "None";
					// Clear textures
				}

				ImGui::Separator();

				for (auto& [path, tex] : textures)
				{
					std::string filename = std::filesystem::path(path).filename().string();
					bool isSelected = (currentTexture == filename);

					if (ImGui::Selectable(filename.c_str(), isSelected)) {
						currentTexture = filename;
						// Apply texture to entity
						if (registry.any_of<MeshComponent>(m_SelectedEntity)) {
							auto& meshComp = registry.get<MeshComponent>(m_SelectedEntity);
							meshComp.mesh->mesh.textures.push_back(tex);
						}
					}
					if (isSelected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			ImGui::Spacing();

			// Material properties
			static float metallic = 0.5f;
			static float roughness = 0.5f;
			static float ao = 1.0f;

			ImGui::SetNextItemWidth(150);
			ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f, "%.2f");
			ImGui::SetNextItemWidth(150);
			ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f, "%.2f");
			ImGui::SetNextItemWidth(150);
			ImGui::SliderFloat("AO", &ao, 0.0f, 1.0f, "%.2f");

			ImGui::EndGroup();
			ImGui::EndGroup();

			ImGui::EndChild();

			ImGui::Unindent(8);
			ImGui::Spacing();
		}

		ImGui::PopStyleVar();
	}

	// --- 3. PHYSICS SECTION WITH MODERN TOGGLES ---
	if (registry.any_of<RigidBody>(m_SelectedEntity))
	{
		bool physicsOpen = ImGui::CollapsingHeader("󰊤 Physics", ImGuiTreeNodeFlags_DefaultOpen);

		// Component toolbar
		ImGui::SameLine(ImGui::GetWindowWidth() - 40);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		if (ImGui::SmallButton("󰍎###PhysicsMenu")) {
			ImGui::OpenPopup("PhysicsContextMenu");
		}
		ImGui::PopStyleColor();

		if (ImGui::BeginPopup("PhysicsContextMenu")) {
			if (ImGui::MenuItem("Remove Component")) {
				registry.remove<RigidBody>(m_SelectedEntity);
			}
			ImGui::EndPopup();
		}

		if (physicsOpen)
		{
			ImGui::Indent(8);

			auto& rb = registry.get<RigidBody>(m_SelectedEntity);

			// Modern toggle switches
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Gravity");
			ImGui::SameLine(120);

			// Custom toggle style
			ImGui::PushStyleColor(ImGuiCol_Button, rb.useGravity ? ImVec4(0.2f, 0.6f, 0.2f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
			if (ImGui::Button(rb.useGravity ? "ON" : "OFF", ImVec2(50, 0))) {
				rb.useGravity = !rb.useGravity;
			}
			ImGui::PopStyleColor();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Kinematic");
			ImGui::SameLine(120);

			ImGui::PushStyleColor(ImGuiCol_Button, rb.isKinematic ? ImVec4(0.2f, 0.6f, 0.2f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
			if (ImGui::Button(rb.isKinematic ? "ON" : "OFF", ImVec2(50, 0))) {
				rb.isKinematic = !rb.isKinematic;
			}
			ImGui::PopStyleColor();

			// Mass property
			static float mass = 1.0f;
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Mass");
			ImGui::SameLine(120);
			ImGui::SetNextItemWidth(150);
			ImGui::DragFloat("##mass", &mass, 0.1f, 0.01f, 1000.0f, "%.2f kg");

			ImGui::Unindent(8);
			ImGui::Spacing();
		}
	}

	// --- 4. CAMERA COMPONENT ---
	if (registry.any_of<Camera>(m_SelectedEntity))
	{
		bool cameraOpen = ImGui::CollapsingHeader("󰀙 Camera", ImGuiTreeNodeFlags_DefaultOpen);

		ImGui::SameLine(ImGui::GetWindowWidth() - 40);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		if (ImGui::SmallButton("󰍎###CameraMenu")) {
			ImGui::OpenPopup("CameraContextMenu");
		}
		ImGui::PopStyleColor();

		if (ImGui::BeginPopup("CameraContextMenu")) {
			if (ImGui::MenuItem("Set as Main Camera")) {
				m_ActiveScene->setCameraType(CameraType::Player);
			}
			if (ImGui::MenuItem("Remove Component")) {
				registry.remove<Camera>(m_SelectedEntity);
			}
			ImGui::EndPopup();
		}

		if (cameraOpen)
		{
			ImGui::Indent(8);

			auto& cam = registry.get<Camera>(m_SelectedEntity);

			// Camera presets
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Preset");
			ImGui::SameLine(80);

			if (ImGui::Button("Perspective", ImVec2(90, 0))) {
				// Set perspective
			}
			ImGui::SameLine();
			if (ImGui::Button("Orthographic", ImVec2(90, 0))) {
				// Set orthographic
			}

			// FOV slider
			static float fov = 60.0f;
			ImGui::AlignTextToFramePadding();
			ImGui::Text("FOV");
			ImGui::SameLine(80);
			ImGui::SetNextItemWidth(150);
			ImGui::SliderFloat("##fov", &fov, 10.0f, 120.0f, "%.1f°");

			// Clipping planes
			static float nearPlane = 0.1f;
			static float farPlane = 1000.0f;
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Clipping");
			ImGui::SameLine(80);
			ImGui::SetNextItemWidth(70);
			ImGui::DragFloat("Near", &nearPlane, 0.01f, 0.01f, 10.0f, "%.2f");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(70);
			ImGui::DragFloat("Far", &farPlane, 1.0f, 10.0f, 10000.0f, "%.1f");

			ImGui::Unindent(8);
			ImGui::Spacing();
		}
	}

	ImGui::EndChild();

	// --- 5. ADD COMPONENT BUTTON (MODERN FLOATING STYLE) ---
	ImGui::Separator();
	ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 150) * 0.5f);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.4f, 0.7f, 1.0f));

	if (ImGui::Button("+ Add Component", ImVec2(150, 28))) {
		ImGui::OpenPopup("AddComponentPopup");
	}

	ImGui::PopStyleColor(3);

	// Modern component search popup
	if (ImGui::BeginPopup("AddComponentPopup"))
	{
		ImGui::Text("Search Components");
		ImGui::Separator();

		static char searchBuffer[128] = "";
		ImGui::InputText("##search", searchBuffer, IM_ARRAYSIZE(searchBuffer));

		ImGui::Separator();

		// Component categories
		if (ImGui::BeginMenu("Rendering")) {
			if (ImGui::MenuItem("Mesh Renderer")) {}
			if (ImGui::MenuItem("Model")) {}
			if (ImGui::MenuItem("Light")) {}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Physics")) {
			if (ImGui::MenuItem("RigidBody")) {
				if (!registry.any_of<RigidBody>(m_SelectedEntity)) {
					auto trans = registry.get<Transform>(m_SelectedEntity).position;
					registry.emplace<RigidBody>(m_SelectedEntity, RigidBodyDesc(trans));
				}
			}
			if (ImGui::MenuItem("Collider")) {
				if (!registry.any_of<Collider>(m_SelectedEntity)) {
					registry.emplace<Collider>(m_SelectedEntity, ColliderDesc());
				}
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Camera")) {
			if (ImGui::MenuItem("Camera")) {
				if (!registry.any_of<Camera>(m_SelectedEntity)) {
					registry.emplace<Camera>(m_SelectedEntity, glm::vec4(0.f));
				}
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Effects")) {
			if (ImGui::MenuItem("Color")) {
				if (!registry.any_of<Color>(m_SelectedEntity)) {
					registry.emplace<Color>(m_SelectedEntity, glm::vec4(0.f));
				}
			}
			ImGui::EndMenu();
		}

		ImGui::EndPopup();
	}

	ImGui::End();
}
