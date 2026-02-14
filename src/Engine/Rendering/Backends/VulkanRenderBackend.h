#pragma once

#include "Engine/Rendering/IRenderBackend.h"

namespace rg
{
class VulkanRenderBackend final : public IRenderBackend
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
    bool m_pipelineReady = false;
};
} // namespace rg
