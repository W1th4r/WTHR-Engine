#pragma once
#include <cstdint>
#include <glm/vec3.hpp>


struct RigidBodyDesc {
    glm::vec3 position{ 0.0f };
    glm::vec3 velocity{ 0.0f };
    float mass = 1.0f;
    bool isKinematic = false;
    bool useGravity = false;
};

class RigidBody {
public:
    RigidBody(const RigidBodyDesc& desc)
        : position(desc.position),
        velocity(desc.velocity),
        mass(desc.mass),
        isKinematic(desc.isKinematic),
        useGravity(desc.useGravity)
    {
    }

    glm::vec3 position;
    glm::vec3 velocity;
    float mass;
    bool useGravity;
    bool isKinematic;
 
};
