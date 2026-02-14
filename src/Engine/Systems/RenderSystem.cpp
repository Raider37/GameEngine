#include "Engine/Systems/RenderSystem.h"

#include <string>

#include "Editor/EditorUI.h"
#include "Engine/Core/Log.h"

namespace rg
{
const char* RenderSystem::Name() const
{
    return "RenderSystem";
}

bool RenderSystem::Initialize(SystemContext& context)
{
    Log::Write(LogLevel::Info, std::string("Initialized RenderSystem with backend: ") + context.renderer.BackendName());
    return true;
}

void RenderSystem::Update(SystemContext& context, const float deltaSeconds)
{
    if ((context.editorUI != nullptr) && context.editorUI->IsEnabled())
    {
        context.editorUI->SetFrameStats(
            editor::EditorFrameStats {
                deltaSeconds,
                context.frameIndex,
                context.renderer.BackendName()});

        context.renderer.Render(context.world, context.voxelWorld, [editor = context.editorUI]()
        {
            editor->Render();
        });
        return;
    }

    context.renderer.Render(context.world, context.voxelWorld);
}
} // namespace rg
