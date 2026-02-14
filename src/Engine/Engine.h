#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "Editor/EditorUI.h"
#include "Engine/Input/InputState.h"
#include "Engine/Platform/Window.h"
#include "Engine/Rendering/Renderer.h"
#include "Engine/Rendering/RenderAPI.h"
#include "Engine/Resources/ResourceManager.h"
#include "Engine/Scene/World.h"
#include "Engine/Scripting/ScriptHost.h"
#include "Engine/Systems/ISystem.h"
#include "Game/Minecraft/VoxelWorld.h"

namespace rg
{
struct EngineConfig
{
    std::string projectName = "RaiderEngine";
    WindowSpec window {};
    RenderAPI renderAPI = RenderAPI::DirectX12;
    bool vsync = true;
    bool enableEditorUI = true;
    bool enableVoxelSandbox = true;
    int voxelWorldRadiusInChunks = 8;
    int voxelWorldSeed = 1337;
    std::filesystem::path assetRoot = "assets";
    std::uint32_t maxFrames = 120;
    float fixedDeltaSeconds = 1.0f / 60.0f;
};

class Engine
{
public:
    explicit Engine(EngineConfig config = {});

    bool Initialize();
    void Run();

    [[nodiscard]] World& GetWorld();

private:
    void RegisterSystems();
    void Update();

    EngineConfig m_config;
    std::uint32_t m_currentFrame = 0;
    InputState m_inputState;
    WindowSystem m_windowSystem;
    ResourceManager m_resources;
    World m_world;
    Renderer m_renderer;
    ScriptHost m_scriptHost;
    minecraft::VoxelWorld m_voxelWorld;
    std::unique_ptr<editor::EditorUI> m_editorUI;
    std::vector<std::unique_ptr<ISystem>> m_systems;
};
} // namespace rg
