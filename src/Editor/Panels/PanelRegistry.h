#pragma once

#include <memory>
#include <vector>

#include "Editor/Panels/IEditorPanel.h"

struct ImVec2;

namespace rg::editor
{
enum class PanelDockSlot
{
    Left = 0,
    Center,
    Right,
    Bottom,
    Floating
};

class PanelRegistry
{
public:
    void Add(
        std::unique_ptr<IEditorPanel> panel,
        bool openByDefault = true,
        PanelDockSlot slot = PanelDockSlot::Floating);
    void DrawMenu(const char* menuLabel = "Panels");
    void DrawPanels(EditorContext& context);
    void ResetLayout();

private:
    struct PanelEntry;

    void DrawDockRegion(
        const char* windowId,
        const char* title,
        const ImVec2& position,
        const ImVec2& size,
        PanelDockSlot slot,
        EditorContext& context);
    void DrawDockContextMenu(PanelEntry& panel);
    [[nodiscard]] bool HasOpenPanels(PanelDockSlot slot) const;

    struct PanelEntry
    {
        std::unique_ptr<IEditorPanel> panel;
        bool open = true;
        PanelDockSlot slot = PanelDockSlot::Floating;
        PanelDockSlot defaultSlot = PanelDockSlot::Floating;
        bool lockSlot = false;
        bool lockOpen = false;
    };

    std::vector<PanelEntry> m_panels;
};
} // namespace rg::editor
