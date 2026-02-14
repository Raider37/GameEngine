#pragma once

#include "Editor/EditorContext.h"

namespace rg::editor
{
class IEditorPanel
{
public:
    virtual ~IEditorPanel() = default;

    [[nodiscard]] virtual const char* Name() const = 0;
    virtual void Draw(EditorContext& context) = 0;
};
} // namespace rg::editor
