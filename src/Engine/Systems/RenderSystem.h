#pragma once

#include "Engine/Systems/ISystem.h"

namespace rg
{
class RenderSystem final : public ISystem
{
public:
    [[nodiscard]] const char* Name() const override;
    bool Initialize(SystemContext& context) override;
    void Update(SystemContext& context, float deltaSeconds) override;
};
} // namespace rg
