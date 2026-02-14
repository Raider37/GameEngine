#include "Engine/Rendering/Backends/NullRenderBackend.h"

#include <iomanip>
#include <sstream>

#include "Engine/Core/Log.h"
#include "Engine/Scene/Components.h"

namespace rg
{
bool NullRenderBackend::Initialize(ResourceManager& resources, const RenderBackendContext& context)
{
    const auto shader = resources.LoadText("shaders/default.hlsl");
    m_cachedShaderBytes = shader.has_value() ? shader->size() : 0U;
    (void)context;
    Log::Write(LogLevel::Info, "[NullRenderer] Initialized.");
    return true;
}

void NullRenderBackend::Render(
    const World& world,
    const minecraft::VoxelWorld* voxelWorld,
    const std::uint64_t frameIndex,
    const UiRenderCallback& uiCallback)
{
    (void)voxelWorld;
    (void)uiCallback;
    std::ostringstream oss;
    oss << "[NullRenderer] Frame " << frameIndex
        << " | entities=" << world.EntityCount()
        << " | cachedShaderBytes=" << m_cachedShaderBytes
        << " | scene=";

    bool first = true;
    world.ForEach<NameComponent, TransformComponent>([&](Entity /*entity*/, const NameComponent& name, const TransformComponent& transform)
    {
        if (!first)
        {
            oss << ", ";
        }
        first = false;

        const auto& p = transform.position;
        oss << name.value << " pos("
            << std::fixed << std::setprecision(2)
            << p.x << ", " << p.y << ", " << p.z << ")";
    });

    Log::Write(LogLevel::Info, oss.str());
}

const char* NullRenderBackend::Name() const
{
    return "Null";
}
} // namespace rg
