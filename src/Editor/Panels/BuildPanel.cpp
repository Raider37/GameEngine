#include "Editor/Panels/BuildPanel.h"

#include <chrono>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string_view>
#include <utility>

#include "Engine/Core/Log.h"

#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
#include <imgui.h>
#endif

namespace rg::editor
{
namespace
{
std::string ResolveCMakeExecutable()
{
#if defined(_WIN32)
    constexpr std::string_view kCandidates[] = {
        "C:\\Program Files\\CMake\\bin\\cmake.exe",
        "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe",
        "C:\\Program Files\\Microsoft Visual Studio\\2022\\BuildTools\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe"};

    for (const std::string_view candidate : kCandidates)
    {
        if (std::filesystem::exists(candidate))
        {
            return std::string(candidate);
        }
    }
#endif
    return "cmake";
}

std::string BuildLogPath(const std::string& root, const std::string& label)
{
    std::filesystem::path logPath = std::filesystem::path(root) / "build" / ("editor_" + label + ".log");
    std::error_code error;
    std::filesystem::create_directories(logPath.parent_path(), error);
    return logPath.string();
}
} // namespace

const char* BuildPanel::Name() const
{
    return "Build";
}

void BuildPanel::Draw(EditorContext& context)
{
#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
    (void)context;

    EnsureToolsResolved();
    PollBuildTask();

    ImGui::TextWrapped("Build the native game target or managed script runtime directly from editor.");
    ImGui::Separator();

    ImGui::Text("Project Root: %s", m_projectRoot.empty() ? "<not found>" : m_projectRoot.c_str());
    ImGui::Text("CMake: %s", m_cmakeExecutable.c_str());
    ImGui::Text("dotnet: %s", m_dotnetExecutable.c_str());
    ImGui::Text("Status: %s", m_lastStatus.c_str());

    if (!m_lastTaskName.empty())
    {
        ImGui::Text("Last task: %s (exit=%d)", m_lastTaskName.c_str(), m_lastExitCode);
    }

    ImGui::Separator();
    const bool disabled = m_buildInProgress || m_projectRoot.empty();
    if (disabled)
    {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Configure + Build Release"))
    {
        StartNativeBuild("Release");
    }

    ImGui::SameLine();
    if (ImGui::Button("Configure + Build Debug"))
    {
        StartNativeBuild("Debug");
    }

    if (ImGui::Button("Build Managed Scripts"))
    {
        StartManagedBuild();
    }

    if (disabled)
    {
        ImGui::EndDisabled();
    }

    if (!m_lastLogPath.empty())
    {
        ImGui::SeparatorText("Last Build Log");
        ImGui::TextUnformatted(m_lastLogPath.c_str());
        const std::string tail = ReadTail(m_lastLogPath, 30);
        ImGui::BeginChild("BuildLogTail", ImVec2(0.0f, 250.0f), true);
        ImGui::TextUnformatted(tail.empty() ? "No log yet." : tail.c_str());
        ImGui::EndChild();
    }
#else
    (void)context;
#endif
}

void BuildPanel::EnsureToolsResolved()
{
    if (!m_projectRoot.empty())
    {
        return;
    }

    m_projectRoot = DetectProjectRoot();
    m_cmakeExecutable = ResolveCMakeExecutable();
}

void BuildPanel::PollBuildTask()
{
    if (!m_buildInProgress || !m_buildTask.valid())
    {
        return;
    }

    if (m_buildTask.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
    {
        return;
    }

    m_lastExitCode = m_buildTask.get();
    m_buildInProgress = false;
    m_lastStatus = (m_lastExitCode == 0) ? "Build completed successfully." : "Build failed. Check log.";
    Log::Write(
        (m_lastExitCode == 0) ? LogLevel::Info : LogLevel::Warning,
        "Editor build task finished: " + m_lastTaskName + " (exit=" + std::to_string(m_lastExitCode) + ")");
}

void BuildPanel::StartNativeBuild(const char* configName)
{
    if (m_buildInProgress || m_projectRoot.empty())
    {
        return;
    }

    const std::string buildDir = (std::filesystem::path(m_projectRoot) / "build").string();
    m_lastTaskName = std::string("Native ") + configName;
    m_lastLogPath = BuildLogPath(m_projectRoot, std::string("native_") + configName);
    m_lastStatus = "Running native build...";

    const std::string command =
        Quote(m_cmakeExecutable) + " -S " + Quote(m_projectRoot) + " -B " + Quote(buildDir) +
        " && " + Quote(m_cmakeExecutable) + " --build " + Quote(buildDir) + " --config " + configName +
        " > " + Quote(m_lastLogPath) + " 2>&1";

    m_buildInProgress = true;
    m_buildTask = std::async(std::launch::async, [command]()
    {
        return std::system(command.c_str());
    });
}

void BuildPanel::StartManagedBuild()
{
    if (m_buildInProgress || m_projectRoot.empty())
    {
        return;
    }

    const std::string projectPath = (std::filesystem::path(m_projectRoot) / "scripts" / "ScriptRuntime" / "ScriptRuntime.csproj").string();
    const std::string outputPath = (std::filesystem::path(m_projectRoot) / "build" / "managed" / "ScriptRuntime").string();
    m_lastTaskName = "Managed Scripts";
    m_lastLogPath = BuildLogPath(m_projectRoot, "managed_scripts");
    m_lastStatus = "Running managed build...";

    const std::string command =
        Quote(m_dotnetExecutable) + " build " + Quote(projectPath) + " -c Release -o " + Quote(outputPath) +
        " > " + Quote(m_lastLogPath) + " 2>&1";

    m_buildInProgress = true;
    m_buildTask = std::async(std::launch::async, [command]()
    {
        return std::system(command.c_str());
    });
}

std::string BuildPanel::DetectProjectRoot()
{
    std::error_code error;
    std::filesystem::path current = std::filesystem::current_path(error);
    if (error)
    {
        return {};
    }

    while (!current.empty())
    {
        if (std::filesystem::exists(current / "CMakeLists.txt"))
        {
            return current.string();
        }

        const std::filesystem::path parent = current.parent_path();
        if (parent == current)
        {
            break;
        }
        current = parent;
    }

    return {};
}

std::string BuildPanel::ReadTail(const std::string& path, const std::size_t maxLines)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        return {};
    }

    std::deque<std::string> lines;
    std::string line;
    while (std::getline(file, line))
    {
        if (lines.size() >= maxLines)
        {
            lines.pop_front();
        }
        lines.push_back(std::move(line));
    }

    std::ostringstream out;
    for (const std::string& item : lines)
    {
        out << item << '\n';
    }
    return out.str();
}

std::string BuildPanel::Quote(const std::string& value)
{
    return "\"" + value + "\"";
}
} // namespace rg::editor

