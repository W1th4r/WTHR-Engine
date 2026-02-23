#pragma once
#include <functional>
#include <cstdint>

using RigidBodyID = uint32_t;
using ColliderID = uint32_t;


using CollisionCallback = std::function<void(RigidBodyID a, RigidBodyID b)>;


using TriggerCallback = std::function<void(RigidBodyID a, ColliderID trigger)>;

