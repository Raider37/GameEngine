#include "Engine/Systems/VoxelGameplaySystem.h"

#include <algorithm>
#include <cstdint>
#include <cmath>

#include "Engine/Core/Log.h"
#include "Engine/Scene/Components.h"
#include "Game/Minecraft/VoxelWorld.h"

namespace rg
{
namespace
{
minecraft::BlockType SelectionToBlock(const std::uint8_t selection)
{
    switch (selection % 5U)
    {
    case 0:
        return minecraft::BlockType::Grass;
    case 1:
        return minecraft::BlockType::Dirt;
    case 2:
        return minecraft::BlockType::Stone;
    case 3:
        return minecraft::BlockType::Sand;
    case 4:
    default:
        return minecraft::BlockType::Wood;
    }
}

Entity FindVoxelPlayer(World& world)
{
    Entity found;
    world.ForEach<VoxelPlayerComponent, TransformComponent>([&](Entity entity, VoxelPlayerComponent&, TransformComponent&)
    {
        if (!found.IsValid())
        {
            found = entity;
        }
    });
    return found;
}
} // namespace

const char* VoxelGameplaySystem::Name() const
{
    return "VoxelGameplaySystem";
}

bool VoxelGameplaySystem::Initialize(SystemContext& context)
{
    if (context.voxelWorld == nullptr)
    {
        Log::Write(LogLevel::Error, "VoxelGameplaySystem requires VoxelWorld.");
        return false;
    }

    EnsurePlayerEntity(context);
    Log::Write(LogLevel::Info, "Initialized VoxelGameplaySystem.");
    return true;
}

void VoxelGameplaySystem::Update(SystemContext& context, const float deltaSeconds)
{
    if (context.voxelWorld == nullptr)
    {
        return;
    }

    EnsurePlayerEntity(context);
    Entity player = FindVoxelPlayer(context.world);
    if (!player.IsValid())
    {
        return;
    }

    UpdateMovement(context, player, deltaSeconds);
    UpdateBlockInteraction(context, player);
}

void VoxelGameplaySystem::EnsurePlayerEntity(SystemContext& context) const
{
    if (FindVoxelPlayer(context.world).IsValid())
    {
        return;
    }

    Entity player = context.world.CreateEntity("VoxelPlayer");
    player.Transform().position = Vector3 {0.0f, 30.0f, 0.0f};
    player.AddComponent<VoxelPlayerComponent>();
}

void VoxelGameplaySystem::UpdateMovement(SystemContext& context, Entity player, const float deltaSeconds) const
{
    auto& transform = player.Transform();
    auto& controller = player.GetComponent<VoxelPlayerComponent>();
    auto& input = context.input;
    auto& world = *context.voxelWorld;

    float moveX = 0.0f;
    float moveZ = 0.0f;
    if (input.IsDown(KeyCode::W))
    {
        moveZ += 1.0f;
    }
    if (input.IsDown(KeyCode::S))
    {
        moveZ -= 1.0f;
    }
    if (input.IsDown(KeyCode::D))
    {
        moveX += 1.0f;
    }
    if (input.IsDown(KeyCode::A))
    {
        moveX -= 1.0f;
    }

    const float moveLength = std::sqrt((moveX * moveX) + (moveZ * moveZ));
    if (moveLength > 0.001f)
    {
        const float invLength = 1.0f / moveLength;
        moveX *= invLength;
        moveZ *= invLength;
    }

    transform.position.x += moveX * controller.walkSpeed * deltaSeconds;
    transform.position.z += moveZ * controller.walkSpeed * deltaSeconds;

    transform.position.x = std::clamp(
        transform.position.x,
        static_cast<float>(world.MinWorldX() + 1),
        static_cast<float>(world.MaxWorldX() - 1));
    transform.position.z = std::clamp(
        transform.position.z,
        static_cast<float>(world.MinWorldZ() + 1),
        static_cast<float>(world.MaxWorldZ() - 1));

    const int sampleX = static_cast<int>(std::floor(transform.position.x));
    const int sampleZ = static_cast<int>(std::floor(transform.position.z));
    const float groundY = static_cast<float>(world.SurfaceHeight(sampleX, sampleZ)) + 1.0f;

    if (transform.position.y <= groundY)
    {
        transform.position.y = groundY;
        controller.grounded = true;
        controller.verticalVelocity = 0.0f;
    }
    else
    {
        controller.grounded = false;
    }

    if (controller.grounded && input.WasPressed(KeyCode::Space))
    {
        controller.verticalVelocity = controller.jumpSpeed;
        controller.grounded = false;
    }

    if (!controller.grounded)
    {
        controller.verticalVelocity += -20.0f * deltaSeconds;
        transform.position.y += controller.verticalVelocity * deltaSeconds;
    }

    transform.position.y = std::clamp(transform.position.y, 1.0f, static_cast<float>(minecraft::VoxelWorld::kWorldHeight - 1));
}

void VoxelGameplaySystem::UpdateBlockInteraction(SystemContext& context, Entity player) const
{
    auto& transform = player.Transform();
    auto& controller = player.GetComponent<VoxelPlayerComponent>();
    auto& input = context.input;
    auto& world = *context.voxelWorld;

    if (input.WasPressed(KeyCode::F))
    {
        controller.selectedBlockId = static_cast<std::uint8_t>((controller.selectedBlockId + 1U) % 5U);
    }

    const Vector3 origin {transform.position.x, transform.position.y + 1.5f, transform.position.z};
    const Vector3 direction {0.0f, -0.3f, 1.0f};
    minecraft::BlockHit hit;
    if (!world.Raycast(origin, direction, controller.reachDistance, hit))
    {
        return;
    }

    if (input.WasPressed(KeyCode::Q))
    {
        if (hit.y > 0)
        {
            const bool changed = world.SetBlock(hit.x, hit.y, hit.z, minecraft::BlockType::Air);
            (void)changed;
        }
    }

    if (input.WasPressed(KeyCode::E))
    {
        const int placeX = hit.x + hit.normalX;
        const int placeY = hit.y + hit.normalY;
        const int placeZ = hit.z + hit.normalZ;

        const int playerBlockX = static_cast<int>(std::floor(transform.position.x));
        const int playerBlockY = static_cast<int>(std::floor(transform.position.y));
        const int playerBlockZ = static_cast<int>(std::floor(transform.position.z));
        if ((placeX == playerBlockX) && (placeY == playerBlockY) && (placeZ == playerBlockZ))
        {
            return;
        }

        const bool changed = world.SetBlock(placeX, placeY, placeZ, SelectionToBlock(controller.selectedBlockId));
        (void)changed;
    }
}
} // namespace rg
