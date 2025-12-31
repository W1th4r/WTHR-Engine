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
	CreateShaderProgram();
	spdlog::info("Renderer initialized");

	pickingShader = Shader("shaders/picking.vert", "shaders/picking.frag");

	// In your main application initialization

}

void Renderer::Clear(const glm::vec3& color)
{
	glClearColor(color.r, color.g, color.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::DrawTriangle()
{
	glUseProgram(m_ShaderProgram);
	glBindVertexArray(m_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
}
void Renderer::DrawTriangle(const glm::vec3& color)
{
	glUseProgram(m_ShaderProgram);

	// Set uniform
	GLint loc = glGetUniformLocation(m_ShaderProgram, "u_Color");
	glUniform3f(loc, color.r, color.g, color.b);

	glBindVertexArray(m_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
}

void Renderer::RenderScene(Scene& scene, Shader& shader)
{
	// Cache camera and projection matrix
	Camera& camera = scene.GetCamera();
	auto view = camera.GetViewMatrix();

	// Cache projection matrix - compute once
	static float nearPlane = 0.1f;
	static float farPlane = 100.f;
	static glm::mat4 projection = glm::perspective(
		glm::radians(45.f),
		static_cast<float>(width) / static_cast<float>(height),
		nearPlane, farPlane
	);

	auto& registry = scene.GetRegistry();

	// Get uniform locations once
	GLint useModelLoc = glGetUniformLocation(shader.ID, "useModel");
	GLint useTextureLoc = glGetUniformLocation(shader.ID, "useTexture");
	GLint useColorLoc = glGetUniformLocation(shader.ID, "useColor");
	GLint uColorLoc = glGetUniformLocation(shader.ID, "uColor");

	shader.use();
	shader.setMat4("view", view);
	shader.setMat4("projection", projection);

	// Render MeshComponents
	registry.view<MeshComponent, Transform>().each([&](auto entity, auto& meshComp, auto& transform) {
		// Skip if mesh is null
		if (!meshComp.mesh) return;

		// Build model matrix
		glm::mat4 model = BuildModelMatrix(transform);
		shader.setMat4("model", model);

		// Check for Color component once
		bool hasColor = scene.HasComponent<Color>(entity);
		bool hasTextures = !meshComp.mesh->mesh.textures.empty();

		// Set uniform flags
		glUniform1i(useModelLoc, 0); // Not a ModelComponent

		if (!hasTextures && hasColor) {
			// Only color, no textures
			glUniform1i(useTextureLoc, 0);
			glUniform1i(useColorLoc, 1);

			auto& color = scene.GetComponent<Color>(entity);
			glUniform4f(uColorLoc,
				color.value.r, color.value.g,
				color.value.b, color.value.a);
		}
		else if (hasTextures) {
			// Has textures, may have color
			glUniform1i(useTextureLoc, 1);
			glUniform1i(useColorLoc, hasColor ? 1 : 0);

			if (hasColor) {
				auto& color = scene.GetComponent<Color>(entity);
				glUniform4f(uColorLoc,
					color.value.r, color.value.g,
					color.value.b, color.value.a);
			}
		}
		else {
			// No textures, no color
			glUniform1i(useTextureLoc, 0);
			glUniform1i(useColorLoc, 0);
		}

		meshComp.mesh->Draw(shader);
		});

	// Render ModelComponents
	registry.view<ModelComponent, Transform>().each([&](auto entity, ModelComponent& modelComp, auto& transform) {
		// Skip if not loaded
		if (!modelComp.model.get()->IsLoaded()) return;

		glm::mat4 model = BuildModelMatrix(transform);
		shader.setMat4("model", model);

		glUniform1i(useModelLoc, 1);
		modelComp.model.get()->Draw(shader);
		});

	// Render Gizmo if needed (consider separating this into its own function)
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
	if (!pixel.ObjectID) return;

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

GLuint Renderer::CompileShader(const std::string& source, GLenum type)
{
	GLuint shader = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);

	int success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		char info[512];
		glGetShaderInfoLog(shader, 512, nullptr, info);
		spdlog::error("Shader compilation failed: {}", info);
	}
	return shader;
}

void Renderer::CreateShaderProgram()
{
	const std::string vertexShader = R"(
        #version 460 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aColor;
        out vec3 vColor;
        void main()
        {
            vColor = aColor;
            gl_Position = vec4(aPos, 1.0);
        }
    )";

	const std::string fragmentShader = R"(
        #version 460 core
        in vec3 vColor;
        uniform vec3 u_Color; // ImGui-controlled color
        out vec4 FragColor;
        void main()
        {
            FragColor = vec4(u_Color * vColor, 1.0);
        }
    )";

	GLuint vs = CompileShader(vertexShader, GL_VERTEX_SHADER);
	GLuint fs = CompileShader(fragmentShader, GL_FRAGMENT_SHADER);

	m_ShaderProgram = glCreateProgram();
	glAttachShader(m_ShaderProgram, vs);
	glAttachShader(m_ShaderProgram, fs);
	glLinkProgram(m_ShaderProgram);

	int success;
	glGetProgramiv(m_ShaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		char info[512];
		glGetProgramInfoLog(m_ShaderProgram, 512, nullptr, info);
		spdlog::error("Shader linking failed: {}", info);
	}

	glDeleteShader(vs);
	glDeleteShader(fs);
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
	glViewport(0, 0, width,height);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
	if (pixel.ObjectID != 0)
	{
		entt::entity clickedEntity = static_cast<entt::entity>(pixel.ObjectID);
		spdlog::debug("Clicked entity ID: {}", pixel.ObjectID);
		// You could highlight or select it here
	}
}


void Renderer::HandlePickingClick(Scene& scene, double mouseX, double mouseY,entt::entity& picked)
{
	// 1. Read pixel info from picking framebuffer
	pixel = m_ObjectPicking.ReadPixel(
		static_cast<int>(mouseX),
		static_cast<int>(height - mouseY - 1) // flip Y
	);

	pixel.Print(); // Optional debug output

	// 2. If no object was clicked, return early
	if (pixel.ObjectID == 0)
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
