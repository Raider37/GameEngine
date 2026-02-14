#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "Engine/Input/InputState.h"

namespace rg
{
struct WindowSpec
{
    std::string title = "RaiderEngine";
    std::uint32_t width = 1600;
    std::uint32_t height = 900;
};

class IWindow
{
public:
    virtual ~IWindow() = default;

    virtual bool Initialize(const WindowSpec& spec) = 0;
    virtual void PollEvents(InputState& input) = 0;
    [[nodiscard]] virtual bool ShouldClose() const = 0;
    virtual void RequestClose() = 0;
    [[nodiscard]] virtual void* NativeHandle() const = 0;
    [[nodiscard]] virtual std::uint32_t Width() const = 0;
    [[nodiscard]] virtual std::uint32_t Height() const = 0;
};

class WindowSystem
{
public:
    bool Initialize(const WindowSpec& spec);
    void PollEvents(InputState& input);

    [[nodiscard]] bool ShouldClose() const;
    void RequestClose();
    [[nodiscard]] void* NativeHandle() const;
    [[nodiscard]] std::uint32_t Width() const;
    [[nodiscard]] std::uint32_t Height() const;

private:
    std::unique_ptr<IWindow> m_window;
};
} // namespace rg
