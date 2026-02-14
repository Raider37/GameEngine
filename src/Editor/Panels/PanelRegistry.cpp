#include "Editor/Panels/PanelRegistry.h"

#include <algorithm>
#include <string>

#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
#include <imgui.h>
#endif

namespace rg::editor
{
namespace
{
const char* SlotLabel(const PanelDockSlot slot)
{
    switch (slot)
    {
    case PanelDockSlot::Left:
        return "Left";
    case PanelDockSlot::Center:
        return "Center";
    case PanelDockSlot::Right:
        return "Right";
    case PanelDockSlot::Bottom:
        return "Bottom";
    case PanelDockSlot::Floating:
    default:
        return "Floating";
    }
}
} // namespace

void PanelRegistry::Add(
    std::unique_ptr<IEditorPanel> panel,
    const bool openByDefault,
    const PanelDockSlot slot)
{
    if (panel == nullptr)
    {
        return;
    }

    const std::string panelName = panel->Name();

    PanelEntry entry;
    entry.panel = std::move(panel);
    entry.open = openByDefault;
    entry.slot = slot;
    entry.defaultSlot = slot;

    if (panelName == "Viewport")
    {
        entry.slot = PanelDockSlot::Center;
        entry.defaultSlot = PanelDockSlot::Center;
        entry.open = true;
        entry.lockSlot = true;
        entry.lockOpen = true;
    }

    m_panels.push_back(std::move(entry));
}

void PanelRegistry::DrawMenu(const char* menuLabel)
{
#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
    if (!ImGui::BeginMenu((menuLabel != nullptr) ? menuLabel : "Panels"))
    {
        return;
    }

    for (auto& panel : m_panels)
    {
        if (ImGui::BeginMenu(panel.panel->Name()))
        {
            if (panel.lockOpen)
            {
                bool visible = true;
                ImGui::BeginDisabled();
                ImGui::MenuItem("Visible (Locked)", nullptr, &visible);
                ImGui::EndDisabled();
            }
            else
            {
                ImGui::MenuItem("Visible", nullptr, &panel.open);
            }
            ImGui::Separator();

            if (panel.lockSlot)
            {
                ImGui::BeginDisabled();
            }
            if (ImGui::MenuItem("Dock Left", nullptr, panel.slot == PanelDockSlot::Left))
            {
                panel.slot = PanelDockSlot::Left;
            }
            if (ImGui::MenuItem("Dock Center", nullptr, panel.slot == PanelDockSlot::Center))
            {
                panel.slot = PanelDockSlot::Center;
            }
            if (ImGui::MenuItem("Dock Right", nullptr, panel.slot == PanelDockSlot::Right))
            {
                panel.slot = PanelDockSlot::Right;
            }
            if (ImGui::MenuItem("Dock Bottom", nullptr, panel.slot == PanelDockSlot::Bottom))
            {
                panel.slot = PanelDockSlot::Bottom;
            }
            if (ImGui::MenuItem("Floating", nullptr, panel.slot == PanelDockSlot::Floating))
            {
                panel.slot = PanelDockSlot::Floating;
            }
            if (panel.lockSlot)
            {
                ImGui::EndDisabled();
                panel.slot = PanelDockSlot::Center;
            }

            ImGui::Separator();
            if (panel.lockSlot)
            {
                ImGui::BeginDisabled();
            }
            if (ImGui::MenuItem("Reset To Default Slot"))
            {
                panel.slot = panel.defaultSlot;
                if (panel.lockOpen)
                {
                    panel.open = true;
                }
            }
            if (panel.lockSlot)
            {
                ImGui::EndDisabled();
            }

            ImGui::TextDisabled("Current: %s", SlotLabel(panel.slot));
            ImGui::EndMenu();
        }
    }

    ImGui::Separator();
    if (ImGui::MenuItem("Reset All Panel Slots"))
    {
        ResetLayout();
    }

    ImGui::EndMenu();
#endif
}

void PanelRegistry::DrawPanels(EditorContext& context)
{
#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
    for (auto& panel : m_panels)
    {
        if (panel.lockSlot)
        {
            panel.slot = PanelDockSlot::Center;
        }
        if (panel.lockOpen)
        {
            panel.open = true;
        }
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 workPos = viewport->WorkPos;
    const ImVec2 workSize = viewport->WorkSize;

    const bool hasLeft = HasOpenPanels(PanelDockSlot::Left);
    const bool hasRight = HasOpenPanels(PanelDockSlot::Right);
    const bool hasBottom = HasOpenPanels(PanelDockSlot::Bottom);
    const bool hasCenter = HasOpenPanels(PanelDockSlot::Center);

    const float leftWidth = hasLeft ? std::clamp(workSize.x * 0.16f, 220.0f, 380.0f) : 0.0f;
    const float rightWidth = hasRight ? std::clamp(workSize.x * 0.28f, 300.0f, 520.0f) : 0.0f;
    const float bottomHeight = hasBottom ? std::clamp(workSize.y * 0.27f, 180.0f, 360.0f) : 0.0f;

    const float topHeight = workSize.y - bottomHeight;
    const float centerX = workPos.x + leftWidth;
    const float centerWidth = workSize.x - leftWidth - rightWidth;
    const float rightX = workPos.x + workSize.x - rightWidth;

    if (hasLeft)
    {
        DrawDockRegion(
            "##DockLeft",
            "Left",
            ImVec2 {workPos.x, workPos.y},
            ImVec2 {leftWidth, topHeight},
            PanelDockSlot::Left,
            context);
    }

    if (hasCenter)
    {
        DrawDockRegion(
            "##DockCenter",
            "Center",
            ImVec2 {centerX, workPos.y},
            ImVec2 {centerWidth, topHeight},
            PanelDockSlot::Center,
            context);
    }

    if (hasRight)
    {
        DrawDockRegion(
            "##DockRight",
            "Right",
            ImVec2 {rightX, workPos.y},
            ImVec2 {rightWidth, topHeight},
            PanelDockSlot::Right,
            context);
    }

    if (hasBottom)
    {
        DrawDockRegion(
            "##DockBottom",
            "Bottom",
            ImVec2 {workPos.x, workPos.y + topHeight},
            ImVec2 {workSize.x, bottomHeight},
            PanelDockSlot::Bottom,
            context);
    }

    for (auto& panel : m_panels)
    {
        if (!panel.open || (panel.slot != PanelDockSlot::Floating))
        {
            continue;
        }

        if (ImGui::Begin(panel.panel->Name(), &panel.open))
        {
            panel.panel->Draw(context);
        }
        ImGui::End();
    }
#else
    (void)context;
#endif
}

void PanelRegistry::ResetLayout()
{
    for (auto& panel : m_panels)
    {
        panel.slot = panel.defaultSlot;
        if (panel.lockOpen)
        {
            panel.open = true;
        }
    }
}

void PanelRegistry::DrawDockRegion(
    const char* windowId,
    const char* title,
    const ImVec2& position,
    const ImVec2& size,
    const PanelDockSlot slot,
    EditorContext& context)
{
#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
    ImGui::SetNextWindowPos(position);
    ImGui::SetNextWindowSize(size);
    if (slot == PanelDockSlot::Center)
    {
        ImGui::SetNextWindowBgAlpha(0.0f);
    }

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings;
    if (slot == PanelDockSlot::Center)
    {
        flags |= ImGuiWindowFlags_NoBackground;
    }

    if (!ImGui::Begin(windowId, nullptr, flags))
    {
        ImGui::End();
        return;
    }

    if (slot != PanelDockSlot::Center)
    {
        ImGui::TextUnformatted(title);
        ImGui::Separator();
    }

    if (ImGui::BeginTabBar("PanelTabs", ImGuiTabBarFlags_Reorderable))
    {
        for (auto& panel : m_panels)
        {
            if (!panel.open || (panel.slot != slot))
            {
                continue;
            }

            ImGui::PushID(panel.panel->Name());
            bool keepOpen = panel.open;
            bool opened = false;
            if (panel.lockOpen)
            {
                opened = ImGui::BeginTabItem(panel.panel->Name());
            }
            else
            {
                opened = ImGui::BeginTabItem(panel.panel->Name(), &keepOpen);
            }

            if (opened)
            {
                panel.panel->Draw(context);
                ImGui::EndTabItem();
            }
            if (!panel.lockOpen)
            {
                panel.open = keepOpen;
            }

            DrawDockContextMenu(panel);
            ImGui::PopID();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
#else
    (void)windowId;
    (void)title;
    (void)position;
    (void)size;
    (void)slot;
    (void)context;
#endif
}

void PanelRegistry::DrawDockContextMenu(PanelEntry& panel)
{
#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
    if (!ImGui::BeginPopupContextItem("DockContext"))
    {
        return;
    }

    if (panel.lockSlot)
    {
        ImGui::BeginDisabled();
    }

    if (ImGui::MenuItem("Dock Left", nullptr, panel.slot == PanelDockSlot::Left))
    {
        panel.slot = PanelDockSlot::Left;
    }
    if (ImGui::MenuItem("Dock Center", nullptr, panel.slot == PanelDockSlot::Center))
    {
        panel.slot = PanelDockSlot::Center;
    }
    if (ImGui::MenuItem("Dock Right", nullptr, panel.slot == PanelDockSlot::Right))
    {
        panel.slot = PanelDockSlot::Right;
    }
    if (ImGui::MenuItem("Dock Bottom", nullptr, panel.slot == PanelDockSlot::Bottom))
    {
        panel.slot = PanelDockSlot::Bottom;
    }
    if (ImGui::MenuItem("Floating", nullptr, panel.slot == PanelDockSlot::Floating))
    {
        panel.slot = PanelDockSlot::Floating;
    }

    if (panel.lockSlot)
    {
        ImGui::EndDisabled();
        panel.slot = PanelDockSlot::Center;
    }

    ImGui::EndPopup();
#else
    (void)panel;
#endif
}

bool PanelRegistry::HasOpenPanels(const PanelDockSlot slot) const
{
    for (const auto& panel : m_panels)
    {
        if (panel.open && (panel.slot == slot))
        {
            return true;
        }
    }
    return false;
}
} // namespace rg::editor
