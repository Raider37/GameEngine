#pragma once

#include "Engine/Systems/SystemContext.h"

namespace rg
{
class ISystem
{
public:
    virtual ~ISystem() = default;

    [[nodiscard]] virtual const char* Name() const = 0;
    virtual bool Initialize(SystemContext& context) = 0;
    virtual void Update(SystemContext& context, float deltaSeconds) = 0;
};
} // namespace rg
