#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace rg
{
enum class KeyCode : std::uint8_t
{
    Escape = 0,
    Space,
    W,
    A,
    S,
    D,
    Q,
    E,
    F,
    Count
};

class InputState
{
public:
    void BeginFrame();
    void Reset();
    void SetKey(KeyCode key, bool isDown);

    [[nodiscard]] bool IsDown(KeyCode key) const;
    [[nodiscard]] bool WasPressed(KeyCode key) const;
    [[nodiscard]] bool WasReleased(KeyCode key) const;

private:
    static constexpr std::size_t kKeyCount = static_cast<std::size_t>(KeyCode::Count);

    static constexpr std::size_t ToIndex(const KeyCode key)
    {
        return static_cast<std::size_t>(key);
    }

    std::array<bool, kKeyCount> m_current {};
    std::array<bool, kKeyCount> m_previous {};
};
} // namespace rg
