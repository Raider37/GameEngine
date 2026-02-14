#pragma once

#include "Engine/Systems/ISystem.h"

namespace rg
{
class VoxelGameplaySystem final : public ISystem
{
public:
    [[nodiscard]] const char* Name() const override;
    bool Initialize(SystemContext& context) override;
    void Update(SystemContext& context, float deltaSeconds) override;

private:
    void EnsurePlayerEntity(SystemContext& context) const;
    void UpdateMovement(SystemContext& context, Entity player, float deltaSeconds) const;
    void UpdateBlockInteraction(SystemContext& context, Entity player) const;
};
} // namespace rg
