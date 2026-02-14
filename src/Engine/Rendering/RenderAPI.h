#pragma once

#include <string_view>

namespace rg
{
enum class RenderAPI
{
    Null,
    DirectX12,
    Vulkan
};

inline const char* ToString(const RenderAPI api)
{
    switch (api)
    {
    case RenderAPI::Null:
        return "Null";
    case RenderAPI::DirectX12:
        return "DirectX12";
    case RenderAPI::Vulkan:
        return "Vulkan";
    default:
        return "Unknown";
    }
}
} // namespace rg
