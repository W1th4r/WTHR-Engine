#include "Scene.hpp"
#include <spdlog/spdlog.h> // optional for logging
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <entt/entt.hpp>
#include "Components.hpp" // Transform, MeshComponent, etc.

using json = nlohmann::json;

void Scene::Save(const std::filesystem::path& filepath)
{
	json sceneJson;


	json modelsJson;
	m_Registry.view<ModelComponent>().each([&](entt::entity entity, ModelComponent& model)
		{
			std::string name = std::to_string((uint32_t)entity);
			modelsJson[name] = model.model->getName();
		});
	sceneJson["Models"].push_back(modelsJson);



	json cameraJson;
	cameraJson["Position"] = { m_Camera.Position.x, m_Camera.Position.y, m_Camera.Position.z };
	cameraJson["Front"] = { m_Camera.Front.x,    m_Camera.Front.y,    m_Camera.Front.z };
	cameraJson["Up"] = { m_Camera.Up.x,       m_Camera.Up.y,       m_Camera.Up.z };
	cameraJson["Right"] = { m_Camera.Right.x,    m_Camera.Right.y,    m_Camera.Right.z };
	cameraJson["WorldUp"] = { m_Camera.WorldUp.x,  m_Camera.WorldUp.y,  m_Camera.WorldUp.z };
	cameraJson["Pitch"] = m_Camera.Pitch;
	cameraJson["Yaw"] = m_Camera.Yaw;

	sceneJson["Camera"] = cameraJson;


	//Scripts
	json attachmentsJson = json::array();  // top-level array

	for (auto& [entity, scripts] : m_Script.objectScripts)
	{
		json entityJson;
		entityJson["entity"] = entity;

		json scriptsArray = json::array();
		for (auto& s : scripts)
		{
			scriptsArray.push_back(s);  // push each script into array
		}

		// Assign scripts array directly to entity object
		entityJson["scripts"] = scriptsArray;

		// Push the entity object into the attachments array
		attachmentsJson.push_back(entityJson);
	}

	// Assign the array to sceneJson
	sceneJson["Attachments"] = attachmentsJson;


	json scriptsJson;
	for (auto& [filepath, script] : m_Script.scripts)
	{
		scriptsJson["Code"] = script.code;
		scriptsJson["Name"] = script.name;


		sceneJson["Scripts"].push_back(scriptsJson);
	}

	for (auto& [path, texture] : m_Textures)
	{
		json textureJson;
		textureJson["name"] = "NAME:";
		textureJson["path"] = path;
		sceneJson["textures"].push_back(textureJson);
	}


	// Entites
	m_Registry.view<Transform>().each([&](entt::entity entity, Transform& trans) {
		json entityJson;
		entityJson["id"] = static_cast<uint32_t>(entity);

		if (m_Registry.any_of<MeshComponent>(entity)) {
			auto& meshComp = m_Registry.get<MeshComponent>(entity);
			if (meshComp.mesh) {
				json meshJson;

				if (auto cube = dynamic_cast<Shapes::Cube*>(meshComp.mesh.get())) {
					meshJson["type"] = "Cube";
					//	meshJson["size"] = cube->size;
				}
				else if (auto sphere = dynamic_cast<Shapes::Sphere*>(meshComp.mesh.get())) {
					meshJson["type"] = "Sphere";
					//	meshJson["radius"] = sphere->radius;
				}

				entityJson["MeshComponent"] = meshJson;
			}
		}


		// Transform
		if (m_Registry.any_of<Transform>(entity)) {
			auto& transform = m_Registry.get<Transform>(entity);
			entityJson["Transform"] = {
				{"position", {transform.position.x, transform.position.y, transform.position.z}},
				{"rotation", {transform.rotation.x, transform.rotation.y, transform.rotation.z}},
				{"scale",    {transform.scale.x,    transform.scale.y,    transform.scale.z}}
			};
		}
		if (m_Registry.any_of<Color>(entity)) {
			auto& color = m_Registry.get<Color>(entity);
			entityJson["Color"] = {
				{"whatColor", {color.value.r, color.value.g, color.value.b,color.value.a}}
			};
		}
		if (m_Registry.any_of<Texture>(entity)) {
			auto& texture = m_Registry.get<MeshComponent>(entity);
			entityJson["Texture"] = texture.mesh.get()->mesh.textures[0].path;
		}


		sceneJson["entities"].push_back(entityJson);
		});



	// Save to file
	std::ofstream file(filepath);
	if (file.is_open()) {
		file << sceneJson.dump(4); // pretty print with 4 spaces
	}
}

void Scene::Load(const std::filesystem::path& filepath)
{
	std::ifstream file(filepath);
	if (!file.is_open()) return;

	json sceneJson;
	file >> sceneJson;

	m_Registry.clear(); // Remove existing entities
	m_Textures.clear();
	m_Script.scripts.clear();
	m_Script.objectScripts.clear();



	for (auto& scriptJson : sceneJson["Scripts"])
	{
		std::string name, code;
		if (scriptJson.contains("Name"))
		{
			auto string = scriptJson["Name"].get<std::string>();
			name = string;
		}
		if (scriptJson.contains("Code"))
		{
			auto string = scriptJson["Code"].get<std::string>();
			code = string;
		}
		m_Script.addScript(name, code);
	}


	for (auto& textureJson : sceneJson["textures"])
	{

		if (textureJson.contains("path"))
		{
			auto stringJson = textureJson["path"].get<std::string>();;
			Texture texture(stringJson, "texture_diffuse");
			m_Textures.insert({ stringJson,texture });
		}
	}

	for (auto& entityJson : sceneJson["entities"]) {
		// Load entity ID from JSON
		entt::entity entity = static_cast<entt::entity>(entityJson["id"].get<std::uint32_t>());

		// Make sure the registry has this entity
		if (!m_Registry.valid(entity))
		{
			m_Registry.create(entity); // create entity with specific ID
		}


		if (entityJson.contains("MeshComponent")) {
			auto& meshJson = entityJson["MeshComponent"];
			MeshComponent meshComp;

			if (meshJson["type"] == "Cube") {
				auto cube = std::make_shared<Shapes::Cube>();
				//	cube->size = meshJson["size"];
				meshComp.mesh = cube;
			}
			else if (meshJson["type"] == "Sphere") {
				auto sphere = std::make_shared<Shapes::Sphere>(0.5f, 36, 18);
				//	sphere->radius = meshJson["radius"];
				meshComp.mesh = sphere;
			}

			m_Registry.emplace<MeshComponent>(entity, meshComp);
		}


		// Transform
		if (entityJson.contains("Transform")) {
			Transform t;
			auto& tJson = entityJson["Transform"];
			t.position = glm::vec3(tJson["position"][0], tJson["position"][1], tJson["position"][2]);
			t.rotation = glm::vec3(tJson["rotation"][0], tJson["rotation"][1], tJson["rotation"][2]);
			t.scale = glm::vec3(tJson["scale"][0], tJson["scale"][1], tJson["scale"][2]);
			m_Registry.emplace<Transform>(entity, t);
		}
		if (entityJson.contains("Color")) {
			Color c;
			auto& tJson = entityJson["Color"];
			c.value = glm::vec4(tJson["whatColor"][0], tJson["whatColor"][1], tJson["whatColor"][2], tJson["whatColor"][3]);
			m_Registry.emplace<Color>(entity, c);
		}



		if (entityJson.contains("Texture")) {
			Texture texture;
			auto strPath = entityJson["Texture"].get<std::string>();
			auto& compp = GetComponent<MeshComponent>(entity);

			texture.LoadFromFile(strPath, "diffuse_texture");
			compp.mesh->mesh.textures.push_back(texture);
			m_Registry.emplace<Texture>(entity);
		}

	}

	const json& attachments = sceneJson["Attachments"];

	for (const auto& entityJson : attachments)
	{
		if (!entityJson.contains("entity") || !entityJson.contains("scripts"))
			continue; // skip invalid entries

		// Convert JSON entity to entt::entity
		entt::entity entity = static_cast<entt::entity>(entityJson["entity"].get<std::uint32_t>());

		const json& scriptsArray = entityJson["scripts"];
		if (!scriptsArray.is_array())
			continue; // scripts should be an array

		std::vector<std::string> scripts;
		for (const auto& s : scriptsArray)
		{
			scripts.push_back(s.get<std::string>());
		}

		m_Script.objectScripts[entity] = scripts;
	}




	if (sceneJson.contains("Models") && sceneJson["Models"].is_array())
	{
		const auto& modelsArray = sceneJson["Models"];

		for (const auto& objGroup : modelsArray)
		{
			if (!objGroup.is_object()) continue;

			for (auto it = objGroup.begin(); it != objGroup.end(); ++it)
			{
				std::string modelPath = it.value().get<std::string>();
				CreateModel(modelPath);  // example
			}
		}
	}



	if (sceneJson.contains("Camera"))
	{
		const auto& cameraJson = sceneJson["Camera"];

		m_Camera.Position = glm::vec3(
			cameraJson["Position"][0],
			cameraJson["Position"][1],
			cameraJson["Position"][2]
		);

		m_Camera.Front = glm::vec3(
			cameraJson["Front"][0],
			cameraJson["Front"][1],
			cameraJson["Front"][2]
		);

		m_Camera.Up = glm::vec3(
			cameraJson["Up"][0],
			cameraJson["Up"][1],
			cameraJson["Up"][2]
		);

		m_Camera.Right = glm::vec3(
			cameraJson["Right"][0],
			cameraJson["Right"][1],
			cameraJson["Right"][2]
		);

		m_Camera.WorldUp = glm::vec3(
			cameraJson["WorldUp"][0],
			cameraJson["WorldUp"][1],
			cameraJson["WorldUp"][2]
		);

		m_Camera.Pitch = cameraJson["Pitch"];
		m_Camera.Yaw = cameraJson["Yaw"];

	}

	m_Script.bindTransforms(m_Registry);

}
