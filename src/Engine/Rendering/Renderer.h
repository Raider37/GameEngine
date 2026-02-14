#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include "Engine/Rendering/IRenderBackend.h"
#include "Engine/Rendering/RenderAPI.h"
#include "Engine/Resources/ResourceManager.h"
#include "Engine/Scene/World.h"

namespace rg
{
struct RendererConfig
{
    RenderAPI api = RenderAPI::Null;
    bool vsync = true;
};

class Renderer
{
public:
    bool Initialize(const RendererConfig& config, ResourceManager& resources, const RenderBackendContext& context);
    void Render(
        const World& world,
        const minecraft::VoxelWorld* voxelWorld = nullptr,
        const UiRenderCallback& uiCallback = {});

    [[nodiscard]] const char* BackendName() const;

private:
    std::uint64_t m_frameIndex = 0;
    std::unique_ptr<IRenderBackend> m_backend;
};
} // namespace rg
