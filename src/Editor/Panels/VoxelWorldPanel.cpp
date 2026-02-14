#include "Editor/Panels/VoxelWorldPanel.h"

#include <cmath>

#include "Engine/Scene/Components.h"
#include "Game/Minecraft/VoxelWorld.h"

#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
#include <imgui.h>
#endif

namespace rg::editor
{
namespace
{
#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
ImU32 BlockColor(const minecraft::BlockType type)
{
    switch (type)
    {
    case minecraft::BlockType::Air:
        return IM_COL32(30, 33, 41, 255);
    case minecraft::BlockType::Grass:
        return IM_COL32(72, 152, 76, 255);
    case minecraft::BlockType::Dirt:
        return IM_COL32(120, 86, 62, 255);
    case minecraft::BlockType::Stone:
        return IM_COL32(132, 132, 136, 255);
    case minecraft::BlockType::Sand:
        return IM_COL32(205, 194, 128, 255);
    case minecraft::BlockType::Wood:
        return IM_COL32(112, 84, 46, 255);
    case minecraft::BlockType::Leaves:
        return IM_COL32(56, 122, 60, 255);
    default:
        return IM_COL32(255, 0, 255, 255);
    }
}
#endif
} // namespace

const char* VoxelWorldPanel::Name() const
{
    return "Voxel World";
}

void VoxelWorldPanel::Draw(EditorContext& context)
{
#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
    if (context.voxelWorld == nullptr)
    {
        ImGui::TextUnformatted("Voxel world is disabled.");
        return;
    }

    auto* voxelWorld = context.voxelWorld;
    static int sliceY = 20;
    static int radius = 24;
    static bool followPlayer = true;
    static bool surfaceView = true;

    Entity player;
    context.world.ForEach<VoxelPlayerComponent, TransformComponent>([&](Entity entity, VoxelPlayerComponent&, TransformComponent&)
    {
        if (!player.IsValid())
        {
            player = entity;
        }
    });

    int centerX = 0;
    int centerZ = 0;
    if (followPlayer && player.IsValid())
    {
        centerX = static_cast<int>(std::floor(player.Transform().position.x));
        centerZ = static_cast<int>(std::floor(player.Transform().position.z));
    }

    ImGui::Text("Chunks: %zu | Revision: %llu", voxelWorld->LoadedChunkCount(), voxelWorld->Revision());
    ImGui::Text("Seed: %d | Radius: %d", voxelWorld->Seed(), voxelWorld->RadiusInChunks());
    ImGui::Checkbox("Follow Player", &followPlayer);
    ImGui::SameLine();
    ImGui::Checkbox("Surface View", &surfaceView);
    ImGui::SliderInt("Slice Y", &sliceY, 0, minecraft::VoxelWorld::kWorldHeight - 1);
    ImGui::SliderInt("View Radius", &radius, 8, 64);

    const float cellSize = 8.0f;
    const int diameter = (radius * 2) + 1;
    const ImVec2 mapSize {diameter * cellSize, diameter * cellSize};
    const ImVec2 mapStart = ImGui::GetCursorScreenPos();

    ImGui::InvisibleButton("VoxelMap", mapSize);
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    for (int dz = -radius; dz <= radius; ++dz)
    {
        for (int dx = -radius; dx <= radius; ++dx)
        {
            const int worldX = centerX + dx;
            const int worldZ = centerZ + dz;

            int sampledY = sliceY;
            if (surfaceView)
            {
                sampledY = voxelWorld->SurfaceHeight(worldX, worldZ);
            }

            const minecraft::BlockType block = voxelWorld->GetBlock(worldX, sampledY, worldZ);
            const ImU32 color = BlockColor(block);

            const float px = mapStart.x + static_cast<float>(dx + radius) * cellSize;
            const float py = mapStart.y + static_cast<float>(dz + radius) * cellSize;
            const ImVec2 min {px, py};
            const ImVec2 max {px + cellSize, py + cellSize};
            drawList->AddRectFilled(min, max, color);
        }
    }

    drawList->AddRect(
        mapStart,
        ImVec2 {mapStart.x + mapSize.x, mapStart.y + mapSize.y},
        IM_COL32(255, 255, 255, 90));

    const ImVec2 centerCellMin {
        mapStart.x + static_cast<float>(radius) * cellSize,
        mapStart.y + static_cast<float>(radius) * cellSize};
    drawList->AddRect(
        centerCellMin,
        ImVec2 {centerCellMin.x + cellSize, centerCellMin.y + cellSize},
        IM_COL32(255, 64, 64, 255),
        0.0f,
        0,
        2.0f);

    if (ImGui::IsItemHovered())
    {
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const int ix = static_cast<int>((mouse.x - mapStart.x) / cellSize);
        const int iz = static_cast<int>((mouse.y - mapStart.y) / cellSize);
        if ((ix >= 0) && (ix < diameter) && (iz >= 0) && (iz < diameter))
        {
            const int worldX = centerX + (ix - radius);
            const int worldZ = centerZ + (iz - radius);
            int sampledY = sliceY;
            if (surfaceView)
            {
                sampledY = voxelWorld->SurfaceHeight(worldX, worldZ);
            }

            const minecraft::BlockType block = voxelWorld->GetBlock(worldX, sampledY, worldZ);
            ImGui::SetTooltip(
                "X:%d Y:%d Z:%d\nBlock:%s",
                worldX,
                sampledY,
                worldZ,
                minecraft::ToString(block));
        }
    }
#else
    (void)context;
#endif
}
} // namespace rg::editor
