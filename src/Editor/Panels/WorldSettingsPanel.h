#pragma once

#include "Editor/Panels/IEditorPanel.h"

namespace rg::editor
{
class WorldSettingsPanel final : public IEditorPanel
{
public:
    [[nodiscard]] const char* Name() const override;
    void Draw(EditorContext& context) override;

private:
    int m_voxelRadius = 8;
    int m_voxelSeed = 1337;
    bool m_initialized = false;
};
} // namespace rg::editor

