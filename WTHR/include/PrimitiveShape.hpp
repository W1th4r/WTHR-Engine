#pragma once
#include "Mesh.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <memory>
#include <Texture.hpp>
#include <cmath>

namespace Shapes
{
	// Abstract Base Class for all shapes
	class PrimitiveShape
	{
	public:
		virtual ~PrimitiveShape() = default;
		// Mesh data for each shape (to be used by derived classes)
		Mesh mesh;
		void Draw(Shader& shader) { mesh.Draw(shader); }
	};
}

namespace Shapes
{
	class Cube : public PrimitiveShape
	{
	public:
		Cube()
		{
			static std::vector<Vertex> cubeVertices = {
				{{-0.5f, -0.5f, -0.5f}, {1,0,0}, {0.0f, 0.0f}},
				{{ 0.5f, -0.5f, -0.5f}, {0,1,0}, {1.0f, 0.0f}},
				{{ 0.5f,  0.5f, -0.5f}, {0,0,1}, {1.0f, 1.0f}},
				{{-0.5f,  0.5f, -0.5f}, {1,1,0}, {0.0f, 1.0f}},
				{{-0.5f, -0.5f,  0.5f}, {1,0,1},    {1.0f, 0.0f}},
				{{ 0.5f, -0.5f,  0.5f}, {0,1,1},    {0.0f, 0.0f}},
				{{ 0.5f,  0.5f,  0.5f}, {1,0.5f,0}, {0.0f, 1.0f}},
				{{-0.5f,  0.5f,  0.5f}, {0.5f,0,1}, {1.0f, 1.0f}},
			};


			static std::vector<unsigned int> cubeIndices = {
				0,1,2, 2,3,0,
				4,5,6, 6,7,4,
				4,0,3, 3,7,4,
				1,5,6, 6,2,1,
				4,5,1, 1,0,4,
				3,2,6, 6,7,3
			};

			std::vector<Texture> cubeTextures;

			mesh = Mesh(cubeVertices, cubeIndices, cubeTextures);
		}


	};
}
namespace Shapes
{
	class Sphere : public PrimitiveShape {
	public:
		Sphere(float r, int sec, int st)
			: radius(r), sectors(sec), stacks(st)
		{
			generateMesh();
			uploadToGPU();
		}

		~Sphere() {

		}

	private:
		float radius;
		int sectors, stacks;



		void generateMesh() {
			const float PI = 3.14159265359f;

			mesh.vertices.clear();
			mesh.indices.clear();



			// Generate vertices
			for (int i = 0; i <= stacks; ++i) {
				float stackAngle = PI / 2 - i * (PI / stacks);
				float xy = radius * cosf(stackAngle);
				float z = radius * sinf(stackAngle);

				for (int j = 0; j <= sectors; ++j) {
					float sectorAngle = j * (2 * PI / sectors);

					float x = xy * cosf(sectorAngle);
					float y = xy * sinf(sectorAngle);

					Vertex v;
					v.Position.x = x;
					v.Position.y = y;
					v.Position.z = z;

					float inv = 1.0f / radius;
					v.Normal.x = x * inv;
					v.Normal.y = y * inv;
					v.Normal.z = z * inv;

					mesh.vertices.push_back(v);
				}
			}

			// Generate indices
			for (int i = 0; i < stacks; ++i) {
				int k1 = i * (sectors + 1);
				int k2 = k1 + sectors + 1;

				for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
					if (i != 0) {
						mesh.indices.push_back(k1);
						mesh.indices.push_back(k2);
						mesh.indices.push_back(k1 + 1);
					}
					if (i != stacks - 1) {
						mesh.indices.push_back(k1 + 1);
						mesh.indices.push_back(k2);
						mesh.indices.push_back(k2 + 1);
					}
				}
			}

		}

		void uploadToGPU() {
			mesh.createMesh();
		}
	};
}

