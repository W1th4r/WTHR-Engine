#pragma once
#include <pch.hpp>


#define MAX_BONE_INFLUENCE 4

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;

    int m_BoneIDs[MAX_BONE_INFLUENCE];
    float m_Weights[MAX_BONE_INFLUENCE];
};

class Shader;
class Texture;

class Mesh {
public:
    // Constructors
    Mesh();
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);

    // Copy/Move constructors and assignment operators
    Mesh(Mesh&& other) noexcept;
    Mesh(const Mesh& other);
    Mesh& operator=(const Mesh& other);
    Mesh& operator=(Mesh&& other) noexcept;

    // Render the mesh
    void Draw(Shader& shader);

    // Mesh data
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    void createMesh() { setupMesh(); }
private:
    unsigned int VAO, VBO, EBO;
    // Map from context ID to VAO ID
    std::unordered_map<uintptr_t, unsigned int> VAOs;

    // Initializes all buffers and attribute pointers
    void setupMesh();
    void setupMeshForContext(uintptr_t contextID);
};
