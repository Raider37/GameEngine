#include "Editor/Panels/ViewportPanel.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "Engine/Scene/Components.h"
#include "Game/Minecraft/VoxelWorld.h"

#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
#include <imgui.h>
#endif

namespace rg::editor
{
namespace
{
ImVec2 WorldToScreen(
    const float worldX,
    const float worldZ,
    const float centerWorldX,
    const float centerWorldZ,
    const float zoom,
    const ImVec2& canvasCenter)
{
    return ImVec2 {
        canvasCenter.x + ((worldX - centerWorldX) * zoom),
        canvasCenter.y - ((worldZ - centerWorldZ) * zoom)};
}
} // namespace

const char* ViewportPanel::Name() const
{
    return "Viewport";
}

void ViewportPanel::Draw(EditorContext& context)
{
#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
    ImGui::Checkbox("Follow Selection", &m_followSelection);
    ImGui::SameLine();
    ImGui::Checkbox("Show Map Overlay", &m_showMapOverlay);
    ImGui::SameLine();
    if (ImGui::Button("Reset View"))
    {
        m_panX = 0.0f;
        m_panZ = 0.0f;
        m_zoomPixelsPerMeter = 16.0f;
    }
    if (m_showMapOverlay)
    {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(170.0f);
        ImGui::DragFloat("Map Width", &m_mapColumnWidth, 1.0f, 240.0f, 720.0f, "%.0f px");
    }
    ImGui::SliderFloat("Map Zoom", &m_zoomPixelsPerMeter, 4.0f, 64.0f, "%.1f px/m");

    const ImVec2 fullSize = ImGui::GetContentRegionAvail();
    if ((fullSize.x < 64.0f) || (fullSize.y < 64.0f))
    {
        ImGui::TextUnformatted("Viewport is too small.");
        return;
    }

    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    float mapWidth = 0.0f;
    if (m_showMapOverlay)
    {
        const float maxMapWidth = std::max(240.0f, fullSize.x - 220.0f);
        m_mapColumnWidth = std::clamp(m_mapColumnWidth, 240.0f, maxMapWidth);
        mapWidth = m_mapColumnWidth;
    }

    float gameWidth = fullSize.x - (m_showMapOverlay ? (mapWidth + spacing) : 0.0f);
    if (gameWidth < 120.0f)
    {
        gameWidth = std::max(120.0f, fullSize.x * 0.55f);
        mapWidth = m_showMapOverlay ? std::max(0.0f, fullSize.x - gameWidth - spacing) : 0.0f;
    }

    ImGui::BeginChild(
        "GameViewportColumn",
        ImVec2(gameWidth, fullSize.y),
        false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground);
    {
        const ImVec2 gamePos = ImGui::GetCursorScreenPos();
        const ImVec2 gameSize = ImGui::GetContentRegionAvail();
        ImGui::InvisibleButton("GameSurface", gameSize);
        (void)gamePos;
    }
    ImGui::EndChild();

    if (!m_showMapOverlay)
    {
        m_draggingSelected = false;
        return;
    }

    ImGui::SameLine();
    ImGui::BeginChild("MapOverlayColumn", ImVec2(0.0f, fullSize.y), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::TextUnformatted("Map Overlay");
    ImGui::Separator();

    const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    if ((canvasSize.x < 16.0f) || (canvasSize.y < 16.0f))
    {
        ImGui::TextUnformatted("Map column is too small.");
        ImGui::EndChild();
        return;
    }

    ImGui::InvisibleButton("WorldViewportCanvas", canvasSize);
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 canvasEnd {canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y};
    const ImVec2 canvasCenter {canvasPos.x + (canvasSize.x * 0.5f), canvasPos.y + (canvasSize.y * 0.5f)};
    draw->AddRectFilled(canvasPos, canvasEnd, IM_COL32(24, 27, 33, 255));

    Entity selected = context.world.FindEntity(context.state.selectedEntity);
    float centerX = m_panX;
    float centerZ = m_panZ;
    if (m_followSelection && selected.IsValid())
    {
        centerX += selected.Transform().position.x;
        centerZ += selected.Transform().position.z;
    }

    const float halfWorldWidth = (canvasSize.x * 0.5f) / m_zoomPixelsPerMeter;
    const float halfWorldHeight = (canvasSize.y * 0.5f) / m_zoomPixelsPerMeter;
    const int minX = static_cast<int>(std::floor(centerX - halfWorldWidth)) - 1;
    const int maxX = static_cast<int>(std::ceil(centerX + halfWorldWidth)) + 1;
    const int minZ = static_cast<int>(std::floor(centerZ - halfWorldHeight)) - 1;
    const int maxZ = static_cast<int>(std::ceil(centerZ + halfWorldHeight)) + 1;

    for (int x = minX; x <= maxX; ++x)
    {
        const ImVec2 a = WorldToScreen(static_cast<float>(x), static_cast<float>(minZ), centerX, centerZ, m_zoomPixelsPerMeter, canvasCenter);
        const ImVec2 b = WorldToScreen(static_cast<float>(x), static_cast<float>(maxZ), centerX, centerZ, m_zoomPixelsPerMeter, canvasCenter);
        const ImU32 color = ((x % 8) == 0) ? IM_COL32(76, 86, 110, 150) : IM_COL32(58, 64, 80, 90);
        draw->AddLine(a, b, color);
    }

    for (int z = minZ; z <= maxZ; ++z)
    {
        const ImVec2 a = WorldToScreen(static_cast<float>(minX), static_cast<float>(z), centerX, centerZ, m_zoomPixelsPerMeter, canvasCenter);
        const ImVec2 b = WorldToScreen(static_cast<float>(maxX), static_cast<float>(z), centerX, centerZ, m_zoomPixelsPerMeter, canvasCenter);
        const ImU32 color = ((z % 8) == 0) ? IM_COL32(76, 86, 110, 150) : IM_COL32(58, 64, 80, 90);
        draw->AddLine(a, b, color);
    }

    if (context.voxelWorld != nullptr)
    {
        const ImVec2 worldMin = WorldToScreen(
            static_cast<float>(context.voxelWorld->MinWorldX()),
            static_cast<float>(context.voxelWorld->MinWorldZ()),
            centerX,
            centerZ,
            m_zoomPixelsPerMeter,
            canvasCenter);
        const ImVec2 worldMax = WorldToScreen(
            static_cast<float>(context.voxelWorld->MaxWorldX()),
            static_cast<float>(context.voxelWorld->MaxWorldZ()),
            centerX,
            centerZ,
            m_zoomPixelsPerMeter,
            canvasCenter);
        draw->AddRect(
            ImVec2 {std::min(worldMin.x, worldMax.x), std::min(worldMin.y, worldMax.y)},
            ImVec2 {std::max(worldMin.x, worldMax.x), std::max(worldMin.y, worldMax.y)},
            IM_COL32(130, 170, 95, 170),
            0.0f,
            0,
            2.0f);
    }

    float nearestDistanceSq = 36.0f;
    ecs::EntityId hoveredEntity = ecs::InvalidEntity;
    for (const Entity entity : context.world.Entities())
    {
        const TransformComponent& transform = entity.Transform();
        const ImVec2 marker = WorldToScreen(
            transform.position.x,
            transform.position.z,
            centerX,
            centerZ,
            m_zoomPixelsPerMeter,
            canvasCenter);

        const bool isSelected = (entity.GetId() == context.state.selectedEntity);
        const float markerRadius = isSelected ? 6.0f : 4.0f;
        ImU32 color = IM_COL32(145, 195, 250, 230);
        if (entity.HasScript())
        {
            color = IM_COL32(255, 208, 92, 240);
        }
        if (isSelected)
        {
            color = IM_COL32(255, 100, 100, 255);
        }

        draw->AddCircleFilled(marker, markerRadius, color, 16);

        if (m_zoomPixelsPerMeter >= 18.0f)
        {
            draw->AddText(ImVec2 {marker.x + 7.0f, marker.y - 8.0f}, IM_COL32(220, 225, 240, 255), entity.GetName().c_str());
        }

        if (hovered)
        {
            const ImVec2 mouse = ImGui::GetIO().MousePos;
            const float dx = marker.x - mouse.x;
            const float dy = marker.y - mouse.y;
            const float distanceSq = (dx * dx) + (dy * dy);
            if (distanceSq <= nearestDistanceSq)
            {
                nearestDistanceSq = distanceSq;
                hoveredEntity = entity.GetId();
            }
        }
    }

    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        context.state.selectedEntity = hoveredEntity;
        m_draggingSelected = (hoveredEntity != ecs::InvalidEntity);
    }

    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        m_draggingSelected = false;
    }

    if (m_draggingSelected)
    {
        Entity dragEntity = context.world.FindEntity(context.state.selectedEntity);
        if (dragEntity.IsValid())
        {
            const ImVec2 mouse = ImGui::GetIO().MousePos;
            const float worldX = centerX + ((mouse.x - canvasCenter.x) / m_zoomPixelsPerMeter);
            const float worldZ = centerZ - ((mouse.y - canvasCenter.y) / m_zoomPixelsPerMeter);
            dragEntity.Transform().position.x = worldX;
            dragEntity.Transform().position.z = worldZ;
        }
    }

    if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
    {
        const ImVec2 delta = ImGui::GetIO().MouseDelta;
        m_panX -= delta.x / m_zoomPixelsPerMeter;
        m_panZ += delta.y / m_zoomPixelsPerMeter;
    }

    if (hovered)
    {
        const float wheel = ImGui::GetIO().MouseWheel;
        if (std::abs(wheel) > 0.001f)
        {
            m_zoomPixelsPerMeter = std::clamp(m_zoomPixelsPerMeter + (wheel * 1.8f), 4.0f, 64.0f);
        }

        if (active && (hoveredEntity != ecs::InvalidEntity))
        {
            Entity hoverEntity = context.world.FindEntity(hoveredEntity);
            if (hoverEntity.IsValid())
            {
                const auto& p = hoverEntity.Transform().position;
                ImGui::SetTooltip(
                    "Entity #%u\n%s\nPos: %.2f %.2f %.2f",
                    hoverEntity.GetId(),
                    hoverEntity.GetName().c_str(),
                    p.x,
                    p.y,
                    p.z);
            }
        }
    }

    ImGui::EndChild();
#else
    (void)context;
#endif
}
} // namespace rg::editor
