#include "Engine/Input/InputState.h"

namespace rg
{
void InputState::BeginFrame()
{
    m_previous = m_current;
}

void InputState::Reset()
{
    m_current.fill(false);
    m_previous.fill(false);
}

void InputState::SetKey(const KeyCode key, const bool isDown)
{
    m_current[ToIndex(key)] = isDown;
}

bool InputState::IsDown(const KeyCode key) const
{
    return m_current[ToIndex(key)];
}

bool InputState::WasPressed(const KeyCode key) const
{
    const auto index = ToIndex(key);
    return m_current[index] && !m_previous[index];
}

bool InputState::WasReleased(const KeyCode key) const
{
    const auto index = ToIndex(key);
    return !m_current[index] && m_previous[index];
}
} // namespace rg
