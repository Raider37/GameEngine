#pragma once

#include <cstdint>

#include "Engine/ECS/Registry.h"

namespace rg::editor
{
struct EditorState
{
    ecs::EntityId selectedEntity = ecs::InvalidEntity;
    bool showDemoWindow = false;
};

struct EditorFrameStats
{
    float deltaSeconds = 0.0f;
    std::uint64_t frameIndex = 0;
    const char* rendererBackend = "None";
};
} // namespace rg::editor
