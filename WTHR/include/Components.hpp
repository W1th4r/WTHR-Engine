#pragma once
#include <pch.hpp>
#include <glm/glm.hpp>
#include <memory>
#include "Mesh.hpp"
#include "Model.hpp"
#include <PrimitiveShape.hpp>
#include <glm/gtc/quaternion.hpp>
#include <chrono>

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
	// Pass the shader in! Don't compile it inside the loop.
	void GenerateThumbnail() {
		Shader thumbShader("shaders/thumb.vert", "shaders/thumb.frag");
		// 1. Prepare destination
		m_Thumbnail.CreateEmpty(256, 256);

		// 2. Create tools
		unsigned int tempFBO, tempRBO;
		glGenFramebuffers(1, &tempFBO);
		glGenRenderbuffers(1, &tempRBO); // Moved up for cleanliness

		glBindFramebuffer(GL_FRAMEBUFFER, tempFBO);

		// 3. Attach Texture
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_Thumbnail.id, 0);

		// 4. Attach Depth
		glBindRenderbuffer(GL_RENDERBUFFER, tempRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 256, 256);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, tempRBO);

		// 5. RENDER
		// Store old viewport to restore it later
		GLint oldViewport[4];
		glGetIntegerv(GL_VIEWPORT, oldViewport);

		glViewport(0, 0, 256, 256);
		glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		thumbShader.use();
		glm::mat4 view = glm::lookAt(glm::vec3(5, 4, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		glm::mat4 proj = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);

		thumbShader.setMat4("view", view);
		thumbShader.setMat4("projection", proj);
		thumbShader.setMat4("model", glm::mat4(1.0f));

		// Use 'this' if this function is inside the Model class
		this->Draw(thumbShader);

		// 6. CLEANUP
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glFlush();
		glDeleteRenderbuffers(1, &tempRBO);
		glDeleteFramebuffers(1, &tempFBO);

		// Restore the game's viewport so the UI doesn't shrink to 256x256!
		glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
	}
	std::string getName() { return model->getDirectory(); }

	std::chrono::duration<double> m_LoadTime;
	int m_FileSize;
	unsigned int GetThumbnail() { return m_Thumbnail.id; }
private:
	std::unique_ptr<Model> model;
	std::mutex modelMutex;
	std::atomic<bool> isLoaded;
	Texture m_Thumbnail;
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