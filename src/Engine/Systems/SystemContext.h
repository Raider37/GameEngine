#pragma once

#include <cstdint>

#include "Engine/Input/InputState.h"
#include "Engine/Platform/Window.h"
#include "Engine/Rendering/Renderer.h"
#include "Engine/Resources/ResourceManager.h"
#include "Engine/Scene/World.h"
#include "Engine/Scripting/ScriptHost.h"

namespace rg
{
namespace editor
{
class EditorUI;
}

namespace minecraft
{
class VoxelWorld;
}

struct SystemContext
{
    World& world;
    InputState& input;
    WindowSystem& window;
    Renderer& renderer;
    ResourceManager& resources;
    ScriptHost& scriptHost;
    editor::EditorUI* editorUI = nullptr;
    minecraft::VoxelWorld* voxelWorld = nullptr;
    std::uint64_t frameIndex = 0;
};
} // namespace rg
