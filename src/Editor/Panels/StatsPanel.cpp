#include "Editor/Panels/StatsPanel.h"

#include "Engine/Scene/Components.h"
#include "Game/Minecraft/VoxelWorld.h"

#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
#include <imgui.h>
#endif

namespace rg::editor
{
const char* StatsPanel::Name() const
{
    return "Stats";
}

void StatsPanel::Draw(EditorContext& context)
{
#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
    const float deltaMs = context.stats.deltaSeconds * 1000.0f;
    const float fps = (context.stats.deltaSeconds > 0.00001f) ? (1.0f / context.stats.deltaSeconds) : 0.0f;

    ImGui::Text("Frame: %llu", context.stats.frameIndex);
    ImGui::Text("Renderer: %s", context.stats.rendererBackend);
    ImGui::Text("Frame time: %.3f ms", deltaMs);
    ImGui::Text("FPS: %.1f", fps);
    ImGui::Text("Entities: %zu", context.world.EntityCount());
    ImGui::Checkbox("Show ImGui Demo", &context.state.showDemoWindow);

    if (context.voxelWorld != nullptr)
    {
        ImGui::SeparatorText("Voxel Sandbox");
        ImGui::Text("Chunks: %zu", context.voxelWorld->LoadedChunkCount());
        ImGui::Text("Revision: %llu", context.voxelWorld->Revision());
        ImGui::TextUnformatted("Controls: WASD move, Space jump, Q break, E place, F switch block");

        context.world.ForEach<VoxelPlayerComponent, TransformComponent>(
            [&](Entity entity, const VoxelPlayerComponent& player, const TransformComponent& transform)
            {
                ImGui::Text(
                    "Player #%u Pos(%.2f, %.2f, %.2f) Block=%s",
                    entity.GetId(),
                    transform.position.x,
                    transform.position.y,
                    transform.position.z,
                    minecraft::ToString(static_cast<minecraft::BlockType>((player.selectedBlockId % 5U) + 1U)));
            });
    }
#else
    (void)context;
#endif
}
} // namespace rg::editor
