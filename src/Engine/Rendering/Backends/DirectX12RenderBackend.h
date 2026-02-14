#pragma once

#include <memory>

#include "Engine/Rendering/IRenderBackend.h"

namespace rg
{
class DirectX12RenderBackend final : public IRenderBackend
{
public:
    DirectX12RenderBackend();
    ~DirectX12RenderBackend() override;

    bool Initialize(ResourceManager& resources, const RenderBackendContext& context) override;
    void Render(
        const World& world,
        const minecraft::VoxelWorld* voxelWorld,
        std::uint64_t frameIndex,
        const UiRenderCallback& uiCallback) override;
    [[nodiscard]] const char* Name() const override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    bool m_pipelineReady = false;
    bool m_reportedRenderFailure = false;
};
} // namespace rg
