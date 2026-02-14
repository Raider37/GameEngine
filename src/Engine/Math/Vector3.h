#pragma once

#include <cmath>
#include <string>

namespace rg
{
struct Vector3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    [[nodiscard]] float Length() const
    {
        return std::sqrt((x * x) + (y * y) + (z * z));
    }
};
} // namespace rg
