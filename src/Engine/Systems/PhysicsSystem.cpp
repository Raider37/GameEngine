#include "Engine/Systems/PhysicsSystem.h"

#include "Engine/Core/Log.h"
#include "Engine/Scene/Components.h"

namespace rg
{
const char* PhysicsSystem::Name() const
{
    return "PhysicsSystem";
}

bool PhysicsSystem::Initialize(SystemContext& context)
{
    (void)context;
    Log::Write(LogLevel::Info, "Initialized PhysicsSystem.");
    return true;
}

void PhysicsSystem::Update(SystemContext& context, const float deltaSeconds)
{
    context.world.ForEach<RigidbodyComponent, TransformComponent>(
        [&](Entity /*entity*/, RigidbodyComponent& body, TransformComponent& transform)
        {
            if (body.isKinematic)
            {
                return;
            }

            if (body.useGravity)
            {
                body.velocity.y += m_gravity * deltaSeconds;
            }

            transform.position.x += body.velocity.x * deltaSeconds;
            transform.position.y += body.velocity.y * deltaSeconds;
            transform.position.z += body.velocity.z * deltaSeconds;

            // Minimal ground collision plane at y=0 for predictable sandbox behavior.
            if (transform.position.y < 0.0f)
            {
                transform.position.y = 0.0f;
                if (body.velocity.y < 0.0f)
                {
                    body.velocity.y = 0.0f;
                }
            }
        });
}
} // namespace rg
