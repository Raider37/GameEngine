#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include "Engine/ECS/Registry.h"
#include "Engine/Scene/Components.h"

namespace rg
{
class World;

class Entity
{
public:
    using Id = ecs::EntityId;

    Entity() = default;
    Entity(World* world, Id id);

    [[nodiscard]] bool IsValid() const;

    [[nodiscard]] Id GetId() const;
    [[nodiscard]] const std::string& GetName() const;
    void SetName(std::string name);

    [[nodiscard]] TransformComponent& Transform();
    [[nodiscard]] const TransformComponent& Transform() const;

    void AttachScript(std::string className);
    [[nodiscard]] bool HasScript() const;
    [[nodiscard]] ScriptComponent& Script();
    [[nodiscard]] const ScriptComponent& Script() const;

    void AttachMesh(std::string meshAsset, std::string materialAsset = {});
    [[nodiscard]] bool HasMesh() const;
    [[nodiscard]] MeshComponent& Mesh();
    [[nodiscard]] const MeshComponent& Mesh() const;

    template <typename Component, typename... Args>
    Component& AddComponent(Args&&... args)
    {
        return Registry().AddComponent<Component>(m_id, std::forward<Args>(args)...);
    }

    template <typename Component>
    [[nodiscard]] bool HasComponent() const
    {
        return Registry().HasComponent<Component>(m_id);
    }

    template <typename Component>
    Component& GetComponent()
    {
        return Registry().GetComponent<Component>(m_id);
    }

    template <typename Component>
    const Component& GetComponent() const
    {
        return Registry().GetComponent<Component>(m_id);
    }

private:
    [[nodiscard]] ecs::Registry& Registry();
    [[nodiscard]] const ecs::Registry& Registry() const;

    World* m_world = nullptr;
    Id m_id = ecs::InvalidEntity;
};
} // namespace rg
