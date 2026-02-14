#pragma once

#include "Editor/EditorState.h"
#include "Engine/Resources/ResourceManager.h"
#include "Engine/Scene/World.h"

namespace rg::minecraft
{
class VoxelWorld;
}

namespace rg::editor
{
struct EditorContext
{
    World& world;
    ResourceManager& resources;
    ::rg::minecraft::VoxelWorld* voxelWorld = nullptr;
    EditorState& state;
    EditorFrameStats stats;
};
} // namespace rg::editor
