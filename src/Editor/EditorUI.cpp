#include "Editor/EditorUI.h"

#include <fstream>
#include <iomanip>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Editor/Panels/AssetsPanel.h"
#include "Editor/Panels/BuildPanel.h"
#include "Editor/Panels/InspectorPanel.h"
#include "Editor/Panels/LevelClassesPanel.h"
#include "Editor/Panels/SceneHierarchyPanel.h"
#include "Editor/Panels/StatsPanel.h"
#include "Editor/Panels/ViewportPanel.h"
#include "Editor/Panels/VoxelWorldPanel.h"
#include "Editor/Panels/WorldSettingsPanel.h"
#include "Engine/Core/Log.h"
#include "Engine/Rendering/Renderer.h"
#include "Engine/Scene/Components.h"
#include "Game/Minecraft/VoxelWorld.h"

#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
#include <imgui.h>
#endif

namespace rg::editor
{
namespace
{
const char* ToString(const EditorUI::PlayMode mode)
{
    switch (mode)
    {
    case EditorUI::PlayMode::Stopped:
        return "Stopped";
    case EditorUI::PlayMode::Playing:
        return "Playing";
    case EditorUI::PlayMode::Paused:
        return "Paused";
    default:
        return "Unknown";
    }
}
} // namespace

bool EditorUI::Initialize(
    World& world,
    ResourceManager& resources,
    Renderer& renderer,
    minecraft::VoxelWorld* voxelWorld)
{
    m_world = &world;
    m_resources = &resources;
    m_renderer = &renderer;
    m_voxelWorld = voxelWorld;

#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
    m_panelRegistry.Add(std::make_unique<SceneHierarchyPanel>(), true, PanelDockSlot::Right);
    m_panelRegistry.Add(std::make_unique<InspectorPanel>(), true, PanelDockSlot::Right);
    m_panelRegistry.Add(std::make_unique<ViewportPanel>(), true, PanelDockSlot::Center);
    m_panelRegistry.Add(std::make_unique<WorldSettingsPanel>(), true, PanelDockSlot::Right);
    m_panelRegistry.Add(std::make_unique<LevelClassesPanel>(), true, PanelDockSlot::Right);
    m_panelRegistry.Add(std::make_unique<AssetsPanel>(), true, PanelDockSlot::Bottom);
    m_panelRegistry.Add(std::make_unique<StatsPanel>(), true, PanelDockSlot::Bottom);
    m_panelRegistry.Add(std::make_unique<VoxelWorldPanel>(), true, PanelDockSlot::Bottom);
    m_panelRegistry.Add(std::make_unique<BuildPanel>(), true, PanelDockSlot::Bottom);
    m_enabled = true;
    Log::Write(LogLevel::Info, "Editor UI initialized.");
    return true;
#else
    m_enabled = false;
    Log::Write(LogLevel::Warning, "Editor UI is disabled because RG_WITH_IMGUI is not enabled.");
    return false;
#endif
}

void EditorUI::SetFrameStats(EditorFrameStats stats)
{
    m_stats = std::move(stats);
}

bool EditorUI::ConsumeCloseRequest()
{
    const bool requested = m_requestClose;
    m_requestClose = false;
    return requested;
}

void EditorUI::Render()
{
#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
    if (!m_enabled || (m_world == nullptr) || (m_resources == nullptr))
    {
        return;
    }

    EditorContext context {*m_world, *m_resources, m_voxelWorld, m_state, m_stats};

    if (ImGui::BeginMainMenuBar())
    {
        DrawMainMenu(context);
        ImGui::EndMainMenuBar();
    }

    m_panelRegistry.DrawPanels(context);

    if (m_showMetricsWindow)
    {
        ImGui::ShowMetricsWindow(&m_showMetricsWindow);
    }

    if (m_showStyleEditor)
    {
        ImGui::Begin("Style Editor", &m_showStyleEditor);
        ImGui::ShowStyleEditor();
        ImGui::End();
    }

    ImGui::SetNextWindowBgAlpha(0.75f);
    if (ImGui::Begin("##EditorStatusOverlay", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Mode: %s | %s", ToString(m_playMode), m_lastActionMessage.c_str());
    }
    ImGui::End();

    if (m_state.showDemoWindow)
    {
        ImGui::ShowDemoWindow(&m_state.showDemoWindow);
    }
#endif
}

bool EditorUI::IsEnabled() const
{
    return m_enabled;
}

void EditorUI::DrawMainMenu(EditorContext& context)
{
#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
    if (ImGui::BeginMenu("File"))
    {
        DrawFileMenu(context);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit"))
    {
        DrawEditMenu(context);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Level"))
    {
        DrawLevelMenu(context);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Display"))
    {
        DrawDisplayMenu(context);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Game"))
    {
        DrawGameMenu();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Tools"))
    {
        if (ImGui::MenuItem("Quick Save Level"))
        {
            SaveLevel(context.world, m_levelFilePath);
        }

        if (ImGui::MenuItem("Clear Asset Cache"))
        {
            context.resources.ClearCache();
            m_lastActionMessage = "Asset cache cleared from Tools.";
        }

        if (ImGui::MenuItem("Open ImGui Style Editor", nullptr, m_showStyleEditor))
        {
            m_showStyleEditor = true;
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Debug"))
    {
        DrawDebugMenu(context);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help"))
    {
        ImGui::TextUnformatted("RaiderEngine Editor");
        ImGui::Separator();
        ImGui::TextUnformatted("Use Panels menu for windows.");
        ImGui::TextUnformatted("WASD/Space/Q/E/F for voxel controls.");
        ImGui::EndMenu();
    }
#else
    (void)context;
#endif
}

void EditorUI::DrawFileMenu(EditorContext& context)
{
#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
    if (ImGui::MenuItem("New Level", "Ctrl+N"))
    {
        NewLevel(context);
    }

    if (ImGui::MenuItem("Save Level", "Ctrl+S"))
    {
        SaveLevel(context.world, m_levelFilePath);
    }

    if (ImGui::MenuItem("Load Level", "Ctrl+O"))
    {
        LoadLevel(context.world, context.state, m_levelFilePath);
    }

    ImGui::Separator();
    ImGui::Text("Level File: %s", m_levelFilePath.string().c_str());

    if (ImGui::MenuItem("Reload Asset Cache"))
    {
        context.resources.ClearCache();
        m_lastActionMessage = "Asset cache cleared.";
    }

    ImGui::Separator();
    if (ImGui::MenuItem("Exit", "Alt+F4"))
    {
        m_requestClose = true;
        m_lastActionMessage = "Close requested from File menu.";
    }
#else
    (void)context;
#endif
}

void EditorUI::DrawEditMenu(EditorContext& context)
{
#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
    const Entity selected = context.world.FindEntity(context.state.selectedEntity);

    ImGui::BeginDisabled();
    ImGui::MenuItem("Undo", "Ctrl+Z", false, false);
    ImGui::MenuItem("Redo", "Ctrl+Y", false, false);
    ImGui::EndDisabled();

    ImGui::Separator();

    if (!selected.IsValid())
    {
        ImGui::BeginDisabled();
    }

    if (ImGui::MenuItem("Duplicate Selected", "Ctrl+D"))
    {
        Entity duplicated = DuplicateEntity(context, selected);
        if (duplicated.IsValid())
        {
            context.state.selectedEntity = duplicated.GetId();
            m_lastActionMessage = "Duplicated selected entity.";
        }
    }

    if (ImGui::MenuItem("Delete Selected", "Del"))
    {
        context.world.DestroyEntity(selected);
        context.state.selectedEntity = ecs::InvalidEntity;
        m_lastActionMessage = "Deleted selected entity.";
    }

    if (!selected.IsValid())
    {
        ImGui::EndDisabled();
    }

    if (ImGui::MenuItem("Deselect"))
    {
        context.state.selectedEntity = ecs::InvalidEntity;
        m_lastActionMessage = "Selection cleared.";
    }
#else
    (void)context;
#endif
}

void EditorUI::DrawLevelMenu(EditorContext& context)
{
#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
    if (ImGui::MenuItem("Create Empty Entity"))
    {
        Entity entity = context.world.CreateEntity("Empty Entity");
        context.state.selectedEntity = entity.GetId();
        m_lastActionMessage = "Created empty entity.";
    }

    if (ImGui::MenuItem("Create Scripted Mover"))
    {
        Entity entity = context.world.CreateEntity("Scripted Mover");
        entity.AttachScript("MoveForwardScript");
        context.state.selectedEntity = entity.GetId();
        m_lastActionMessage = "Created scripted mover entity.";
    }

    if (ImGui::MenuItem("Create Rigidbody"))
    {
        Entity entity = context.world.CreateEntity("Dynamic Body");
        entity.AddComponent<RigidbodyComponent>();
        context.state.selectedEntity = entity.GetId();
        m_lastActionMessage = "Created rigidbody entity.";
    }

    if (context.voxelWorld != nullptr)
    {
        ImGui::Separator();
        if (ImGui::MenuItem("Regenerate Voxel World"))
        {
            context.voxelWorld->Generate(context.voxelWorld->RadiusInChunks(), context.voxelWorld->Seed());
            m_lastActionMessage = "Voxel world regenerated.";
        }
    }
#else
    (void)context;
#endif
}

void EditorUI::DrawDisplayMenu(EditorContext& context)
{
    (void)context;
#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
    ImGui::MenuItem("ImGui Demo Window", nullptr, &m_state.showDemoWindow);
    ImGui::MenuItem("ImGui Metrics", nullptr, &m_showMetricsWindow);
    ImGui::MenuItem("Style Editor", nullptr, &m_showStyleEditor);
    if (ImGui::MenuItem("Reset Editor Layout"))
    {
        m_panelRegistry.ResetLayout();
        m_lastActionMessage = "Editor layout reset.";
    }
    ImGui::Separator();
    m_panelRegistry.DrawMenu("Panels");
#endif
}

void EditorUI::DrawGameMenu()
{
#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
    bool play = (m_playMode == PlayMode::Playing);
    bool pause = (m_playMode == PlayMode::Paused);
    bool stop = (m_playMode == PlayMode::Stopped);

    if (ImGui::MenuItem("Play", "F5", play))
    {
        m_playMode = PlayMode::Playing;
        m_lastActionMessage = "Game mode: Playing.";
    }

    if (ImGui::MenuItem("Pause", "F6", pause))
    {
        m_playMode = PlayMode::Paused;
        m_lastActionMessage = "Game mode: Paused.";
    }

    if (ImGui::MenuItem("Stop", "F7", stop))
    {
        m_playMode = PlayMode::Stopped;
        m_lastActionMessage = "Game mode: Stopped.";
    }

    ImGui::Separator();
    ImGui::Text("Current: %s", ToString(m_playMode));
#endif
}

void EditorUI::DrawDebugMenu(EditorContext& context)
{
#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
    if (ImGui::MenuItem("Log Scene Summary"))
    {
        std::size_t scripts = 0;
        std::size_t rigidbodies = 0;
        std::size_t meshes = 0;

        context.world.ForEach<ScriptComponent>([&](Entity /*entity*/, const ScriptComponent&)
        {
            ++scripts;
        });
        context.world.ForEach<RigidbodyComponent>([&](Entity /*entity*/, const RigidbodyComponent&)
        {
            ++rigidbodies;
        });
        context.world.ForEach<MeshComponent>([&](Entity /*entity*/, const MeshComponent&)
        {
            ++meshes;
        });

        Log::Write(
            LogLevel::Info,
            "Scene summary: entities=" + std::to_string(context.world.EntityCount()) +
                ", scripts=" + std::to_string(scripts) +
                ", rigidbodies=" + std::to_string(rigidbodies) +
                ", meshes=" + std::to_string(meshes));
        m_lastActionMessage = "Scene summary logged.";
    }

    if (ImGui::MenuItem("Spawn 16 Test Entities"))
    {
        for (int z = 0; z < 4; ++z)
        {
            for (int x = 0; x < 4; ++x)
            {
                Entity entity = context.world.CreateEntity("DebugEntity");
                entity.Transform().position = Vector3 {static_cast<float>(x * 2), 0.0f, static_cast<float>(z * 2)};
            }
        }
        m_lastActionMessage = "Spawned 16 test entities.";
    }

    if (ImGui::MenuItem("Reload Default Shaders"))
    {
        context.resources.ClearCache();
        const bool hlsl = context.resources.Exists("shaders/default.hlsl");
        const bool glsl = context.resources.Exists("shaders/default.vert.glsl");
        Log::Write(
            LogLevel::Info,
            "Shader check: default.hlsl=" + std::string(hlsl ? "ok" : "missing") +
                ", default.vert.glsl=" + std::string(glsl ? "ok" : "missing"));
        m_lastActionMessage = "Shader assets checked. See log.";
    }
#else
    (void)context;
#endif
}

void EditorUI::NewLevel(EditorContext& context)
{
    const std::vector<Entity> entities = context.world.Entities();
    for (const Entity entity : entities)
    {
        context.world.DestroyEntity(entity);
    }
    context.state.selectedEntity = ecs::InvalidEntity;
    m_lastActionMessage = "Created new empty level.";
}

void EditorUI::SaveLevel(const World& world, const std::filesystem::path& path)
{
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);

    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open())
    {
        Log::Write(LogLevel::Error, "Failed to save level file: " + path.string());
        m_lastActionMessage = "Failed to save level.";
        return;
    }

    file << "RAIDER_LEVEL_V1\n";
    for (const Entity entity : world.Entities())
    {
        const TransformComponent& transform = entity.Transform();

        file << "ENTITY\n";
        file << "NAME " << std::quoted(entity.GetName()) << '\n';
        file << "TRANSFORM "
             << transform.position.x << ' ' << transform.position.y << ' ' << transform.position.z << ' '
             << transform.rotation.x << ' ' << transform.rotation.y << ' ' << transform.rotation.z << ' '
             << transform.scale.x << ' ' << transform.scale.y << ' ' << transform.scale.z << '\n';

        if (entity.HasScript())
        {
            const ScriptComponent& script = entity.Script();
            file << "SCRIPT " << script.enabled << ' ' << std::quoted(script.className) << '\n';
        }

        if (entity.HasMesh())
        {
            const MeshComponent& mesh = entity.Mesh();
            file << "MESH " << mesh.visible << ' ' << std::quoted(mesh.meshAsset) << ' ' << std::quoted(mesh.materialAsset) << '\n';
        }

        if (entity.HasComponent<RigidbodyComponent>())
        {
            const RigidbodyComponent& body = entity.GetComponent<RigidbodyComponent>();
            file << "RIGIDBODY "
                 << body.mass << ' ' << body.useGravity << ' ' << body.isKinematic << ' '
                 << body.velocity.x << ' ' << body.velocity.y << ' ' << body.velocity.z << '\n';
        }

        if (entity.HasComponent<VoxelPlayerComponent>())
        {
            const VoxelPlayerComponent& player = entity.GetComponent<VoxelPlayerComponent>();
            file << "VOXEL_PLAYER "
                 << player.walkSpeed << ' ' << player.jumpSpeed << ' ' << player.verticalVelocity << ' '
                 << player.reachDistance << ' ' << static_cast<int>(player.selectedBlockId) << ' ' << player.grounded << '\n';
        }

        file << "END\n";
    }

    m_lastActionMessage = "Level saved: " + path.string();
}

void EditorUI::LoadLevel(World& world, EditorState& state, const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        Log::Write(LogLevel::Error, "Failed to load level file: " + path.string());
        m_lastActionMessage = "Failed to load level.";
        return;
    }

    std::string header;
    file >> header;
    if (header != "RAIDER_LEVEL_V1")
    {
        Log::Write(LogLevel::Error, "Invalid level format: " + path.string());
        m_lastActionMessage = "Invalid level format.";
        return;
    }

    struct EntityData
    {
        std::string name = "Entity";
        TransformComponent transform {};
        bool hasScript = false;
        ScriptComponent script {};
        bool hasMesh = false;
        MeshComponent mesh {};
        bool hasRigidbody = false;
        RigidbodyComponent rigidbody {};
        bool hasVoxelPlayer = false;
        VoxelPlayerComponent voxelPlayer {};
    };

    EditorContext clearContext {world, *m_resources, m_voxelWorld, state, m_stats};
    NewLevel(clearContext);

    std::string token;
    while (file >> token)
    {
        if (token != "ENTITY")
        {
            continue;
        }

        EntityData data;
        while (file >> token)
        {
            if (token == "END")
            {
                break;
            }
            if (token == "NAME")
            {
                file >> std::quoted(data.name);
            }
            else if (token == "TRANSFORM")
            {
                file >> data.transform.position.x >> data.transform.position.y >> data.transform.position.z;
                file >> data.transform.rotation.x >> data.transform.rotation.y >> data.transform.rotation.z;
                file >> data.transform.scale.x >> data.transform.scale.y >> data.transform.scale.z;
            }
            else if (token == "SCRIPT")
            {
                data.hasScript = true;
                file >> data.script.enabled >> std::quoted(data.script.className);
            }
            else if (token == "MESH")
            {
                data.hasMesh = true;
                file >> data.mesh.visible >> std::quoted(data.mesh.meshAsset) >> std::quoted(data.mesh.materialAsset);
            }
            else if (token == "RIGIDBODY")
            {
                data.hasRigidbody = true;
                file >> data.rigidbody.mass >> data.rigidbody.useGravity >> data.rigidbody.isKinematic;
                file >> data.rigidbody.velocity.x >> data.rigidbody.velocity.y >> data.rigidbody.velocity.z;
            }
            else if (token == "VOXEL_PLAYER")
            {
                data.hasVoxelPlayer = true;
                int selected = 1;
                file >> data.voxelPlayer.walkSpeed >> data.voxelPlayer.jumpSpeed >> data.voxelPlayer.verticalVelocity;
                file >> data.voxelPlayer.reachDistance >> selected >> data.voxelPlayer.grounded;
                data.voxelPlayer.selectedBlockId = static_cast<std::uint8_t>(selected);
            }
        }

        Entity entity = world.CreateEntity(data.name);
        entity.Transform() = data.transform;

        if (data.hasScript)
        {
            entity.AttachScript(data.script.className);
            entity.Script().enabled = data.script.enabled;
        }

        if (data.hasMesh)
        {
            entity.AttachMesh(data.mesh.meshAsset, data.mesh.materialAsset);
            entity.Mesh().visible = data.mesh.visible;
        }

        if (data.hasRigidbody)
        {
            entity.AddComponent<RigidbodyComponent>(data.rigidbody);
        }

        if (data.hasVoxelPlayer)
        {
            entity.AddComponent<VoxelPlayerComponent>(data.voxelPlayer);
        }
    }

    state.selectedEntity = ecs::InvalidEntity;
    m_lastActionMessage = "Level loaded: " + path.string();
}

Entity EditorUI::DuplicateEntity(EditorContext& context, const Entity source)
{
    if (!source.IsValid())
    {
        return {};
    }

    Entity duplicated = context.world.CreateEntity(source.GetName() + " Copy");
    duplicated.Transform() = source.Transform();
    duplicated.Transform().position.x += 1.0f;

    if (source.HasScript())
    {
        duplicated.AttachScript(source.Script().className);
        duplicated.Script().enabled = source.Script().enabled;
    }

    if (source.HasMesh())
    {
        duplicated.AttachMesh(source.Mesh().meshAsset, source.Mesh().materialAsset);
        duplicated.Mesh().visible = source.Mesh().visible;
    }

    if (source.HasComponent<RigidbodyComponent>())
    {
        duplicated.AddComponent<RigidbodyComponent>(source.GetComponent<RigidbodyComponent>());
    }

    if (source.HasComponent<VoxelPlayerComponent>())
    {
        duplicated.AddComponent<VoxelPlayerComponent>(source.GetComponent<VoxelPlayerComponent>());
    }

    return duplicated;
}
} // namespace rg::editor
