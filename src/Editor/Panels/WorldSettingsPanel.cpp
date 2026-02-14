#include "Editor/Panels/WorldSettingsPanel.h"

#include <algorithm>

#include "Engine/Scene/Components.h"
#include "Game/Minecraft/VoxelWorld.h"

#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
#include <imgui.h>
#endif

namespace rg::editor
{
const char* WorldSettingsPanel::Name() const
{
    return "World Settings";
}

void WorldSettingsPanel::Draw(EditorContext& context)
{
#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
    ImGui::Text("Entities: %zu", context.world.EntityCount());

    std::size_t rigidbodies = 0;
    std::size_t scripts = 0;
    context.world.ForEach<RigidbodyComponent>([&](Entity /*entity*/, const RigidbodyComponent&)
    {
        ++rigidbodies;
    });
    context.world.ForEach<ScriptComponent>([&](Entity /*entity*/, const ScriptComponent& script)
    {
        if (script.enabled)
        {
            ++scripts;
        }
    });

    ImGui::Text("Active Scripts: %zu", scripts);
    ImGui::Text("Rigidbodies: %zu", rigidbodies);
    ImGui::Separator();

    if (context.voxelWorld == nullptr)
    {
        ImGui::TextUnformatted("Voxel world is disabled in EngineConfig.");
        return;
    }

    if (!m_initialized)
    {
        m_voxelRadius = context.voxelWorld->RadiusInChunks();
        m_voxelSeed = context.voxelWorld->Seed();
        m_initialized = true;
    }

    ImGui::SeparatorText("Voxel World");
    ImGui::InputInt("Radius (chunks)", &m_voxelRadius);
    ImGui::InputInt("Seed", &m_voxelSeed);
    m_voxelRadius = std::clamp(m_voxelRadius, 1, 64);

    if (ImGui::Button("Regenerate World"))
    {
        context.voxelWorld->Generate(m_voxelRadius, m_voxelSeed);
    }

    ImGui::SameLine();
    if (ImGui::Button("Reload Current"))
    {
        context.voxelWorld->Generate(context.voxelWorld->RadiusInChunks(), context.voxelWorld->Seed());
        m_voxelRadius = context.voxelWorld->RadiusInChunks();
        m_voxelSeed = context.voxelWorld->Seed();
    }

    ImGui::Separator();
    ImGui::Text("Revision: %llu", context.voxelWorld->Revision());
    ImGui::Text("Loaded Chunks: %zu", context.voxelWorld->LoadedChunkCount());
    ImGui::Text("World X: [%d .. %d]", context.voxelWorld->MinWorldX(), context.voxelWorld->MaxWorldX());
    ImGui::Text("World Z: [%d .. %d]", context.voxelWorld->MinWorldZ(), context.voxelWorld->MaxWorldZ());
#else
    (void)context;
#endif
}
} // namespace rg::editor

