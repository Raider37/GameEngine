#include "Engine/Rendering/Backends/VulkanRenderBackend.h"

#include <sstream>

#include "Engine/Core/Log.h"
#include "Engine/Scene/Components.h"

namespace rg
{
bool VulkanRenderBackend::Initialize(ResourceManager& resources, const RenderBackendContext& context)
{
    const auto vert = resources.LoadText("shaders/default.vert.glsl");
    const auto frag = resources.LoadText("shaders/default.frag.glsl");
    m_pipelineReady = vert.has_value() && frag.has_value();
    (void)context;

    if (m_pipelineReady)
    {
        Log::Write(LogLevel::Info, "[Vulkan] Backend initialized with GLSL shader assets.");
    }
    else
    {
        Log::Write(LogLevel::Warning, "[Vulkan] Shader assets missing. Backend running in stub mode.");
    }

    return true;
}

void VulkanRenderBackend::Render(
    const World& world,
    const minecraft::VoxelWorld* voxelWorld,
    const std::uint64_t frameIndex,
    const UiRenderCallback& uiCallback)
{
    (void)voxelWorld;
    (void)uiCallback;
    std::size_t drawCalls = 0;
    world.ForEach<MeshComponent, TransformComponent>([&](Entity /*entity*/, const MeshComponent& mesh, const TransformComponent& /*transform*/)
    {
        if (mesh.visible)
        {
            ++drawCalls;
        }
    });

    std::ostringstream oss;
    oss << "[Vulkan] Frame " << frameIndex
        << " | entities=" << world.EntityCount()
        << " | drawCalls=" << drawCalls
        << " | pipeline=" << (m_pipelineReady ? "ready" : "stub");
    Log::Write(LogLevel::Info, oss.str());
}

const char* VulkanRenderBackend::Name() const
{
    return "Vulkan";
}
} // namespace rg
