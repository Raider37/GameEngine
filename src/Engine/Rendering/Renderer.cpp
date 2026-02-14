#include "Engine/Rendering/Renderer.h"

#include <memory>
#include <string>

#include "Engine/Core/Log.h"
#include "Engine/Rendering/Backends/DirectX12RenderBackend.h"
#include "Engine/Rendering/Backends/NullRenderBackend.h"
#include "Engine/Rendering/Backends/VulkanRenderBackend.h"

namespace rg
{
namespace
{
std::unique_ptr<IRenderBackend> CreateBackend(const RenderAPI api)
{
    switch (api)
    {
    case RenderAPI::DirectX12:
        return std::make_unique<DirectX12RenderBackend>();
    case RenderAPI::Vulkan:
        return std::make_unique<VulkanRenderBackend>();
    case RenderAPI::Null:
    default:
        return std::make_unique<NullRenderBackend>();
    }
}
} // namespace

bool Renderer::Initialize(const RendererConfig& config, ResourceManager& resources, const RenderBackendContext& context)
{
    m_backend = CreateBackend(config.api);
    if (m_backend == nullptr)
    {
        return false;
    }

    RenderBackendContext backendContext = context;
    backendContext.vsync = config.vsync;

    if (!m_backend->Initialize(resources, backendContext))
    {
        Log::Write(LogLevel::Warning, "Requested renderer backend failed. Falling back to Null backend.");
        m_backend = std::make_unique<NullRenderBackend>();
        return m_backend->Initialize(resources, backendContext);
    }

    Log::Write(LogLevel::Info, std::string("Renderer backend: ") + m_backend->Name());
    return true;
}

void Renderer::Render(
    const World& world,
    const minecraft::VoxelWorld* voxelWorld,
    const UiRenderCallback& uiCallback)
{
    if (m_backend == nullptr)
    {
        return;
    }

    ++m_frameIndex;
    m_backend->Render(world, voxelWorld, m_frameIndex, uiCallback);
}

const char* Renderer::BackendName() const
{
    return m_backend ? m_backend->Name() : "None";
}
} // namespace rg
