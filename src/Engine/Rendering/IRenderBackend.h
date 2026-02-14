#pragma once

#include <cstdint>
#include <functional>

#include "Engine/Resources/ResourceManager.h"
#include "Engine/Scene/World.h"

namespace rg
{
namespace minecraft
{
class VoxelWorld;
}

struct RenderBackendContext
{
    void* nativeWindowHandle = nullptr;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    bool vsync = true;
};

using UiRenderCallback = std::function<void()>;

class IRenderBackend
{
public:
    virtual ~IRenderBackend() = default;

    virtual bool Initialize(ResourceManager& resources, const RenderBackendContext& context) = 0;
    virtual void Render(
        const World& world,
        const minecraft::VoxelWorld* voxelWorld,
        std::uint64_t frameIndex,
        const UiRenderCallback& uiCallback) = 0;
    [[nodiscard]] virtual const char* Name() const = 0;
};
} // namespace rg
