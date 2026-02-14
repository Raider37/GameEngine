#include "Editor/Panels/LevelClassesPanel.h"

#include <map>
#include <string>

#include "Engine/Scene/Components.h"

#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
#include <imgui.h>
#endif

namespace rg::editor
{
const char* LevelClassesPanel::Name() const
{
    return "Level Classes";
}

void LevelClassesPanel::Draw(EditorContext& context)
{
#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
    std::size_t nameCount = 0;
    std::size_t transformCount = 0;
    std::size_t meshCount = 0;
    std::size_t rigidbodyCount = 0;
    std::size_t voxelPlayerCount = 0;
    std::map<std::string, std::size_t> scriptClasses;

    for (const Entity entity : context.world.Entities())
    {
        if (entity.HasComponent<NameComponent>())
        {
            ++nameCount;
        }
        if (entity.HasComponent<TransformComponent>())
        {
            ++transformCount;
        }
        if (entity.HasComponent<MeshComponent>())
        {
            ++meshCount;
        }
        if (entity.HasComponent<RigidbodyComponent>())
        {
            ++rigidbodyCount;
        }
        if (entity.HasComponent<VoxelPlayerComponent>())
        {
            ++voxelPlayerCount;
        }
        if (entity.HasScript())
        {
            const ScriptComponent& script = entity.Script();
            ++scriptClasses[script.className.empty() ? "<empty>" : script.className];
        }
    }

    ImGui::Text("Entities: %zu", context.world.EntityCount());
    ImGui::SeparatorText("Component Classes");
    ImGui::BulletText("NameComponent: %zu", nameCount);
    ImGui::BulletText("TransformComponent: %zu", transformCount);
    ImGui::BulletText("MeshComponent: %zu", meshCount);
    ImGui::BulletText("RigidbodyComponent: %zu", rigidbodyCount);
    ImGui::BulletText("VoxelPlayerComponent: %zu", voxelPlayerCount);

    ImGui::SeparatorText("Script Classes In Level");
    if (scriptClasses.empty())
    {
        ImGui::TextUnformatted("No script classes on level.");
        return;
    }

    Entity selected = context.world.FindEntity(context.state.selectedEntity);
    for (const auto& [className, count] : scriptClasses)
    {
        ImGui::PushID(className.c_str());
        ImGui::Text("%s (%zu)", className.c_str(), count);
        ImGui::SameLine();
        if (!selected.IsValid())
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("Apply To Selected"))
        {
            if (selected.HasScript())
            {
                selected.Script().className = className;
                selected.Script().enabled = true;
            }
            else
            {
                selected.AttachScript(className);
            }
        }

        if (!selected.IsValid())
        {
            ImGui::EndDisabled();
        }

        ImGui::PopID();
    }
#else
    (void)context;
#endif
}
} // namespace rg::editor

