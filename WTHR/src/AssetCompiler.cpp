#include "AssetCompiler.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

bool AssetCompiler::CookModel(const std::string& inputPatht, const std::string& outputPath) {
    std::string inputPath = "backpack.obj";
    Assimp::Importer importer;
    // Step 1: Import with full processing (matching your loadModel flags)
    const aiScene* scene = importer.ReadFile(inputPath,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        return false;
    }

    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile) return false;

    // 1. Write the total number of meshes in the scene
    uint32_t meshCount = scene->mNumMeshes;
    outFile.write((char*)&meshCount, sizeof(uint32_t));

    // 2. Loop through every mesh in the OBJ
    for (unsigned int m = 0; m < meshCount; m++) {
        aiMesh* mesh = scene->mMeshes[m];
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        // Extract Vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            // Position
            vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

            // Normals
            if (mesh->HasNormals())
                vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

            // TexCoords
            if (mesh->mTextureCoords[0])
                vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }

        // Extract Indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        // 3. The "Chunk" Export for this specific mesh
        uint32_t vCount = static_cast<uint32_t>(vertices.size());
        uint32_t iCount = static_cast<uint32_t>(indices.size());

        outFile.write((char*)&vCount, sizeof(uint32_t));
        outFile.write((char*)&iCount, sizeof(uint32_t));

        outFile.write((char*)&vertices[0], vCount * sizeof(Vertex));
        outFile.write((char*)&indices[0], iCount * sizeof(unsigned int));
    }

    outFile.close();
    return true;
}

bool AssetCompiler::ImportOBJ(const std::string& path)
{
    return false;
}

void AssetCompiler::FindTexturesHeuristic(const std::string& modelFolder) {
    namespace fs = std::filesystem;

    for (const auto& entry : fs::directory_iterator(modelFolder)) {
        if (!entry.is_regular_file()) continue;

        std::string filename = entry.path().filename().string();
        std::string ext = entry.path().extension().string();

        // Only look for images
        if (ext == ".png" || ext == ".jpg" || ext == ".tga") {
            // Check for keywords (case-insensitive)
            std::string lower = filename;
            for (char& c : lower) c = std::tolower(c);

            if (lower.find("diff") != std::string::npos || lower.find("_d") != std::string::npos || lower.find("_s") != std::string::npos) {
                m_TexturePaths.push_back(filename); // Found a Diffuse/Surface map
                std::cout << "Cooker: Automatically linked diffuse: " << filename << std::endl;
            }
            else if (lower.find("norm") != std::string::npos || lower.find("_n") != std::string::npos) {
                // Future-proofing: Store normal maps separately in your WTHR format
                std::cout << "Cooker: Found normal map candidate: " << filename << std::endl;
            }
        }
    }
}

bool AssetCompiler::SaveWTHR(const std::string& path, const std::vector<Mesh>& meshes) {
    std::ofstream outFile(path, std::ios::binary);

    // 1. Write the TOTAL number of meshes
    uint32_t meshCount = static_cast<uint32_t>(meshes.size());
    outFile.write((char*)&meshCount, sizeof(uint32_t));

    for (const Mesh& mesh : meshes) {
        // 2. Write the counts for THIS specific mesh
        uint32_t vCount = (uint32_t)mesh.vertices.size();
        uint32_t iCount = (uint32_t)mesh.indices.size();

        outFile.write((char*)&vCount, sizeof(uint32_t));
        outFile.write((char*)&iCount, sizeof(uint32_t));

        // 3. The Raw Pour for THIS mesh
        outFile.write((char*)&mesh.vertices[0], vCount * sizeof(Vertex));
        outFile.write((char*)&mesh.indices[0], iCount * sizeof(unsigned int));

        // Note: You'll eventually need to save texture info here too!
    }

    outFile.close();
    return true;
}