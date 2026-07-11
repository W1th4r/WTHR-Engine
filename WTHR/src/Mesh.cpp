#include "Mesh.hpp"
//----------------------//
// Constructors
//----------------------//

Mesh::Mesh() : VAO(0), VBO(0), EBO(0) {}

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
	: vertices(std::move(vertices)), indices(std::move(indices)), textures(std::move(textures)) {
	setupMesh();
}

// Move constructor
Mesh::Mesh(Mesh&& other) noexcept
	: vertices(std::move(other.vertices)),
	indices(std::move(other.indices)),
	textures(std::move(other.textures)),
	VAO(other.VAO), VBO(other.VBO), EBO(other.EBO) {
	other.VAO = 0;
	other.VBO = 0;
	other.EBO = 0;
}

// Copy constructor
Mesh::Mesh(const Mesh& other)
	: vertices(other.vertices),
	indices(other.indices),
	textures(other.textures),
	VAO(other.VAO), VBO(other.VBO), EBO(other.EBO) {
	setupMesh(); // Might need adjustment to avoid double VAO/VBO/EBO recreation
}

// Copy assignment operator
Mesh& Mesh::operator=(const Mesh& other) {
	if (this == &other) return *this;

	vertices = other.vertices;
	indices = other.indices;
	textures = other.textures;
	VAO = other.VAO;
	VBO = other.VBO;
	EBO = other.EBO;

	setupMesh();
	return *this;
}

// Move assignment operator
Mesh& Mesh::operator=(Mesh&& other) noexcept {
	if (this == &other) return *this;

	vertices = std::move(other.vertices);
	indices = std::move(other.indices);
	textures = std::move(other.textures);
	VAO = other.VAO;
	VBO = other.VBO;
	EBO = other.EBO;

	other.VAO = 0;
	other.VBO = 0;
	other.EBO = 0;

	return *this;
}

//----------------------//
// Draw Function
//----------------------//

#include <glad/glad.h>
#include <spdlog/spdlog.h>

// Optional: helper to get OpenGL error strings
const char* GetGLErrorString(GLenum err) {
	switch (err) {
	case GL_NO_ERROR: return "GL_NO_ERROR";
	case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
	case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
	case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
	case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
	case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
	case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
	case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
	default: return "UNKNOWN_ERROR";
	}
}
void SafeDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices)
{
#ifdef WANT_DEBUG
	// Log the input parameters
	spdlog::debug("glDrawElements called:");
	spdlog::debug("  Mode: 0x{:X}", mode);
	spdlog::debug("  Count: {}", count);
	spdlog::debug("  Type: 0x{:X}", type);
	spdlog::debug("  Indices pointer: {}", static_cast<const void*>(indices));

	// Check currently bound buffers and VAO
	GLint currentVAO = 0, currentVBO = 0, currentEBO = 0;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &currentVBO);
	glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &currentEBO);

	spdlog::debug("  Bound VAO: {}", currentVAO);
	spdlog::debug("  Bound VBO: {}", currentVBO);
	spdlog::debug("  Bound EBO: {}", currentEBO);

	// Check shader program
	GLint currentProgram = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
	spdlog::debug("  Current Shader Program: {}", currentProgram);

	// Check viewport
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	spdlog::debug("  Viewport: x={}, y={}, w={}, h={}", viewport[0], viewport[1], viewport[2], viewport[3]);

	// Check active texture unit
	GLint activeTex = 0;
	glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTex);
	spdlog::debug("  Active Texture Unit: {}", activeTex - GL_TEXTURE0);

	// Flush any previous GL errors before draw
	while (glGetError() != GL_NO_ERROR) {}
#endif

	// --- Perform the actual draw ---
	glDrawElements(mode, count, type, indices);

#ifdef WANT_DEBUG
	glBindVertexArray(0);       // unbind VAO
	glUseProgram(0);            // unbind shader
	glBindTexture(GL_TEXTURE_2D, 0); // unbind textures
	glActiveTexture(GL_TEXTURE0);    // reset active texture

	// Check for OpenGL errors after draw
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		spdlog::error("OpenGL error after glDrawElements: {}", GetGLErrorString(err));
	}

	spdlog::debug("glDrawElements finished successfully.");
#endif
}

void Mesh::Draw(Shader& shader) {
	unsigned int diffuseNr = 1;
	unsigned int specularNr = 1;
	unsigned int normalNr = 1;
	unsigned int heightNr = 1;

	// 1. Multi-context tracking (Clean approach for editor/game views!)
	uintptr_t contextID = reinterpret_cast<uintptr_t>(glfwGetCurrentContext());
	if (VAOs.find(contextID) == VAOs.end()) {
		setupMeshForContext(contextID);
	}
	unsigned int VAO = VAOs[contextID];

	// 2. FIXED: Bind the shader BEFORE updating its uniform texture slots
	glUseProgram(shader.ID);

	// 3. Bind Textures and set Samplers
	for (unsigned int i = 0; i < textures.size(); i++) {
		glActiveTexture(GL_TEXTURE0 + i);

		std::string number;
		std::string name = textures[i].type;

		if (name == "texture_diffuse") number = std::to_string(diffuseNr++);
		else if (name == "texture_specular") number = std::to_string(specularNr++);
		else if (name == "texture_normal") number = std::to_string(normalNr++);
		else if (name == "texture_height") number = std::to_string(heightNr++);

		// Safely uploads to the active program bound above
		glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), i);
		glBindTexture(GL_TEXTURE_2D, textures[i].id);
	}

	// 4. Bind geometry
	glBindVertexArray(VAO);

#ifdef _DEBUG
	// Keep the safety validation tracking restricted ONLY to debug builds
	GLint currentVAO = 0, currentEBO = 0;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
	glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &currentEBO);
	if (currentVAO == 0 || currentEBO == 0) {
		spdlog::error("Cannot draw Mesh: VAO or EBO uninitialized!");
		return;
	}
#endif

	// 5. High performance draw call without CPU-GPU syncing pipeline stalls
	SafeDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);

	// 6. Cleanup pipeline state
	glBindVertexArray(0);

	// Reset the active texture slots we messed with so we don't leak state leaks
	for (unsigned int i = 0; i < textures.size(); i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	glActiveTexture(GL_TEXTURE0);
}
//----------------------//
// Setup Mesh
//----------------------//
void Mesh::setupMesh() {
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

	// Vertex positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	// Vertex normals
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
	// Texture coords
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
	// Tangent
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
	// Bitangent
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
	// Bone IDs
	glEnableVertexAttribArray(5);
	glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));
	// Bone weights
	glEnableVertexAttribArray(6);
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));

	glBindVertexArray(0);
}
void Mesh::setupMeshForContext(uintptr_t contextID)
{
	if (VAOs.find(contextID) != VAOs.end())
		return; // already set up for this context

	unsigned int VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// Bind the shared VBO/EBO
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

	// Vertex positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	// Vertex normals
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
	// Texture coords
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
	// Tangent
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
	// Bitangent
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
	// Bone IDs
	glEnableVertexAttribArray(5);
	glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));
	// Bone weights
	glEnableVertexAttribArray(6);
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));

	glBindVertexArray(0);

	glBindVertexArray(0);

	VAOs[contextID] = VAO;
}