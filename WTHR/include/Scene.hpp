#pragma once
#include <entt/entt.hpp>
#include <memory>
#include <filesystem>

#include <Scripts.hpp>
#include "Components.hpp"
#include "PrimitiveShape.hpp"
#include "Model.hpp"
#include <GLContextWorker.hpp>
#include "Camera.hpp"

enum class CameraType {
	Player,
	Editor,
	FreeCam
};

class Scene {
public:
	Scene() {}
	//Scene(GLFWwindow* window) : worker(window)
	//{
	//	spdlog::set_level(spdlog::level::debug);
	//	spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
	//}
	~Scene()
	{
		delete m_Worker;
		m_Worker = nullptr;
	}
	void SetWorker(GLFWwindow* window)
	{
		// Use 'new' because it's a pointer. 
		// This calls the explicit constructor.
		m_Worker = new GLContextWorker(window);
	}
	void print_transform(const Transform& t) {
		std::cout << "Position: (" << t.position.x << ", " << t.position.y << ", " << t.position.z << ")\n";
		std::cout << "Rotation: (" << t.rotation.x << ", " << t.rotation.y << ", " << t.rotation.z << ")\n";
		std::cout << "Scale:    (" << t.scale.x << ", " << t.scale.y << ", " << t.scale.z << ")\n";
	}
	entt::registry& GetRegistry() { return m_Registry; }
	Camera& GetCamera()
	{
		Camera* foundCamera = nullptr;

		switch (m_CameraType)
		{
		case CameraType::Player:
			m_Registry.view<Camera>().each([&](auto entity, Camera& camera) {
				// Here, decide which camera is the player camera
				// For example, check if this entity has PlayerController
				if (m_Registry.any_of<Camera>(entity)) {
					foundCamera = &camera;
				}
				});
			if (foundCamera) return *foundCamera;
			//throw std::runtime_error("No camera found for the selected type");
			return m_Camera;
			break;
		case CameraType::Editor:
			return m_Camera;
			break;
		case CameraType::FreeCam:
			break;
		}

	}
	CameraType GetCameraType() { return m_CameraType; }
	void setCameraType(CameraType type) { m_CameraType = type; }
	std::unordered_map<std::string, Texture>& GetTextures() { return m_Textures; }; // path or name → texture data



	void CreateCubeGrid(int width, int length, int height, glm::vec3 pos = glm::vec3(0.f))
	{

		for (int i = 0; i < width; i++)
		{
			for (int j = 0; j < length; j++)
			{
				for (int k = 0; k < height; k++)
				{
					CreateCube(glm::vec3(i * 1.1, j * 1.1, k * 1.1));
				}
			}
		}

	}

	entt::entity CreatePlayer(const glm::vec3& position = glm::vec3(0.f)) {
		auto entity = m_Registry.create();
		Transform& trans = m_Registry.emplace<Transform>(entity, position);
		m_Registry.emplace<PlayerController>(entity, PlayerController());
		m_Registry.emplace<MeshComponent>(entity, std::make_shared<Shapes::Cube>());
		Camera& camera = m_Registry.emplace<Camera>(entity, Camera());
		trans.scale.y *= 2;
		camera.GetViewMatrix();
		camera.Front = glm::vec3(0.f);
		camera.Position = glm::vec3(0.f);



		return entity;
	}



	// -------------------------
	// Helper to create a cube
	// -------------------------
	entt::entity CreateCube(const glm::vec3& position = glm::vec3(0.f)) {
		auto entity = m_Registry.create();
		m_Registry.emplace<Transform>(entity, position);
		auto& mesh = m_Registry.emplace<MeshComponent>(entity, std::make_shared<Shapes::Cube>());
		auto it = m_Textures.find("stone.png");
		if (it != m_Textures.end())
		{
			mesh.mesh->mesh.textures.push_back(it->second);
			m_Registry.emplace<Texture>(entity);
		}
		return entity;
	}
	entt::entity CreateSphere(const glm::vec3& position = glm::vec3(0.f)) {
		auto entity = m_Registry.create();
		m_Registry.emplace<Transform>(entity, position);
		auto& mesh = m_Registry.emplace<MeshComponent>(entity, std::make_shared<Shapes::Sphere>(0.5f, 36, 18));
		return entity;
	}
	entt::entity CreateBullet() {
		auto entity = m_Registry.create();
		m_Registry.emplace<Transform>(entity, GetCamera().Position);
		m_Registry.emplace<Color>(entity, glm::vec4(1.f));
		m_Registry.emplace<Bullet>(entity, GetCamera().Position,GetCamera().Front,0.1f,true);
		m_Registry.emplace<MeshComponent>(entity, std::make_shared<Shapes::Sphere>(0.1f, 36, 18));
		return entity;
	}

	// -------------------------
	// Helper to create a model (async)
	// -------------------------
	entt::entity CreateModel(const std::string& path, const glm::vec3& position = glm::vec3(0.f))
	{
		auto entity = m_Registry.create();

		// Add a Transform component
		m_Registry.emplace<Transform>(entity, position);

		// Create a ThreadSafeModel wrapper
		auto modelWrapper = std::make_shared<ThreadSafeModel>();

		// Add the ModelComponent to the registry
		m_Registry.emplace<ModelComponent>(entity, modelWrapper);

		// Reset wrapper just in case
		modelWrapper->Reset();

		// Queue asynchronous GPU load
		m_Worker->AsyncLoadModel(modelWrapper, path);


		return entity;
	}

	void Save(const std::filesystem::path&);
	void Load(const std::filesystem::path&);


	template<typename T>
	bool HasComponent(entt::entity entity) {
		return m_Registry.any_of<T>(entity);
	}

	template<typename T>
	T& GetComponent(entt::entity entity) {
		return m_Registry.get<T>(entity);
	}

	template<typename T, typename... Args>
	T& AddComponent(entt::entity entity, Args&&... args) {
		return m_Registry.emplace<T>(entity, std::forward<Args>(args)...);
	}
	GLContextWorker* m_Worker;
	Script m_Script;
private:
	std::unordered_map<std::string, Texture> m_Textures; // path or name → texture data
	entt::registry m_Registry;
	Camera m_Camera;
	CameraType m_CameraType = CameraType::Editor;
};
