#include "pch.hpp"      // precompiled header
#include "Application.hpp"
#include <PrimitiveShape.hpp>
#include <filesystem>
#include <Components.hpp>
#include <Scene.hpp>


Application::Application()
{

}

Application::~Application()
{
	Shutdown();
}
bool Application::isFocused = false;
void WindowSizeCallback(GLFWwindow* window, int width, int height)
{
	Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
	if (renderer)
		renderer->setSize(width, height);
}


bool Application::Init()
{
	m_WindowManager.Init();

	InitImGui();
	InitEngineSystems();
	m_Renderer.Init();
	// Setup
	glfwSetWindowUserPointer(m_WindowManager.GetWindow(), &m_Renderer);
	glfwSetWindowSizeCallback(m_WindowManager.GetWindow(), WindowSizeCallback);

	m_Input.setWindow(m_WindowManager.GetWindow());
	if (m_WindowManager.GetWindow() == nullptr) __debugbreak();
	return true;
}


bool CheckSharedContextThreads(GLFWwindow* contextA, GLFWwindow* contextB, std::thread& threadB)
{
	if (!contextA || !contextB)
	{
		spdlog::error("Invalid GLFW window pointer(s) provided.");
		return false;
	}

	std::atomic<bool> result{ false };
	std::atomic<bool> done{ false };

	// Launch a thread to test contextA against contextB
	std::thread testThread([&]() {
		// Make contextA current on this thread
		glfwMakeContextCurrent(contextA);

		// Try to query some OpenGL state that should exist if contextB is shared
		GLint maxTexUnits = 0;
		glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTexUnits);

		// If we got a valid value (>0), assume contexts are shared
		result = (maxTexUnits > 0);

		done = true;
		});

	// Wait for the test to finish (timeout optional)
	while (!done)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

	testThread.join();

	return result;
}
std::string load_file(const std::string& path) {
	std::ifstream f(path);
	std::stringstream ss;
	ss << f.rdbuf();
	return ss.str();
}

#include <imgui.h>
#include <string>
#include <functional>

int nextGroupId = 0; // keep track somewhere in your editor state
void ShowTwoNumberModal(const char* title, const char* label1, int& a, const char* label2, int& b, std::function<void()> onOk = nullptr)
{



}

void DrawEntity(entt::registry& registry, entt::entity e, Scene& scene)
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
				std::string pathStr = filepath;

				// Check if texture exists in unordered_map
				auto it = all_Textures.find(pathStr);
				if (it != all_Textures.end()) // exists
				{
					meshComp.mesh->mesh.textures.push_back(it->second); // push the Texture object
					registry.emplace<Texture>(e);
					ImGui::CloseCurrentPopup();
				}
				else
				{
					// Optionally handle missing texture, e.g., load it or show error
					std::cout << "Texture not found in all_Textures: " << pathStr << std::endl;
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





static auto lastFrame = std::chrono::high_resolution_clock::now();
void Application::Run()
{
	if (!m_WindowManager.isOpen())
		return;

	spdlog::info("Running WTHR Application...");

	glm::vec3 triangleColor(1, 0, 0);
	ImGuiIO& io = ImGui::GetIO();


	Scene scene(m_WindowManager.GetWindow());

	m_World.SetScene(&scene);

	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	GLFWwindow* workerWindow = glfwCreateWindow(1, 1, "GPU Worker", nullptr, m_WindowManager.GetWindow());


	GLFWwindow* contextA = m_WindowManager.GetWindow();
	//if (CheckSharedContextThreads(contextA, workerWindow,scene.gpuWorker.GetThread()))
	//{
	//	spdlog::info("Contextes are shared");
	//}
	//else
	//{
	//	spdlog::error("Contextes are NOT shared");

	//}
	//std::shared_ptr<Shapes::Cube> ptrCube = std::make_shared<Shapes::Cube>();
	//std::shared_ptr<Shapes::Pyramid>  ptrPyramid = std::make_shared<Shapes::Pyramid>();

	//Texture texture("texture.png","texture_diffuse");
	//ptrCube.get()->mesh.textures.push_back(texture);
	//ptrPyramid.get()->mesh.textures.push_back(texture);

	auto ptrShdr = std::make_shared<Shader>("shaders/default.vert", "shaders/default.frag");



	//scene.AddMesh(ptrCube);
	//scene.AddMesh(ptrPyramid);
	//scene.AddShader(ptrShdr);

	////TODO MODEL LOADING

	m_Renderer.m_Editor.set(&scene.script);
	auto& registry = scene.GetRegistry();
	entt::entity ent;



	while (m_WindowManager.isOpen())
	{
		auto currentFrame = std::chrono::high_resolution_clock::now();
		float deltaTime = std::chrono::duration<float>(currentFrame - lastFrame).count();
		lastFrame = currentFrame;

		io.DeltaTime = deltaTime;
		scene.script.update(1);

		m_World.stepSimulation(.016f);



		m_WindowManager.PollEvents();
		m_Input.Update();
		m_WindowManager.BeginFrame();




		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;

		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);

		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0)); // transparent background



		// Start full-screen dockspace host window
		ImGui::Begin("DockSpace Window", nullptr, window_flags);
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);

		// Create the actual DockSpace
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);





		if (scene.GetCameraType() == CameraType::Editor)
		{

			if (glfwGetKey(m_WindowManager.GetWindow(), GLFW_KEY_W) == GLFW_PRESS)
				scene.GetCamera().ProcessKeyboard(FORWARD, deltaTime);
			if (glfwGetKey(m_WindowManager.GetWindow(), GLFW_KEY_S) == GLFW_PRESS)
				scene.GetCamera().ProcessKeyboard(BACKWARD, deltaTime);
			if (glfwGetKey(m_WindowManager.GetWindow(), GLFW_KEY_A) == GLFW_PRESS)
				scene.GetCamera().ProcessKeyboard(LEFT, deltaTime);
			if (glfwGetKey(m_WindowManager.GetWindow(), GLFW_KEY_D) == GLFW_PRESS)
				scene.GetCamera().ProcessKeyboard(RIGHT, deltaTime);
		}
		else
		{

			if (glfwGetKey(m_WindowManager.GetWindow(), GLFW_KEY_W) == GLFW_PRESS)
				scene.GetCamera().ProcessKeyboard_Player(FORWARD, deltaTime);
			if (glfwGetKey(m_WindowManager.GetWindow(), GLFW_KEY_S) == GLFW_PRESS)
				scene.GetCamera().ProcessKeyboard_Player(BACKWARD, deltaTime);
			if (glfwGetKey(m_WindowManager.GetWindow(), GLFW_KEY_A) == GLFW_PRESS)
				scene.GetCamera().ProcessKeyboard_Player(LEFT, deltaTime);
			if (glfwGetKey(m_WindowManager.GetWindow(), GLFW_KEY_D) == GLFW_PRESS)
				scene.GetCamera().ProcessKeyboard_Player(RIGHT, deltaTime);

			registry.view<Camera, Transform>().each([&](entt::entity, Camera& cam, Transform& trans)
				{

					cam.Position.y -= 1.5f;
					trans.position = cam.Position;
					cam.Position.y += 1.5f;

				});





		}


		std::filesystem::path path = std::filesystem::current_path().concat("\\Default.sce");

		ImGui::Begin("ECS");
		if (ImGui::Button("Save"))
		{
			scene.Save(path);
		}

		ImGui::SameLine(); // Put next button on the same row

		if (ImGui::Button("Load"))
		{
			scene.Load(path);
		}
		ImGui::SameLine(); // Put next button on the same row

		if (ImGui::Button("Clear"))
		{
			registry.clear();
		}


		ImGui::Separator();


		if (ImGui::Button("+ Entity"))
		{
			scene.CreateCube();
		}

		ImGui::SameLine(); // Put next button on the same row

		if (ImGui::Button("- Entity"))
		{
			if (registry.valid(ent)) // optional: check if entity is valid
			{
				registry.destroy(ent); // deletes the entity and all its components
				ent = entt::null;     // reset your selected entity if needed
			}
		}
		// Persistent state
		static int a = 0, b = 0;
		static int nextGroupId = 0;

		if (ImGui::Button("Group"))
		{
			ImGui::OpenPopup("Group what"); // <-- open once when button is clicked
		}

		// Now draw the modal outside the button
		if (ImGui::BeginPopupModal("Group what", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			// Input numbers
			ImGui::InputInt("From", &a);
			ImGui::InputInt("To", &b);

			// Buttons
			if (ImGui::Button("OK", ImVec2(120, 0)))
			{
				if (a > b) std::swap(a, b);
				int groupId = nextGroupId++;

				// Assign entities in range [a,b] to the group
				registry.view<MeshComponent>().each([&](auto e, MeshComponent& mesh) {
					uint32_t id = (uint32_t)e;
					if (id >= a && id <= b)
					{
						if (registry.any_of<GroupComponent>(e))
							registry.get<GroupComponent>(e).id = groupId;
						else
							registry.emplace<GroupComponent>(e, groupId);
					}
					});

				ImGui::CloseCurrentPopup(); // close modal
			}

			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0)))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}


		ImGui::Separator();


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
						DrawEntity(registry, e, scene);

					ImGui::PopStyleColor(3);

					ImGui::TreePop();
				}
			}

			// Draw entities without a group (no indentation)
			registry.view<MeshComponent>(entt::exclude<GroupComponent>).each([&](auto e, MeshComponent& mesh) {
				DrawEntity(registry, e, scene);
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



		ImGui::Begin("Texture Inspector");

		auto& m_Textures = scene.GetTextures();
		static std::string currentTextureKey;

		for (auto& [name, tex] : m_Textures)
		{
			ImGui::Text("Current Texture: %s", currentTextureKey.c_str());

			// Display thumbnail if you have OpenGL ID
			if (tex.id != 0) // assuming Texture struct has 'id' as GPU handle
			{
				ImVec2 size = ImVec2(128, 128);
				ImGui::Image((void*)(intptr_t)tex.id, size, ImVec2(0, 1), ImVec2(1, 0));
				ImGui::SameLine();
			}
		}



		// Button to open file path popup
		if (ImGui::Button("Add/Load Texture"))
		{
			ImGui::OpenPopup("Load Texture");
		}

		if (ImGui::BeginPopupModal("Load Texture", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			static char filepath[256] = "";
			ImGui::InputText("File Path", filepath, IM_ARRAYSIZE(filepath));

			if (ImGui::Button("Load"))
			{
				std::string pathStr = filepath;
				if (!pathStr.empty())
				{
					// Call your background loader here:
					// Texture tex = LoadTexture(filepath);
					// m_Textures[pathStr] = tex;
					Texture texture(filepath, "texture_diffuse");

					m_Textures.insert({ filepath, texture });

					// Optionally set it as current
					currentTextureKey = pathStr;

					filepath[0] = '\0';
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				filepath[0] = '\0';
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::End();

		ImGui::Begin("Modal inspector");


		if (ImGui::Button(("Load Model")))
		{
			ImGui::OpenPopup(("Load Model"));
		}
		if (ImGui::BeginPopupModal(("Load Model"), NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			static char filepath[256] = "";
			ImGui::InputText("File Path", filepath, IM_ARRAYSIZE(filepath));

			if (ImGui::Button("Load"))
			{
				std::string pathStr = filepath;

				scene.CreateModel(filepath, glm::vec4(1.0f));
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				filepath[0] = '\0';
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::End();

		ImGui::Begin("Registry Debug");


		registry.view<Transform, ModelComponent>().each([&](auto entity, Transform& t, ModelComponent& mc) {
			ImGui::Separator();
			ImGui::Text("Entity: %u", static_cast<uint32_t>(entity));
			ImGui::Text("Transform ptr: %p", &t);
			ImGui::Text("Position: %.2f %.2f %.2f", t.position.x, t.position.y, t.position.z);
			ImGui::Text("Scale: %.2f %.2f %.2f", t.scale.x, t.scale.y, t.scale.z);
			ImGui::Text("ModelComponent shared_ptr: %p", mc.model.get());
			ImGui::Text("ModelComponent loaded: %s", mc.model->IsLoaded() ? "true" : "false");
			});

		ImGui::End();

		ImGui::End(); // end dockspace



		m_Renderer.Clear();


		if (scene.GetCameraType() == CameraType::Editor)
		{

			if (m_Input.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_1))
			{
				double x, y;
				glfwGetCursorPos(m_WindowManager.GetWindow(), &x, &y);
				m_Renderer.RenderPicking(scene, x, y);
			}

			if (m_Input.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_1))
			{
				double x, y;
				glfwGetCursorPos(m_WindowManager.GetWindow(), &x, &y);
				m_Renderer.HandlePickingClick(scene, x, y, ent);
			}
		}
		else if (scene.GetCameraType() == CameraType::Player)
		{
			if (m_Input.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_1))
			{
				//shoot bullet
				scene.CreateBullet();
			}
		}

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoBackground;

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
						if (ImGui::MenuItem("Single")) scene.CreateCube();
						if (ImGui::MenuItem("Grid 10x10x10")) scene.CreateCubeGrid(10, 10, 10);
						if (ImGui::MenuItem("Grid 10x10x1")) scene.CreateCubeGrid(10, 1, 10);

						ImGui::EndMenu();
					}

					if (ImGui::BeginMenu("Sphere"))
					{
						if (ImGui::MenuItem("Single")) scene.CreateSphere();
						ImGui::EndMenu();
					}

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Entities"))
				{
					if (ImGui::MenuItem("Player")) scene.CreatePlayer();

					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}

			ImGui::EndPopup();
		}


		const char* label = "Play";

		// Get the current window size
		ImVec2 windowSize = ImGui::GetContentRegionAvail(); // available size in the window

		// Calculate the size of the button
		ImVec2 buttonSize = ImVec2(100, 0); // width 100, height 0 (automatic)

		// Set cursor X to center the button
		ImGui::SetCursorPosX((windowSize.x - buttonSize.x) * 0.5f);

		// Optional: center vertically in remaining space
		// ImGui::SetCursorPosY((windowSize.y - buttonSize.y) * 0.5f);

		// Draw button
		if (ImGui::Button("Play", buttonSize)) {
			for (auto& instance : scene.script.scripts)
			{
				instance.second.isActive = true;
			}

		}
		ImGui::SameLine();
		if (ImGui::Button("Pause", buttonSize)) {
			for (auto& instance : scene.script.scripts)
			{
				instance.second.isActive = false;
			}

		}
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y,
			ImGui::GetWindowSize().x, ImGui::GetWindowSize().y);
		m_Renderer.RenderScene(scene, *ptrShdr);
		ImGui::End();


		m_Renderer.m_Editor.draw(registry);
		m_Renderer.m_Editor.drawDebugPanel(registry);

		m_WindowManager.EndFrame();

		m_WindowManager.SwapBuffers();
		static bool firstMouse = false;
		static double lastX;
		static double lastY;
		if (m_Input.IsKeyPressed(GLFW_KEY_ESCAPE))
		{
			isFocused = !isFocused;

			if (isFocused)
			{
				double xpos, ypos;
				glfwGetCursorPos(m_WindowManager.GetWindow(), &xpos, &ypos);
				lastX = static_cast<float>(xpos);
				lastY = static_cast<float>(ypos);
				firstMouse = true; // reset to avoid huge jump
			}
		}


		if (glfwGetKey(m_WindowManager.GetWindow(), GLFW_KEY_Q) == GLFW_PRESS)
		{
			m_Renderer.gizmoType = ImGuizmo::OPERATION::TRANSLATE;
		}
		else if (glfwGetKey(m_WindowManager.GetWindow(), GLFW_KEY_W) == GLFW_PRESS)
		{
			m_Renderer.gizmoType = ImGuizmo::OPERATION::ROTATE;
		}
		else if (glfwGetKey(m_WindowManager.GetWindow(), GLFW_KEY_E) == GLFW_PRESS)
		{
			m_Renderer.gizmoType = ImGuizmo::OPERATION::SCALE;
		}
		else if (glfwGetKey(m_WindowManager.GetWindow(), GLFW_KEY_R) == GLFW_PRESS)
		{
			m_Renderer.gizmoType = ImGuizmo::OPERATION::SCALEU; // uniform scale
		}


		if (glfwGetKey(m_WindowManager.GetWindow(), GLFW_KEY_1) == GLFW_PRESS)
		{
			scene.setCameraType(CameraType::Editor);
		}
		else if (glfwGetKey(m_WindowManager.GetWindow(), GLFW_KEY_2) == GLFW_PRESS)
		{
			scene.setCameraType(CameraType::Player);
		}








		if (isFocused)
		{
			double xposIn, yposIn;
			glfwGetCursorPos(m_WindowManager.GetWindow(), &xposIn, &yposIn);
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

			scene.GetCamera().ProcessMouseMovement(xoffset, yoffset);

			glfwSetInputMode(m_WindowManager.GetWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		else
		{
			glfwSetInputMode(m_WindowManager.GetWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}

	}
}


void Application::Update()
{
	// ECS, input, editor logic
}

void Application::Render()
{
	// Rendering is handled by Renderer library
}

void Application::InitEngineSystems()
{
	spdlog::info("Initializing ECS, asset loaders, physics, etc.");
	// Example: EnTT registry, JSON parser, Assimp importer setup
}

void Application::InitImGui()
{
	spdlog::info("Initializing ImGui...");
	// Setup ImGui context here
	// ImGui::CreateContext();
	// ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
	// ImGui_ImplOpenGL3_Init("#version 460");
}

void Application::Shutdown()
{
	spdlog::info("Shutting down WTHR Application...");

	// Cleanup ImGui
	// ImGui_ImplOpenGL3_Shutdown();
	// ImGui_ImplGlfw_Shutdown();
	// ImGui::DestroyContext();


}
