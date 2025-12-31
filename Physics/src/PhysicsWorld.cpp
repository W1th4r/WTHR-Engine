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

    // Spatial hash grid structure
    struct GridCell {
        std::vector<entt::entity> entities;
    };

    // Use a better cell size based on your game scale
    float cellSize = 2.0f;

    // Create spatial hash grid - using unordered_map for sparse grids
    std::unordered_map<uint64_t, GridCell> grid;

    // Hash function to convert grid coordinates to key
    auto cellKey = [](int x, int y, int z) -> uint64_t {
        // Use bit mixing for better distribution
        uint64_t h1 = std::hash<int>{}(x);
        uint64_t h2 = std::hash<int>{}(y);
        uint64_t h3 = std::hash<int>{}(z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
        };

    // Phase 1: Broad phase - build spatial grid
    auto view = reg.view<RigidBody, Collider, Transform>();

    // Reserve space if you know approximate count
    // grid.reserve(view.size_hint() * 8); // Estimate 8 cells per entity

    view.each([&](entt::entity entity, RigidBody& body, Collider& col, Transform& trans) {
        glm::vec3 halfExtents = col.size * 0.5f;
        glm::vec3 min = trans.position - halfExtents;
        glm::vec3 max = trans.position + halfExtents;

        // Calculate grid cells that this AABB touches
        int minX = static_cast<int>(std::floor(min.x / cellSize));
        int maxX = static_cast<int>(std::floor(max.x / cellSize));
        int minY = static_cast<int>(std::floor(min.y / cellSize));
        int maxY = static_cast<int>(std::floor(max.y / cellSize));
        int minZ = static_cast<int>(std::floor(min.z / cellSize));
        int maxZ = static_cast<int>(std::floor(max.z / cellSize));

        // Add entity to all overlapping cells
        for (int x = minX; x <= maxX; ++x) {
            for (int y = minY; y <= maxY; ++y) {
                for (int z = minZ; z <= maxZ; ++z) {
                    uint64_t key = cellKey(x, y, z);
                    grid[key].entities.push_back(entity);
                }
            }
        }
        });

    // Track which pairs we've already processed to avoid duplicates
    std::unordered_set<uint64_t> processedPairs;

    // Phase 2: Narrow phase - check collisions within each cell
    for (auto& [cellKey, cell] : grid) {
        auto& entities = cell.entities;
        size_t count = entities.size();

        // Skip cells with 0 or 1 entities
        if (count < 2) continue;

        // Check all unique pairs in this cell
        for (size_t i = 0; i < count; ++i) {
            entt::entity entityA = entities[i];

            for (size_t j = i + 1; j < count; ++j) {
                entt::entity entityB = entities[j];

                // Create unique pair ID (ordered to avoid duplicates)
                entt::entity smaller = entityA < entityB ? entityA : entityB;
                entt::entity larger = entityA < entityB ? entityB : entityA;

                uint64_t pairId = (static_cast<uint64_t>(smaller) << 32) | static_cast<uint64_t>(larger);

                // Skip if we've already processed this pair
                if (processedPairs.find(pairId) != processedPairs.end()) {
                    continue;
                }

                processedPairs.insert(pairId);

                // Get components
                auto [bodyA, colA, transA] = reg.get<RigidBody, Collider, Transform>(entityA);
                auto [bodyB, colB, transB] = reg.get<RigidBody, Collider, Transform>(entityB);

                // Fast AABB overlap test
                glm::vec3 halfA = colA.size * 0.5f;
                glm::vec3 halfB = colB.size * 0.5f;

                glm::vec3 minA = transA.position - halfA;
                glm::vec3 maxA = transA.position + halfA;
                glm::vec3 minB = transB.position - halfB;
                glm::vec3 maxB = transB.position + halfB;

                // Early out if no overlap
                if (maxA.x < minB.x || minA.x > maxB.x ||
                    maxA.y < minB.y || minA.y > maxB.y ||
                    maxA.z < minB.z || minA.z > maxB.z) {
                    continue;
                }

                // Detailed collision resolution
                resolveCollision(bodyA, colA, transA, bodyB, colB, transB);
            }
        }
    }
}
// Extract collision resolution to separate function
void PhysicsWorld::resolveCollision(RigidBody& bodyA, Collider& colA, Transform& transA,
    RigidBody& bodyB, Collider& colB, Transform& transB)
{
    glm::vec3 colA_halfExtents = colA.size * 0.5f;
    glm::vec3 colB_halfExtents = colB.size * 0.5f;

    glm::vec3 minA = transA.position - colA_halfExtents;
    glm::vec3 maxA = transA.position + colA_halfExtents;
    glm::vec3 minB = transB.position - colB_halfExtents;
    glm::vec3 maxB = transB.position + colB_halfExtents;

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