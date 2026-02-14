#include "Engine/Systems/ScriptSystem.h"

#include "Engine/Core/Log.h"

namespace rg
{
const char* ScriptSystem::Name() const
{
    return "ScriptSystem";
}

bool ScriptSystem::Initialize(SystemContext& context)
{
    (void)context;
    Log::Write(LogLevel::Info, "Initialized ScriptSystem.");
    return true;
}

void ScriptSystem::Update(SystemContext& context, const float deltaSeconds)
{
    context.scriptHost.Tick(context.world, deltaSeconds);
}
} // namespace rg
