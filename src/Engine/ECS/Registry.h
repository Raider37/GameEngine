#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace rg::ecs
{
using EntityId = std::uint32_t;
constexpr EntityId InvalidEntity = 0;

class Registry
{
public:
    EntityId CreateEntity()
    {
        const EntityId id = m_nextId++;
        m_alive.insert(id);
        m_entities.push_back(id);
        return id;
    }

    void DestroyEntity(const EntityId id)
    {
        if (!m_alive.erase(id))
        {
            return;
        }

        m_entities.erase(std::remove(m_entities.begin(), m_entities.end(), id), m_entities.end());
        for (auto& pair : m_componentStores)
        {
            pair.second->Remove(id);
        }
    }

    [[nodiscard]] bool IsAlive(const EntityId id) const
    {
        return m_alive.contains(id);
    }

    [[nodiscard]] std::size_t EntityCount() const
    {
        return m_entities.size();
    }

    [[nodiscard]] const std::vector<EntityId>& Entities() const
    {
        return m_entities;
    }

    template <typename Component, typename... Args>
    Component& AddComponent(const EntityId id, Args&&... args)
    {
        RequireAlive(id);
        auto& data = MutableStore<Component>().data;
        auto [it, inserted] = data.try_emplace(id, std::forward<Args>(args)...);
        if (!inserted)
        {
            it->second = Component(std::forward<Args>(args)...);
        }
        return it->second;
    }

    template <typename Component>
    void RemoveComponent(const EntityId id)
    {
        MutableStore<Component>().data.erase(id);
    }

    template <typename Component>
    [[nodiscard]] bool HasComponent(const EntityId id) const
    {
        const auto* store = TryStore<Component>();
        return (store != nullptr) && store->data.contains(id);
    }

    template <typename Component>
    Component& GetComponent(const EntityId id)
    {
        auto& data = MutableStore<Component>().data;
        const auto it = data.find(id);
        if (it == data.end())
        {
            throw std::runtime_error("Component is missing on entity.");
        }
        return it->second;
    }

    template <typename Component>
    const Component& GetComponent(const EntityId id) const
    {
        const auto* store = TryStore<Component>();
        if (store == nullptr)
        {
            throw std::runtime_error("Component store is missing.");
        }

        const auto it = store->data.find(id);
        if (it == store->data.end())
        {
            throw std::runtime_error("Component is missing on entity.");
        }
        return it->second;
    }

    template <typename... Components, typename Func>
    void ForEach(Func&& func)
    {
        for (const EntityId entity : m_entities)
        {
            if ((HasComponent<Components>(entity) && ...))
            {
                func(entity, GetComponent<Components>(entity)...);
            }
        }
    }

    template <typename... Components, typename Func>
    void ForEach(Func&& func) const
    {
        for (const EntityId entity : m_entities)
        {
            if ((HasComponent<Components>(entity) && ...))
            {
                func(entity, GetComponent<Components>(entity)...);
            }
        }
    }

private:
    struct IStore
    {
        virtual ~IStore() = default;
        virtual void Remove(EntityId id) = 0;
    };

    template <typename Component>
    struct Store final : IStore
    {
        std::unordered_map<EntityId, Component> data;

        void Remove(const EntityId id) override
        {
            data.erase(id);
        }
    };

    template <typename Component>
    Store<Component>* TryStore()
    {
        const auto it = m_componentStores.find(std::type_index(typeid(Component)));
        if (it == m_componentStores.end())
        {
            return nullptr;
        }
        return static_cast<Store<Component>*>(it->second.get());
    }

    template <typename Component>
    const Store<Component>* TryStore() const
    {
        const auto it = m_componentStores.find(std::type_index(typeid(Component)));
        if (it == m_componentStores.end())
        {
            return nullptr;
        }
        return static_cast<const Store<Component>*>(it->second.get());
    }

    template <typename Component>
    Store<Component>& MutableStore()
    {
        const auto key = std::type_index(typeid(Component));
        const auto it = m_componentStores.find(key);
        if (it != m_componentStores.end())
        {
            return *static_cast<Store<Component>*>(it->second.get());
        }

        auto store = std::make_unique<Store<Component>>();
        auto* ptr = store.get();
        m_componentStores.emplace(key, std::move(store));
        return *ptr;
    }

    void RequireAlive(const EntityId id) const
    {
        if (!IsAlive(id))
        {
            throw std::runtime_error("Entity is not alive.");
        }
    }

    EntityId m_nextId = 1;
    std::vector<EntityId> m_entities;
    std::unordered_set<EntityId> m_alive;
    std::unordered_map<std::type_index, std::unique_ptr<IStore>> m_componentStores;
};
} // namespace rg::ecs
