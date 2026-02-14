#pragma once

#include <functional>
#include <vector>

#include "Engine/ECS/Registry.h"
#include "Engine/Scene/Entity.h"

namespace rg
{
class World
{
public:
    Entity CreateEntity(const std::string& name);
    void DestroyEntity(Entity entity);

    [[nodiscard]] std::vector<Entity> Entities();
    [[nodiscard]] std::vector<Entity> Entities() const;

    [[nodiscard]] Entity FindEntity(Entity::Id id);
    [[nodiscard]] Entity FindEntity(Entity::Id id) const;

    [[nodiscard]] std::size_t EntityCount() const;

    [[nodiscard]] ecs::Registry& GetRegistry();
    [[nodiscard]] const ecs::Registry& GetRegistry() const;

    template <typename... Components, typename Func>
    void ForEach(Func&& func)
    {
        m_registry.ForEach<Components...>([this, &func](const ecs::EntityId id, Components&... components)
        {
            func(Entity(this, id), components...);
        });
    }

    template <typename... Components, typename Func>
    void ForEach(Func&& func) const
    {
        m_registry.ForEach<Components...>([this, &func](const ecs::EntityId id, const Components&... components)
        {
            func(Entity(const_cast<World*>(this), id), components...);
        });
    }

private:
    ecs::Registry m_registry;
};
} // namespace rg
