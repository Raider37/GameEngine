#include "Engine/Scene/World.h"

namespace rg
{
Entity World::CreateEntity(const std::string& name)
{
    const auto id = m_registry.CreateEntity();
    m_registry.AddComponent<NameComponent>(id, NameComponent {name});
    m_registry.AddComponent<TransformComponent>(id, TransformComponent {});
    return Entity(this, id);
}

void World::DestroyEntity(const Entity entity)
{
    if (entity.IsValid())
    {
        m_registry.DestroyEntity(entity.GetId());
    }
}

std::vector<Entity> World::Entities()
{
    std::vector<Entity> handles;
    handles.reserve(m_registry.EntityCount());
    for (const auto id : m_registry.Entities())
    {
        handles.emplace_back(this, id);
    }
    return handles;
}

std::vector<Entity> World::Entities() const
{
    std::vector<Entity> handles;
    handles.reserve(m_registry.EntityCount());
    for (const auto id : m_registry.Entities())
    {
        handles.emplace_back(const_cast<World*>(this), id);
    }
    return handles;
}

Entity World::FindEntity(const Entity::Id id)
{
    return m_registry.IsAlive(id) ? Entity(this, id) : Entity {};
}

Entity World::FindEntity(const Entity::Id id) const
{
    return m_registry.IsAlive(id) ? Entity(const_cast<World*>(this), id) : Entity {};
}

std::size_t World::EntityCount() const
{
    return m_registry.EntityCount();
}

ecs::Registry& World::GetRegistry()
{
    return m_registry;
}

const ecs::Registry& World::GetRegistry() const
{
    return m_registry;
}
} // namespace rg
