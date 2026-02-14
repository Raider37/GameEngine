#pragma once

#include <cstddef>

#include "Engine/Rendering/IRenderBackend.h"

namespace rg
{
class NullRenderBackend final : public IRenderBackend
{
public:
    bool Initialize(ResourceManager& resources, const RenderBackendContext& context) override;
    void Render(
        const World& world,
        const minecraft::VoxelWorld* voxelWorld,
        std::uint64_t frameIndex,
        const UiRenderCallback& uiCallback) override;
    [[nodiscard]] const char* Name() const override;

private:
    std::size_t m_cachedShaderBytes = 0;
};
} // namespace rg
