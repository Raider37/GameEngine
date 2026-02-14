#include "Editor/Panels/AssetsPanel.h"

#include <filesystem>
#include <string>

#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
#include <imgui.h>
#endif

namespace rg::editor
{
const char* AssetsPanel::Name() const
{
    return "Content Browser";
}

void AssetsPanel::Draw(EditorContext& context)
{
#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
    const std::filesystem::path root = context.resources.Root();
    ImGui::Text("Root: %s", root.string().c_str());
    ImGui::Separator();

    if (!std::filesystem::exists(root))
    {
        ImGui::TextUnformatted("Asset root path does not exist.");
        return;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(root))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        const auto relative = std::filesystem::relative(entry.path(), root);
        ImGui::BulletText("%s", relative.string().c_str());
    }
#else
    (void)context;
#endif
}
} // namespace rg::editor
