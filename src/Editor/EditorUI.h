#pragma once

#include <filesystem>
#include <string>

#include "Editor/EditorContext.h"
#include "Editor/Panels/PanelRegistry.h"

namespace rg
{
class Renderer;
}

namespace rg::minecraft
{
class VoxelWorld;
}

namespace rg::editor
{
class EditorUI
{
public:
    enum class PlayMode
    {
        Stopped = 0,
        Playing,
        Paused
    };

    bool Initialize(World& world, ResourceManager& resources, Renderer& renderer, minecraft::VoxelWorld* voxelWorld);
    void SetFrameStats(EditorFrameStats stats);
    void Render();
    [[nodiscard]] bool ConsumeCloseRequest();

    [[nodiscard]] bool IsEnabled() const;

private:
    void DrawMainMenu(EditorContext& context);
    void DrawFileMenu(EditorContext& context);
    void DrawEditMenu(EditorContext& context);
    void DrawLevelMenu(EditorContext& context);
    void DrawDisplayMenu(EditorContext& context);
    void DrawGameMenu();
    void DrawDebugMenu(EditorContext& context);

    void NewLevel(EditorContext& context);
    void SaveLevel(const World& world, const std::filesystem::path& path);
    void LoadLevel(World& world, EditorState& state, const std::filesystem::path& path);
    [[nodiscard]] Entity DuplicateEntity(EditorContext& context, Entity source);

    bool m_enabled = false;
    World* m_world = nullptr;
    ResourceManager* m_resources = nullptr;
    Renderer* m_renderer = nullptr;
    minecraft::VoxelWorld* m_voxelWorld = nullptr;
    EditorState m_state;
    EditorFrameStats m_stats {};
    PanelRegistry m_panelRegistry;
    bool m_showMetricsWindow = false;
    bool m_showStyleEditor = false;
    bool m_requestClose = false;
    PlayMode m_playMode = PlayMode::Stopped;
    std::filesystem::path m_levelFilePath = "build/editor_level.rglvl";
    std::string m_lastActionMessage = "Ready.";
};
} // namespace rg::editor
