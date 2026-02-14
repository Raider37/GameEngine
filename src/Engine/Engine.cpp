#include "Engine/Engine.h"

#include <chrono>
#include <string>
#include <thread>
#include <utility>

#include "Engine/Core/Log.h"
#include "Engine/Systems/PhysicsSystem.h"
#include "Engine/Systems/RenderSystem.h"
#include "Engine/Systems/ScriptSystem.h"
#include "Engine/Systems/SystemContext.h"
#include "Engine/Systems/VoxelGameplaySystem.h"

namespace rg
{
Engine::Engine(EngineConfig config) : m_config(std::move(config))
{
}

bool Engine::Initialize()
{
    Log::Write(LogLevel::Info, "Initializing project: " + m_config.projectName);

    m_resources.SetRoot(m_config.assetRoot);
    if (!m_windowSystem.Initialize(m_config.window))
    {
        Log::Write(LogLevel::Error, "Window system initialization failed.");
        return false;
    }

    RenderBackendContext renderContext;
    renderContext.nativeWindowHandle = m_windowSystem.NativeHandle();
    renderContext.width = m_windowSystem.Width();
    renderContext.height = m_windowSystem.Height();

    if (!m_renderer.Initialize(RendererConfig {m_config.renderAPI, m_config.vsync}, m_resources, renderContext))
    {
        Log::Write(LogLevel::Error, "Renderer initialization failed.");
        return false;
    }

    m_scriptHost.Initialize();

    if (m_config.enableVoxelSandbox)
    {
        m_voxelWorld.Generate(m_config.voxelWorldRadiusInChunks, m_config.voxelWorldSeed);
        Log::Write(
            LogLevel::Info,
            "Voxel world generated: radius=" + std::to_string(m_config.voxelWorldRadiusInChunks) +
                " chunks, seed=" + std::to_string(m_config.voxelWorldSeed));
    }

    if (m_config.enableEditorUI)
    {
        m_editorUI = std::make_unique<editor::EditorUI>();
        if (!m_editorUI->Initialize(
                m_world,
                m_resources,
                m_renderer,
                m_config.enableVoxelSandbox ? &m_voxelWorld : nullptr))
        {
            Log::Write(LogLevel::Warning, "Editor UI failed to initialize and will be disabled.");
            m_editorUI.reset();
        }
    }

    RegisterSystems();
    SystemContext systemContext {
        m_world,
        m_inputState,
        m_windowSystem,
        m_renderer,
        m_resources,
        m_scriptHost,
        m_editorUI.get(),
        m_config.enableVoxelSandbox ? &m_voxelWorld : nullptr,
        m_currentFrame + 1U};

    for (auto& system : m_systems)
    {
        if (!system->Initialize(systemContext))
        {
            Log::Write(LogLevel::Error, std::string("System initialization failed: ") + system->Name());
            return false;
        }
    }

    return true;
}

void Engine::Run()
{
    Log::Write(LogLevel::Info, "Starting game loop with backend: " + std::string(m_renderer.BackendName()));

    while ((m_currentFrame < m_config.maxFrames) && !m_windowSystem.ShouldClose())
    {
        m_inputState.BeginFrame();
        m_windowSystem.PollEvents(m_inputState);

        if (m_inputState.WasPressed(KeyCode::Escape))
        {
            m_windowSystem.RequestClose();
        }

        Update();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    Log::Write(LogLevel::Info, "Game loop completed.");
}

World& Engine::GetWorld()
{
    return m_world;
}

void Engine::Update()
{
    SystemContext systemContext {
        m_world,
        m_inputState,
        m_windowSystem,
        m_renderer,
        m_resources,
        m_scriptHost,
        m_editorUI.get(),
        m_config.enableVoxelSandbox ? &m_voxelWorld : nullptr,
        m_currentFrame + 1U};

    for (auto& system : m_systems)
    {
        system->Update(systemContext, m_config.fixedDeltaSeconds);
    }

    if ((m_editorUI != nullptr) && m_editorUI->ConsumeCloseRequest())
    {
        m_windowSystem.RequestClose();
    }

    ++m_currentFrame;
}

void Engine::RegisterSystems()
{
    m_systems.clear();
    if (m_config.enableVoxelSandbox)
    {
        m_systems.emplace_back(std::make_unique<VoxelGameplaySystem>());
    }
    m_systems.emplace_back(std::make_unique<ScriptSystem>());
    m_systems.emplace_back(std::make_unique<PhysicsSystem>());
    m_systems.emplace_back(std::make_unique<RenderSystem>());
}
} // namespace rg
