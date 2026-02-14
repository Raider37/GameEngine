#pragma once

#include <filesystem>

#include "Engine/Scene/World.h"

namespace rg
{
class ScriptHost
{
public:
    bool Initialize();
    void Tick(World& world, float deltaSeconds);

    [[nodiscard]] bool IsEnabled() const;

private:
    bool ResolveRuntimePath();
    void WriteInput(const World& world, float deltaSeconds) const;
    void ReadOutput(World& world) const;
    bool RunRuntime() const;

    bool m_enabled = false;
    std::filesystem::path m_runtimeDllPath;
    std::filesystem::path m_inputPath;
    std::filesystem::path m_outputPath;
};
} // namespace rg
