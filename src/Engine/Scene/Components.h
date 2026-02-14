#pragma once

#include <cstdint>
#include <string>

#include "Engine/Math/Vector3.h"

namespace rg
{
struct NameComponent
{
    std::string value;
};

struct TransformComponent
{
    Vector3 position {};
    Vector3 rotation {};
    Vector3 scale {1.0f, 1.0f, 1.0f};
};

struct ScriptComponent
{
    std::string className;
    bool enabled = true;
};

struct MeshComponent
{
    std::string meshAsset;
    std::string materialAsset;
    bool visible = true;
};

struct RigidbodyComponent
{
    Vector3 velocity {};
    float mass = 1.0f;
    bool useGravity = true;
    bool isKinematic = false;
};

struct VoxelPlayerComponent
{
    float walkSpeed = 6.0f;
    float jumpSpeed = 7.5f;
    float verticalVelocity = 0.0f;
    float reachDistance = 6.0f;
    std::uint8_t selectedBlockId = 1; // 1 = grass block in voxel module.
    bool grounded = false;
};
} // namespace rg
