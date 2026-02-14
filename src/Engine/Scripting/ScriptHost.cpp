#include "Engine/Scripting/ScriptHost.h"

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "Engine/Core/Log.h"

namespace rg
{
namespace
{
std::string Quote(const std::filesystem::path& path)
{
    return "\"" + path.string() + "\"";
}

std::vector<std::string> Split(const std::string& line, const char separator)
{
    std::vector<std::string> parts;
    std::stringstream parser(line);
    std::string token;

    while (std::getline(parser, token, separator))
    {
        parts.push_back(token);
    }

    return parts;
}
} // namespace

bool ScriptHost::Initialize()
{
#if defined(RG_DISABLE_SCRIPTING)
    Log::Write(LogLevel::Warning, "Script host disabled at compile time. Build with dotnet available to enable C# scripting.");
    m_enabled = false;
    return false;
#else
    if (!ResolveRuntimePath())
    {
        m_enabled = false;
        return false;
    }

    m_inputPath = std::filesystem::temp_directory_path() / "raider_engine_script_input.txt";
    m_outputPath = std::filesystem::temp_directory_path() / "raider_engine_script_output.txt";
    m_enabled = true;
    Log::Write(LogLevel::Info, "Script host initialized with runtime: " + m_runtimeDllPath.string());
    return true;
#endif
}

void ScriptHost::Tick(World& world, const float deltaSeconds)
{
    if (!m_enabled)
    {
        return;
    }

    bool hasScriptedEntities = false;
    world.ForEach<ScriptComponent, TransformComponent>([&](Entity /*entity*/, const ScriptComponent& script, const TransformComponent& /*transform*/)
    {
        if (script.enabled)
        {
            hasScriptedEntities = true;
        }
    });

    if (!hasScriptedEntities)
    {
        return;
    }

    WriteInput(world, deltaSeconds);

    if (!RunRuntime())
    {
        Log::Write(LogLevel::Warning, "Managed script runtime returned a non-zero exit code.");
        return;
    }

    ReadOutput(world);
}

bool ScriptHost::IsEnabled() const
{
    return m_enabled;
}

bool ScriptHost::ResolveRuntimePath()
{
#if defined(RG_SCRIPT_RUNTIME_DLL)
    m_runtimeDllPath = RG_SCRIPT_RUNTIME_DLL;
    if (std::filesystem::exists(m_runtimeDllPath))
    {
        return true;
    }
#endif

    const auto fallbackA = std::filesystem::current_path() / "managed/ScriptRuntime/ScriptRuntime.dll";
    if (std::filesystem::exists(fallbackA))
    {
        m_runtimeDllPath = fallbackA;
        return true;
    }

    const auto fallbackB = std::filesystem::current_path() / "../managed/ScriptRuntime/ScriptRuntime.dll";
    if (std::filesystem::exists(fallbackB))
    {
        m_runtimeDllPath = fallbackB;
        return true;
    }

    Log::Write(LogLevel::Warning, "Script runtime DLL was not found. Build target ManagedScripts first.");
    return false;
}

void ScriptHost::WriteInput(const World& world, const float deltaSeconds) const
{
    std::ofstream file(m_inputPath, std::ios::trunc);
    file << std::fixed << std::setprecision(6) << deltaSeconds << '\n';

    world.ForEach<ScriptComponent, TransformComponent>([&](Entity entity, const ScriptComponent& script, const TransformComponent& transform)
    {
        if (!script.enabled)
        {
            return;
        }

        file << entity.GetId() << '|'
             << script.className << '|'
             << std::fixed << std::setprecision(6)
             << transform.position.x << '|'
             << transform.position.y << '|'
             << transform.position.z << '\n';
    });
}

void ScriptHost::ReadOutput(World& world) const
{
    std::ifstream file(m_outputPath);
    if (!file.is_open())
    {
        Log::Write(LogLevel::Warning, "Managed runtime did not produce an output file.");
        return;
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty())
        {
            continue;
        }

        const auto parts = Split(line, '|');
        if (parts.size() != 4)
        {
            continue;
        }

        try
        {
            const auto id = static_cast<Entity::Id>(std::stoul(parts[0]));
            const float x = std::stof(parts[1]);
            const float y = std::stof(parts[2]);
            const float z = std::stof(parts[3]);

            Entity entity = world.FindEntity(id);
            if (entity.IsValid())
            {
                entity.Transform().position = Vector3 {x, y, z};
            }
        }
        catch (...)
        {
            // Invalid script runtime line. Ignore it and continue processing.
        }
    }
}

bool ScriptHost::RunRuntime() const
{
    const std::string command = "dotnet " + Quote(m_runtimeDllPath) + " " + Quote(m_inputPath) + " " + Quote(m_outputPath);
    return std::system(command.c_str()) == 0;
}
} // namespace rg
