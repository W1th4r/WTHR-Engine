#pragma once

#include <assimp/Importer.hpp>
#include <string>
#include <vector>
#include "Mesh.hpp" 

class AssetCompiler {
public:
    // Main entry point: Takes "tree.obj", produces "tree.wthr"
    bool CookModel(const std::string& inputPath, const std::string& outputPath);

private:
    // Step 1: Wrap Assimp logic
    bool ImportOBJ(const std::string& path);

    // Step 2: The "Programmer Power" logic (Fixing the artist's mistakes)
    void FindTexturesHeuristic(const std::string& modelFolder);

    // Step 3: Write the WTHR binary file
    bool SaveWTHR(const std::string& path, const std::vector<Mesh>& meshes);

    void FindTexturesHeurstic(const std::string);

    // Data held during the cooking process
    std::vector<Vertex> m_Vertices;
    std::vector<unsigned int> m_Indices;
    std::vector<std::string> m_TexturePaths;
    std::string m_Directory;
};