#pragma once
#include <pch.hpp>  // includes everything from PCH: Shader, Texture, Mesh, etc.

class Texture;
class Mesh;
class Shader;
class aiMesh;
class aiNode;

extern unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma = false);

class Model
{
public:
	Model();
	Model(const Model& other);
	Model& operator=(const Model& other);
	Model(Model&& other) noexcept;
	Model& operator=(Model&& other) noexcept;

	Model(const std::string& path, bool gamma = false);

	bool LoadWTHR(const std::string&);
	void Draw(Shader& shader);
	std::string getDirectory() { return directory; }
private:
	std::vector<Texture> textures_loaded;
	std::vector<Mesh> meshes;
	std::string directory;
	bool gammaCorrection;

	void loadModel(const std::string& path);
	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
};
