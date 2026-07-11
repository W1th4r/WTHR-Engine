#pragma once
#include <RigidBody.hpp>
#include <Collider.hpp>
#include <Scene.hpp>
#include <functional>
#include <cstdint>

using RigidBodyID = uint32_t;
using ColliderID = uint32_t;


using CollisionCallback = std::function<void(RigidBodyID a, RigidBodyID b)>;


using TriggerCallback = std::function<void(RigidBodyID a, ColliderID trigger)>;


class PhysicsWorld {
public:
    PhysicsWorld();
    PhysicsWorld(Scene*);
    ~PhysicsWorld();

    void stepSimulation(float fixedDeltaTime);
    void SetScene(Scene* scene);

    RigidBody createRigidBody(const RigidBodyDesc& desc);
    void destroyRigidBody(RigidBodyID id);
    RigidBody* getRigidBody(RigidBodyID id);

    Collider createCollider(const ColliderDesc& desc);
    void attachCollider(RigidBodyID body, ColliderID collider);
    void detachCollider(RigidBodyID body, ColliderID collider);

    void setCollisionCallback(CollisionCallback cb);
    void setTriggerCallback(TriggerCallback cb);

private:


    void detectCollision();

    void resolveCollision(RigidBody& bodyA, Collider& colA, Transform& transA, RigidBody& bodyB, Collider& colB, Transform& transB);


    std::vector<RigidBody*>   m_rigidBodies;
    std::vector<Collider*>    m_colliders;
    Scene* m_Scene;
    float gravity = -9.81f;
};
