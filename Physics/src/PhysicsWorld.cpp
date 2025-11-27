#include "PhysicsWorld.hpp"

PhysicsWorld::PhysicsWorld() : m_Scene(nullptr)
{

}

PhysicsWorld::PhysicsWorld(Scene* scene) : m_Scene(scene)
{

}
void PhysicsWorld::SetScene(Scene* scene)
{
	m_Scene = scene;
}

PhysicsWorld::~PhysicsWorld()
{
}

void PhysicsWorld::stepSimulation(float fixedDeltaTime)
{
	entt::registry& reg = m_Scene->GetRegistry();
	reg.view<RigidBody, Transform>().each([&](entt::entity entity, RigidBody& body, Transform& transform) {

		// Apply gravity if dynamic and enabled
		if (!body.isKinematic && body.useGravity) {
			body.velocity.y += gravity * fixedDeltaTime;
		}

		// Update position
		body.position += body.velocity * fixedDeltaTime;

		// Clamp Y so object doesn't fall below -5
		if (body.position.y < -5.0f) {
			body.position.y = -5.0f;

			// Optional: zero velocity when hitting the floor
			if (body.velocity.y < 0.0f) {
				body.velocity.y = 0.0f;
			}
		}

		// Kinematic movement logic (if any)
		if (body.isKinematic) {
			// body.position = yourManualMovementFunction(entity);
		}

		// Sync transform
		transform.position = body.position;
		});

	detectCollision();
	reg.view<Bullet, Transform>().each([&](entt::entity entity, Bullet& bullet, Transform& transform) {
		if (bullet.active)
		{
			bullet.position += bullet.velocity * fixedDeltaTime;

			transform.position = bullet.position;
		}


		});



}



RigidBody PhysicsWorld::createRigidBody(const RigidBodyDesc& desc)
{
	return RigidBody(desc);
}

void PhysicsWorld::destroyRigidBody(RigidBodyID id)
{
}

RigidBody* PhysicsWorld::getRigidBody(RigidBodyID id)
{
	return nullptr;
}

Collider PhysicsWorld::createCollider(const ColliderDesc& desc)
{
	return Collider(desc);
}

void PhysicsWorld::attachCollider(RigidBodyID body, ColliderID collider)
{
}

void PhysicsWorld::detachCollider(RigidBodyID body, ColliderID collider)
{
}

//bool PhysicsWorld::raycast(const Ray& ray, RaycastHit& hit)
//{
//	return false;
//}
//
//void PhysicsWorld::overlapQuery(const Shape& shape, const Transform& pose, std::vector<BodyID>& results)
//{
//}

void PhysicsWorld::setCollisionCallback(CollisionCallback cb)
{
}

void PhysicsWorld::setTriggerCallback(TriggerCallback cb)
{
}

void PhysicsWorld::detectCollision()
{
	entt::registry& reg = m_Scene->GetRegistry();
	reg.view<RigidBody, Collider, Transform>().each([&](entt::entity entityA, RigidBody& bodyA, Collider& colA, Transform& transA) {
		reg.view<RigidBody, Collider, Transform>().each([&](entt::entity entityB, RigidBody& bodyB, Collider& colB, Transform& transB) {
			if (entityA >= entityB) return; // avoid duplicate checks

			glm::vec3 colA_halfExtents = colA.size * 0.5f;
			glm::vec3 colB_halfExtents = colB.size * 0.5f;

			glm::vec3 minA = transA.position - colA_halfExtents;
			glm::vec3 maxA = transA.position + colA_halfExtents;
			glm::vec3 minB = transB.position - colB_halfExtents;
			glm::vec3 maxB = transB.position + colB_halfExtents;

			bool overlapX = maxA.x >= minB.x && maxB.x >= minA.x;
			bool overlapY = maxA.y >= minB.y && maxB.y >= minA.y;
			bool overlapZ = maxA.z >= minB.z && maxB.z >= minA.z;

			if (overlapX && overlapY && overlapZ)
			{
				// Compute overlap
				glm::vec3 overlap;
				overlap.x = std::min(maxA.x - minB.x, maxB.x - minA.x);
				overlap.y = std::min(maxA.y - minB.y, maxB.y - minA.y);
				overlap.z = std::min(maxA.z - minB.z, maxB.z - minA.z);

				// Prioritize Y-axis stacking
				if (overlap.y < overlap.x && overlap.y < overlap.z)
				{
					float push = overlap.y;

					if (!bodyA.isKinematic && !bodyB.isKinematic)
					{
						bodyA.position.y -= push / 2.0f;
						bodyB.position.y += push / 2.0f;
						bodyA.velocity.y = 0.0f;
						bodyB.velocity.y = 0.0f;
					}
					else if (!bodyA.isKinematic)
					{
						bodyA.position.y -= push;
						bodyA.velocity.y = 0.0f;
					}
					else if (!bodyB.isKinematic)
					{
						bodyB.position.y += push;
						bodyB.velocity.y = 0.0f;
					}
				}
				else
				{
					// Optional: resolve horizontal collisions more gently
					float pushX = overlap.x;
					float pushZ = overlap.z;

					if (!bodyA.isKinematic && !bodyB.isKinematic)
					{
						bodyA.position.x -= pushX / 2.0f;
						bodyB.position.x += pushX / 2.0f;

						bodyA.position.z -= pushZ / 2.0f;
						bodyB.position.z += pushZ / 2.0f;
					}
					else if (!bodyA.isKinematic)
					{
						bodyA.position.x -= pushX;
						bodyA.position.z -= pushZ;
					}
					else if (!bodyB.isKinematic)
					{
						bodyB.position.x += pushX;
						bodyB.position.z += pushZ;
					}
				}

				// Sync transforms
				transA.position = bodyA.position;
				transB.position = bodyB.position;
			}
			});
		});
}

