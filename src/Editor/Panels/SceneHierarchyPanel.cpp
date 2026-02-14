#include "Editor/Panels/SceneHierarchyPanel.h"

#include <string>

#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
#include <imgui.h>
#endif

namespace rg::editor
{
const char* SceneHierarchyPanel::Name() const
{
    return "Scene Objects";
}

void SceneHierarchyPanel::Draw(EditorContext& context)
{
#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
    if (ImGui::Button("Create Empty"))
    {
        Entity entity = context.world.CreateEntity("Empty Entity");
        context.state.selectedEntity = entity.GetId();
    }

    ImGui::Separator();

    for (const Entity entity : context.world.Entities())
    {
        const bool selected = (context.state.selectedEntity == entity.GetId());
        std::string label = entity.GetName() + "##" + std::to_string(entity.GetId());

        if (ImGui::Selectable(label.c_str(), selected))
        {
            context.state.selectedEntity = entity.GetId();
        }
    }
#else
    (void)context;
#endif
}
} // namespace rg::editor
