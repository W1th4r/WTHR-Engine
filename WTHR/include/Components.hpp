#pragma once
#include <pch.hpp>
#include <glm/glm.hpp>
#include <memory>
#include "Mesh.hpp"
#include "Model.hpp"
#include <PrimitiveShape.hpp>
#include <glm/gtc/quaternion.hpp>

//Color component
struct Color {
	Color() { value = glm::vec4(1.f); }
	Color(glm::vec4 col) : value(col) {}
	glm::vec4 value{ 1.0f, 1.0f, 1.0f, 1.0f }; // RGBA, default white
};

// Transform component
struct Transform {
	glm::vec3 position{ 0.0f };
	glm::vec3 rotation{ 0.0f }; // Euler angles in degrees
	glm::vec3 scale{ 1.0f };

	void SetFromMatrix(const glm::mat4& matrix)
	{
		// Extract translation
		position = glm::vec3(matrix[3]);

		// Extract scale
		scale.x = glm::length(glm::vec3(matrix[0]));
		scale.y = glm::length(glm::vec3(matrix[1]));
		scale.z = glm::length(glm::vec3(matrix[2]));

		// Remove scale from rotation matrix
		glm::mat3 rotationMatrix;
		rotationMatrix[0] = glm::vec3(matrix[0]) / scale.x;
		rotationMatrix[1] = glm::vec3(matrix[1]) / scale.y;
		rotationMatrix[2] = glm::vec3(matrix[2]) / scale.z;

		// Extract Euler angles (in radians)
		rotation = glm::degrees(glm::eulerAngles(glm::quat_cast(rotationMatrix)));
	}

};


// Mesh component (wraps a primitive shape)
struct MeshComponent {
	std::shared_ptr<Shapes::PrimitiveShape> mesh;
};

class ThreadSafeModel {
public:
	ThreadSafeModel() : isLoaded(false) {}

	void SetModel(std::unique_ptr<Model> newModel) {
		std::lock_guard<std::mutex> lock(modelMutex);
		model = std::move(newModel);
		isLoaded = (model != nullptr);
	}

	bool Draw(Shader& shader) {
		std::lock_guard<std::mutex> lock(modelMutex);
		if (model && isLoaded) {
			model->Draw(shader);
			return true;
		}
		return false;
	}

	bool IsLoaded() const {
		return isLoaded;
	}

	void Reset() {
		std::lock_guard<std::mutex> lock(modelMutex);
		model.reset();
		isLoaded = false;
	}
	std::string getName() { return model->getDirectory(); }
private:
	std::unique_ptr<Model> model;
	std::mutex modelMutex;
	std::atomic<bool> isLoaded;
};


struct ModelComponent {
    std::shared_ptr<ThreadSafeModel> model;
};
struct PlayerController {
	glm::vec3 velocity;      
	glm::vec3 halfExtents;   
	bool isGrounded = false; 
	float moveSpeed = 5.0f;  
	float jumpForce = 10.0f; 
};
struct GroupComponent
{
	int id;
};
struct Bullet {
	Bullet(glm::vec3 pos, glm::vec3 vec, float ra, bool ac)
	{
		position = pos;
		velocity = vec * bulletSpeed;
		radius = ra;
		active = ac;
	}
	glm::vec3 position;
	glm::vec3 velocity;
	float bulletSpeed = 10.f;
	float radius;
	bool active;
};