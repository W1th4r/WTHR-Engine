#include <pch.hpp>
#include <Framebuffer.hpp>
#include <Renderer.hpp>
#include <Scene.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ScriptEditor.hpp>

Renderer::Renderer()
{
	width = 1920;
	height = 1080;
}

static Framebuffer::PixelInfo pixel;
void Renderer::Init()
{
	// OpenGL state
	glEnable(GL_DEPTH_TEST);

	// Triangle vertices
	float vertices[] = {
		// positions        // colors
		 0.0f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
		-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
		 0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f
	};

	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);

	glBindVertexArray(m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// positions
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// colors
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);

	// Compile shaders
	spdlog::info("Renderer initialized");

	pickingShader = Shader("shaders/picking.vert", "shaders/picking.frag");

	// In your main application initialization

}

void Renderer::Clear(const glm::vec3& color)
{
	uint32_t clearValue = 0xFFFFFFFF; // uint -1
	glClearBufferuiv(GL_COLOR, 1, &clearValue);
	glClearColor(color.r, color.g, color.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
void Renderer::RenderScene(Scene& scene, Shader& shader)
{
	auto& registry = scene.GetRegistry();
	Camera& camera = scene.GetCamera();

	//TODO fails if window minimized
	glm::mat4 projection = glm::perspective(
		glm::radians(45.f),
		static_cast<float>(width) / static_cast<float>(height),
		0.1f, 100.f
	);

	// 1. Bind Shader and Upload Global Uniforms
	shader.use();
	shader.setMat4("view", camera.GetViewMatrix());
	shader.setMat4("projection", projection);
	shader.setVec3("viewPos", camera.Position); // Matches uniform vec3 viewPos

	// 2. Upload Directional Light Uniforms (Matches struct Light dirLight)
	shader.setVec3("dirLight.direction",this->m_SunDirection);
	shader.setVec3("dirLight.ambient", this->m_SunAmbient);
	shader.setVec3("dirLight.diffuse", this->m_SunDiffuse);
	shader.setVec3("dirLight.specular", this->m_SunSpecular);

	int pointLightCount = 0;
	const int MAX_POINT_LIGHTS = 16;

	auto lightView = registry.view<PointLight, Transform>();
	lightView.each([&](auto entity, auto& light, auto& transform) {
		if (pointLightCount >= MAX_POINT_LIGHTS) return;

		// Construct the uniform array string dynamically (e.g., "pointLights[0].color")
		std::string baseName = "pointLights[" + std::to_string(pointLightCount) + "].";

		// Grab position straight out of the light's Transform component
		shader.setVec3(baseName + "position", transform.position);
		shader.setVec3(baseName + "color", light.color);
		shader.setFloat(baseName + "intensity", light.intensity);
		shader.setFloat(baseName + "linear", light.linear);
		shader.setFloat(baseName + "quadratic", light.quadratic);

		pointLightCount++;
		});
	shader.setInt("activePointLightCount", pointLightCount);


	// ==========================================
	// PASS 1: Render MeshComponents
	// ==========================================
	shader.setBool("useModel", false);

	registry.view<MeshComponent, Transform>().each([&](auto entity, auto& meshComp, auto& transform) {
		// Skip player/camera entity from drawing inside its own head
		if (registry.all_of<Camera>(entity)) return;
		if (!meshComp.mesh) return;

		shader.setMat4("model", BuildModelMatrix(transform));

		// Single-lookup check for optional Color component
		Color* color = registry.try_get<Color>(entity);
		bool hasTextures = !meshComp.mesh->mesh.textures.empty();

		// Material state evaluation for your Uber Shader toggles
		if (hasTextures)
		{
			shader.setBool("useTexture", true);
			shader.setBool("useColor", color != nullptr);
			if (color)
			{
				shader.setVec4("uColor", color->value);
			}
		}
		else if (color)
		{
			shader.setBool("useTexture", false);
			shader.setBool("useColor", true);
			shader.setVec4("uColor", color->value);
		}
		else
		{
			// Fallback flat white if completely empty
			shader.setBool("useTexture", false);
			shader.setBool("useColor", true);
			shader.setVec4("uColor", glm::vec4(1.0f));
		}

		// Enable lighting calculations for standard meshes
		bool isLightSource = registry.any_of<PointLight>(entity);

		shader.setBool("useLighting", !isLightSource);

		shader.setFloat("materialShininess", 32.0f); // Default shininess factor (or pull from a component)

		// Assimp mesh draw routine binds texture_diffus	e1 (Slot 0) and texture_specular1 (Slot 1)
		meshComp.mesh->Draw(shader);
		});

	// ==========================================
	// PASS 2: Render ModelComponents
	// ==========================================
	shader.setBool("useModel", true);
	shader.setBool("useTexture", true);
	shader.setBool("useColor", false);
	shader.setBool("useLighting", true);
	shader.setFloat("materialShininess", 32.0f);

	registry.view<ModelComponent, Transform>().each([&](auto entity, auto& modelComp, auto& transform) {
		if (!modelComp.model || !modelComp.model->IsLoaded()) return;

		shader.setMat4("model", BuildModelMatrix(transform));
		modelComp.model->Draw(shader);
		});

	// ==========================================
	// PASS 3: Debug / Gizmos
	// ==========================================
	// Turn off lighting calculations so gizmos are completely unlit full-color
	shader.setBool("useLighting", false);
	RenderGizmo(scene, shader);
}
// Helper function for building model matrices
glm::mat4 Renderer::BuildModelMatrix(const Transform& transform)
{
	glm::mat4 model = glm::translate(glm::mat4(1.0f), transform.position);
	model = glm::rotate(model, glm::radians(transform.rotation.x), glm::vec3(1, 0, 0));
	model = glm::rotate(model, glm::radians(transform.rotation.y), glm::vec3(0, 1, 0));
	model = glm::rotate(model, glm::radians(transform.rotation.z), glm::vec3(0, 0, 1));
	model = glm::scale(model, transform.scale);
	return model;
}

// Separate function for gizmo rendering
void Renderer::RenderGizmo(Scene& scene, Shader& shader)
{
	if (!pixel.ObjectID == -1) return;

	entt::entity clickedEntity = static_cast<entt::entity>(pixel.ObjectID);

	if (scene.HasComponent<Transform>(clickedEntity)) {
		// Setup ImGuizmo
		glViewport(0, 0, width, height);
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist();

		float windowHeight = ImGui::GetWindowHeight();
		float windowWidth = ImGui::GetWindowWidth();
		ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y,
			windowWidth, windowHeight);

		auto& camera = scene.GetCamera();
		glm::mat4 cameraView = camera.GetViewMatrix();
		glm::mat4 projection = glm::perspective(
			glm::radians(45.0f),
			static_cast<float>(width) / static_cast<float>(height),
			0.1f, 100.0f
		);

		auto& transform = scene.GetComponent<Transform>(clickedEntity);
		glm::mat4 model = BuildModelMatrix(transform);

		ImGuizmo::Manipulate(
			glm::value_ptr(cameraView),
			glm::value_ptr(projection),
			gizmoType,
			ImGuizmo::MODE::LOCAL,
			glm::value_ptr(model)
		);

		if (ImGuizmo::IsUsing()) {
			transform.SetFromMatrix(model);
		}
	}
}

void Renderer::RenderPicking(Scene& scene, int x, int y)
{
	static bool framebufferInitialized = false;
	if (!framebufferInitialized)
	{
		if (!m_ObjectPicking.create(width, width))
		{
			spdlog::error("Failed to create picking framebuffer!");
			return;
		}
		framebufferInitialized = true;
	}

	// --- Bind and clear picking framebuffer ---
	m_ObjectPicking.Bind();
	glViewport(0, 0, width, height);

	// 1. MUST use glClearBufferuiv (with a 'u') for GL_RGB32UI / GL_R32UI textures!
	//    0xFFFFFFFF is equivalent to (uint32_t)-1
	uint32_t clearValue[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
	glClearBufferuiv(GL_COLOR, 0, clearValue);

	// 2. Clear ONLY depth
	glClear(GL_DEPTH_BUFFER_BIT);
	pickingShader.use();

	float nearPlane = 0.1f;
	float farPlane = 100.f;
	glm::mat4 view = scene.GetCamera().GetViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(45.f), width / height, nearPlane, farPlane);

	auto& reg = scene.GetRegistry();
	reg.view<Transform>().each([&](entt::entity ent, Transform& transform)
		{
			glm::mat4 World(1.0f);

			// Translation
			World = glm::translate(World, transform.position);
			// Rotation (XYZ order)
			World = glm::rotate(World, transform.rotation.x, glm::vec3(1, 0, 0));
			World = glm::rotate(World, transform.rotation.y, glm::vec3(0, 1, 0));
			World = glm::rotate(World, transform.rotation.z, glm::vec3(0, 0, 1));
			// Scale
			World = glm::scale(World, transform.scale);

			glm::mat4 MVP = projection * view * World;
			pickingShader.setMat4("MVP", MVP);
			pickingShader.setUInt("ObjectID", static_cast<uint32_t>(entt::to_integral(ent)));
			pickingShader.setUInt("DrawID", 0);
			pickingShader.setUInt("PrimID", 0);

			if (reg.any_of<MeshComponent>(ent))
			{
				auto& mesh = reg.get<MeshComponent>(ent);
				mesh.mesh->Draw(pickingShader);
			}
			else if (reg.any_of<ModelComponent>(ent))
			{
				auto& model = reg.get<ModelComponent>(ent);
				model.model->Draw(pickingShader);
			}
		});

	// --- Read pixel ---
	Framebuffer::PixelInfo pixel = m_ObjectPicking.ReadPixel(
		static_cast<GLuint>(x),
		static_cast<GLuint>(height - y - 1) // Flip Y
	);

	// Unbind framebuffer
	m_ObjectPicking.Unbind();

	// Convert to entt entity safely
	if (pixel.ObjectID != -1)
	{
		entt::entity clickedEntity = static_cast<entt::entity>(pixel.ObjectID);
		spdlog::debug("Clicked entity ID: {}", pixel.ObjectID);
		// You could highlight or select it here
	}
}


void Renderer::HandlePickingClick(Scene& scene, double mouseX, double mouseY, entt::entity& picked)
{
	// 1. Read pixel info from picking framebuffer
	pixel = m_ObjectPicking.ReadPixel(
		static_cast<int>(mouseX),
		static_cast<int>(height - mouseY - 1) // flip Y
	);

	pixel.Print(); // Optional debug output

	// 2. If no object was clicked, return early
	if (pixel.ObjectID == -1)
		return;

	// 3. Convert back to entt::entity ID
	entt::entity clickedEntity = static_cast<entt::entity>(pixel.ObjectID);
	picked = clickedEntity;

	if (!scene.GetRegistry().valid(clickedEntity))
	{
		spdlog::warn("Clicked entity {} is not valid!", pixel.ObjectID);
		return;
	}

	// 4. Optional — highlight or log selection
	spdlog::info("Entity {} clicked (DrawID: {}, PrimID: {})",
		pixel.ObjectID, pixel.DrawID, pixel.PrimID);

	// 5. Example: draw highlight around the clicked entity
	Shader highlightShader("shaders/simple_color.vert", "shaders/simple_color.frag");
	highlightShader.use();

	// build view/projection
	glm::mat4 view = scene.GetCamera().GetViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(45.f),
		width / height, 0.1f, 100.f);

	auto& reg = scene.GetRegistry();

	if (reg.any_of<Transform>(clickedEntity))
	{
		auto& transform = reg.get<Transform>(clickedEntity);

		glm::mat4 world(1.0f);
		world = glm::translate(world, transform.position);
		world = glm::rotate(world, transform.rotation.x, glm::vec3(1, 0, 0));
		world = glm::rotate(world, transform.rotation.y, glm::vec3(0, 1, 0));
		world = glm::rotate(world, transform.rotation.z, glm::vec3(0, 0, 1));
		world = glm::scale(world, transform.scale);

		glm::mat4 wvp = projection * view * world;
		highlightShader.setMat4("WVP", wvp);

		if (reg.any_of<MeshComponent>(clickedEntity))

		{

		}
		else if (reg.any_of<ModelComponent>(clickedEntity))
		{
			auto& model = reg.get<ModelComponent>(clickedEntity);
			model.model->Draw(highlightShader);
		}
	}
}
