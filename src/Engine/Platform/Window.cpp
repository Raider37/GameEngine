#include "Engine/Platform/Window.h"

#include <memory>
#include <string>
#include <utility>

#include "Engine/Core/Log.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
#include <backends/imgui_impl_win32.h>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif
#endif

namespace rg
{
namespace
{
class NullWindow final : public IWindow
{
public:
    bool Initialize(const WindowSpec& spec) override
    {
        m_spec = spec;
        m_shouldClose = false;
        return true;
    }

    void PollEvents(InputState& input) override
    {
        // Headless fallback backend: no OS events, keep deterministic input.
        input.SetKey(KeyCode::Escape, false);
    }

    [[nodiscard]] bool ShouldClose() const override
    {
        return m_shouldClose;
    }

    void RequestClose() override
    {
        m_shouldClose = true;
    }

    [[nodiscard]] void* NativeHandle() const override
    {
        return nullptr;
    }

    [[nodiscard]] std::uint32_t Width() const override
    {
        return m_spec.width;
    }

    [[nodiscard]] std::uint32_t Height() const override
    {
        return m_spec.height;
    }

private:
    WindowSpec m_spec;
    bool m_shouldClose = false;
};

#if defined(_WIN32)
std::wstring Utf8ToWide(const std::string& value)
{
    if (value.empty())
    {
        return {};
    }

    const int requiredSize = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
    if (requiredSize <= 0)
    {
        return {};
    }

    std::wstring result(static_cast<std::size_t>(requiredSize), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, result.data(), requiredSize);
    result.pop_back();
    return result;
}

class Win32Window final : public IWindow
{
public:
    bool Initialize(const WindowSpec& spec) override
    {
        m_spec = spec;
        m_hinstance = GetModuleHandleW(nullptr);
        if (m_hinstance == nullptr)
        {
            return false;
        }

        if (!RegisterClass())
        {
            return false;
        }

        RECT rect {0, 0, static_cast<LONG>(spec.width), static_cast<LONG>(spec.height)};
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

        const std::wstring title = Utf8ToWide(spec.title);
        m_hwnd = CreateWindowExW(
            0,
            kWindowClassName,
            title.empty() ? L"RaiderEngine" : title.c_str(),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            rect.right - rect.left,
            rect.bottom - rect.top,
            nullptr,
            nullptr,
            m_hinstance,
            this);

        if (m_hwnd == nullptr)
        {
            return false;
        }

        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);

        RECT clientRect {};
        GetClientRect(m_hwnd, &clientRect);
        m_width = static_cast<std::uint32_t>(clientRect.right - clientRect.left);
        m_height = static_cast<std::uint32_t>(clientRect.bottom - clientRect.top);
        m_shouldClose = false;
        return true;
    }

    void PollEvents(InputState& input) override
    {
        m_input = &input;

        MSG msg {};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);

            if (msg.message == WM_QUIT)
            {
                m_shouldClose = true;
            }
        }

        m_input = nullptr;
    }

    [[nodiscard]] bool ShouldClose() const override
    {
        return m_shouldClose;
    }

    void RequestClose() override
    {
        if (m_hwnd != nullptr)
        {
            PostMessageW(m_hwnd, WM_CLOSE, 0, 0);
        }
        else
        {
            m_shouldClose = true;
        }
    }

    [[nodiscard]] void* NativeHandle() const override
    {
        return m_hwnd;
    }

    [[nodiscard]] std::uint32_t Width() const override
    {
        return m_width;
    }

    [[nodiscard]] std::uint32_t Height() const override
    {
        return m_height;
    }

private:
    static constexpr wchar_t kWindowClassName[] = L"RaiderEngineWin32WindowClass";

    bool RegisterClass()
    {
        WNDCLASSEXW existing {};
        if (GetClassInfoExW(m_hinstance, kWindowClassName, &existing))
        {
            return true;
        }

        WNDCLASSEXW wc {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = &Win32Window::StaticWndProc;
        wc.hInstance = m_hinstance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = kWindowClassName;
        return RegisterClassExW(&wc) != 0;
    }

    static LRESULT CALLBACK StaticWndProc(HWND hwnd, const UINT msg, const WPARAM wParam, const LPARAM lParam)
    {
        Win32Window* self = nullptr;
        if (msg == WM_NCCREATE)
        {
            const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
            self = static_cast<Win32Window*>(create->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        else
        {
            self = reinterpret_cast<Win32Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (self != nullptr)
        {
            self->m_hwnd = hwnd;
            return self->HandleMessage(msg, wParam, lParam);
        }

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleMessage(const UINT msg, const WPARAM wParam, const LPARAM lParam)
    {
#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
        if (ImGui_ImplWin32_WndProcHandler(m_hwnd, msg, wParam, lParam))
        {
            return 1;
        }
#endif

        switch (msg)
        {
        case WM_CLOSE:
            m_shouldClose = true;
            DestroyWindow(m_hwnd);
            return 0;
        case WM_DESTROY:
            m_shouldClose = true;
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:
            m_width = static_cast<std::uint32_t>(LOWORD(lParam));
            m_height = static_cast<std::uint32_t>(HIWORD(lParam));
            return 0;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            ApplyKey(static_cast<UINT>(wParam), true);
            return 0;
        case WM_KEYUP:
        case WM_SYSKEYUP:
            ApplyKey(static_cast<UINT>(wParam), false);
            return 0;
        case WM_KILLFOCUS:
            if (m_input != nullptr)
            {
                m_input->Reset();
            }
            return 0;
        default:
            break;
        }

        return DefWindowProcW(m_hwnd, msg, wParam, lParam);
    }

    void ApplyKey(const UINT vk, const bool isDown)
    {
        if (m_input == nullptr)
        {
            return;
        }

        switch (vk)
        {
        case VK_ESCAPE:
            m_input->SetKey(KeyCode::Escape, isDown);
            break;
        case VK_SPACE:
            m_input->SetKey(KeyCode::Space, isDown);
            break;
        case 'W':
            m_input->SetKey(KeyCode::W, isDown);
            break;
        case 'A':
            m_input->SetKey(KeyCode::A, isDown);
            break;
        case 'S':
            m_input->SetKey(KeyCode::S, isDown);
            break;
        case 'D':
            m_input->SetKey(KeyCode::D, isDown);
            break;
        case 'Q':
            m_input->SetKey(KeyCode::Q, isDown);
            break;
        case 'E':
            m_input->SetKey(KeyCode::E, isDown);
            break;
        case 'F':
            m_input->SetKey(KeyCode::F, isDown);
            break;
        default:
            break;
        }
    }

    WindowSpec m_spec;
    InputState* m_input = nullptr;
    HWND m_hwnd = nullptr;
    HINSTANCE m_hinstance = nullptr;
    std::uint32_t m_width = 0;
    std::uint32_t m_height = 0;
    bool m_shouldClose = false;
};
#endif
} // namespace

bool WindowSystem::Initialize(const WindowSpec& spec)
{
#if defined(_WIN32)
    m_window = std::make_unique<Win32Window>();
    if (m_window->Initialize(spec))
    {
        Log::Write(LogLevel::Info, "Window backend: Win32");
        return true;
    }

    Log::Write(LogLevel::Warning, "Win32 window initialization failed. Falling back to Null window backend.");
#endif

    m_window = std::make_unique<NullWindow>();
    const bool initialized = m_window->Initialize(spec);
    if (initialized)
    {
        Log::Write(LogLevel::Info, "Window backend: Null");
    }
    return initialized;
}

void WindowSystem::PollEvents(InputState& input)
{
    if (m_window)
    {
        m_window->PollEvents(input);
    }
}

bool WindowSystem::ShouldClose() const
{
    return m_window ? m_window->ShouldClose() : true;
}

void WindowSystem::RequestClose()
{
    if (m_window)
    {
        m_window->RequestClose();
    }
}

void* WindowSystem::NativeHandle() const
{
    return m_window ? m_window->NativeHandle() : nullptr;
}

std::uint32_t WindowSystem::Width() const
{
    return m_window ? m_window->Width() : 0U;
}

std::uint32_t WindowSystem::Height() const
{
    return m_window ? m_window->Height() : 0U;
}
} // namespace rg
