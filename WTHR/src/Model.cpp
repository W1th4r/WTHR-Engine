#include <pch.hpp>
#include "model.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.hpp>
#include <filesystem>

// ---------------- Constructors & Assignment Operators ----------------
Model::Model() : gammaCorrection(false) {}

Model::Model(const Model& other)
{
	__debugbreak(); // breaks when copy constructor is called
}

Model& Model::operator=(const Model& other)
{
	__debugbreak(); // breaks when copy assignment happens
	return *this;
}

Model::Model(Model&& other) noexcept
{
	__debugbreak(); // breaks when move constructor is called
}

Model& Model::operator=(Model&& other) noexcept
{
	__debugbreak(); // breaks when move assignment happens
	return *this;
}

Model::Model(const std::string& path, bool gamma) : gammaCorrection(gamma)
{
	loadModel(path);
}

// ---------------- Public Methods ----------------
void Model::Draw(Shader& shader)
{
	if (this == nullptr) return;
	for (auto& mesh : meshes)
		mesh.Draw(shader);
}

static std::vector<Texture> fallbackVector;

bool Model::LoadWTHR(const std::string& path) {
	std::ifstream inFile(path, std::ios::binary);
	if (!inFile) return false;

	// 1. How many meshes are in this file?
	uint32_t meshCount = 0;
	inFile.read((char*)&meshCount, sizeof(uint32_t));

	std::vector<Texture> fallbackVector;
	Texture* fallbackTex = new Texture;

	fallbackTex->id = fallbackTex->LoadDefaultTexture();
	fallbackTex->type = "texture_diffuse1"; // <--- THIS is what the shader looks for
	fallbackTex->path = "internal_fallback_pink";
	fallbackVector.push_back(*fallbackTex);

	for (uint32_t i = 0; i < meshCount; i++) {
		uint32_t vCount, iCount;
		inFile.read((char*)&vCount, sizeof(uint32_t));
		inFile.read((char*)&iCount, sizeof(uint32_t));

		std::vector<Vertex> vertices(vCount);

		std::vector<unsigned int> indices(iCount);

		inFile.read((char*)&vertices[0], vCount * sizeof(Vertex));
		inFile.read((char*)&indices[0], iCount * sizeof(unsigned int));

		// 1. Create a temporary vector for THIS specific mesh
		// 3. Move everything into the Mesh
		// This matches your Mesh(vector, vector, vector) constructor
		// Remove std::move from fallbackVector
// This forces a COPY for every mesh, keeping the original fallbackVector alive
		meshes.emplace_back(std::move(vertices), std::move(indices), fallbackVector);
	}

	inFile.close();
	return true;
}

// ---------------- Private Methods ----------------
void Model::loadModel(const std::string& path)
{
	std::filesystem::path filePath(path);

	// 1. Check if we have a pre-cooked binary file
	if (filePath.extension() == ".WTHR")
	{
		std::cout << "Loading optimized binary: " << filePath.filename() << std::endl;
		if (LoadWTHR(path)) {
			return; // Success! Skip Assimp entirely.
		}
		std::cout << "Binary load failed, falling back to Assimp..." << std::endl;
	}

	// 2. Fallback: Load via Assimp (The "Slow" Way)
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(
		path, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
		aiProcess_FlipUVs | aiProcess_CalcTangentSpace
	);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
		return;
	}

	directory = filePath.parent_path().string();
	processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(processMesh(mesh, scene));
	}

	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(node->mChildren[i], scene);
	}
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;

	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex;
		glm::vec3 vector;

		vector.x = mesh->mVertices[i].x;
		vector.y = mesh->mVertices[i].y;
		vector.z = mesh->mVertices[i].z;
		vertex.Position = vector;

		if (mesh->HasNormals())
		{
			vector.x = mesh->mNormals[i].x;
			vector.y = mesh->mNormals[i].y;
			vector.z = mesh->mNormals[i].z;
			vertex.Normal = vector;
		}

		if (mesh->mTextureCoords[0])
		{
			glm::vec2 vec;
			vec.x = mesh->mTextureCoords[0][i].x;
			vec.y = mesh->mTextureCoords[0][i].y;
			vertex.TexCoords = vec;

			vector.x = mesh->mTangents[i].x;
			vector.y = mesh->mTangents[i].y;
			vector.z = mesh->mTangents[i].z;
			vertex.Tangent = vector;

			vector.x = mesh->mBitangents[i].x;
			vector.y = mesh->mBitangents[i].y;
			vector.z = mesh->mBitangents[i].z;
			vertex.Bitangent = vector;
		}
		else
		{
			vertex.TexCoords = glm::vec2(0.0f, 0.0f);
		}

		vertices.push_back(vertex);
	}

	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];



	// Correct way to pass the type names to the shader
	auto diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
	textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

	auto specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
	textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

	auto normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
	textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

	auto heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, directory + "texture_height");
	textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

	return Mesh(vertices, indices, textures);
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
{
	std::vector<Texture> textures;
	for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
	{
		aiString str;
		mat->GetTexture(type, i, &str);

		// Use std::string for the path logic
		std::string filename = std::string(str.C_Str());

		// CHECK CACHE: Use the relative filename for comparison
		bool skip = false;
		for (auto& loadedTex : textures_loaded)
		{
			if (std::strcmp(loadedTex.path.data(), filename.c_str()) == 0)
			{
				textures.push_back(loadedTex);
				skip = true;
				break;
			}
		}

		if (!skip)
		{
			Texture texture;
			// Only combine the path inside the loader function
			texture.id = TextureFromFile(filename.c_str(), this->directory);
			texture.type = typeName;
			texture.path = filename;
			textures.push_back(texture);
			textures_loaded.push_back(texture);
		}
	}
	return textures;
}

unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma)
{
	// Convert to filesystem paths
	std::filesystem::path dirPath(directory);
	std::filesystem::path fileName(path);

	// The / operator automatically handles slashes correctly for Windows/Linux
	// Result: "C:/Models/Backpack" + "diffuse.png" -> "C:/Models/Backpack/diffuse.png"
	std::filesystem::path fullPath = dirPath / fileName;

	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	stbi_set_flip_vertically_on_load(true);
	// Use .string().c_str() to get the absolute system path
	unsigned char* data = stbi_load(fullPath.string().c_str(), &width, &height, &nrComponents, 0);

	if (data)
	{
		GLenum format = GL_RGB;
		if (nrComponents == 1) format = GL_RED;
		else if (nrComponents == 3) format = GL_RGB;
		else if (nrComponents == 4) format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		// Standard wrapping/filtering
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << fullPath << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}