#include "Engine/Scene/Entity.h"

#include <stdexcept>
#include <utility>

#include "Engine/Scene/World.h"

namespace rg
{
Entity::Entity(World* world, const Id id) : m_world(world), m_id(id)
{
}

bool Entity::IsValid() const
{
    return (m_world != nullptr) && (m_id != ecs::InvalidEntity) && Registry().IsAlive(m_id);
}

Entity::Id Entity::GetId() const
{
    return m_id;
}

const std::string& Entity::GetName() const
{
    return Registry().GetComponent<NameComponent>(m_id).value;
}

void Entity::SetName(std::string name)
{
    Registry().GetComponent<NameComponent>(m_id).value = std::move(name);
}

TransformComponent& Entity::Transform()
{
    return Registry().GetComponent<TransformComponent>(m_id);
}

const TransformComponent& Entity::Transform() const
{
    return Registry().GetComponent<TransformComponent>(m_id);
}

void Entity::AttachScript(std::string className)
{
    Registry().AddComponent<ScriptComponent>(m_id, ScriptComponent {std::move(className), true});
}

bool Entity::HasScript() const
{
    return Registry().HasComponent<ScriptComponent>(m_id);
}

ScriptComponent& Entity::Script()
{
    return Registry().GetComponent<ScriptComponent>(m_id);
}

const ScriptComponent& Entity::Script() const
{
    return Registry().GetComponent<ScriptComponent>(m_id);
}

void Entity::AttachMesh(std::string meshAsset, std::string materialAsset)
{
    Registry().AddComponent<MeshComponent>(m_id, MeshComponent {std::move(meshAsset), std::move(materialAsset), true});
}

bool Entity::HasMesh() const
{
    return Registry().HasComponent<MeshComponent>(m_id);
}

MeshComponent& Entity::Mesh()
{
    return Registry().GetComponent<MeshComponent>(m_id);
}

const MeshComponent& Entity::Mesh() const
{
    return Registry().GetComponent<MeshComponent>(m_id);
}

ecs::Registry& Entity::Registry()
{
    if (m_world == nullptr)
    {
        throw std::runtime_error("Entity is not bound to a world.");
    }
    return m_world->GetRegistry();
}

const ecs::Registry& Entity::Registry() const
{
    if (m_world == nullptr)
    {
        throw std::runtime_error("Entity is not bound to a world.");
    }
    return m_world->GetRegistry();
}
} // namespace rg
