#pragma once

#include "Engine/Systems/ISystem.h"

namespace rg
{
class PhysicsSystem final : public ISystem
{
public:
    [[nodiscard]] const char* Name() const override;
    bool Initialize(SystemContext& context) override;
    void Update(SystemContext& context, float deltaSeconds) override;

private:
    float m_gravity = -9.81f;
};
} // namespace rg
