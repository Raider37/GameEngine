#pragma once

#include "Editor/Panels/IEditorPanel.h"

namespace rg::editor
{
class LevelClassesPanel final : public IEditorPanel
{
public:
    [[nodiscard]] const char* Name() const override;
    void Draw(EditorContext& context) override;
};
} // namespace rg::editor

