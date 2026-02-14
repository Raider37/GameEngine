#pragma once

#include "Editor/Panels/IEditorPanel.h"

namespace rg::editor
{
class ViewportPanel final : public IEditorPanel
{
public:
    [[nodiscard]] const char* Name() const override;
    void Draw(EditorContext& context) override;

private:
    float m_mapColumnWidth = 360.0f;
    float m_zoomPixelsPerMeter = 16.0f;
    float m_panX = 0.0f;
    float m_panZ = 0.0f;
    bool m_followSelection = true;
    bool m_showMapOverlay = true;
    bool m_draggingSelected = false;
};
} // namespace rg::editor
