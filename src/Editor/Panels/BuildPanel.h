#pragma once

#include <future>
#include <string>

#include "Editor/Panels/IEditorPanel.h"

namespace rg::editor
{
class BuildPanel final : public IEditorPanel
{
public:
    [[nodiscard]] const char* Name() const override;
    void Draw(EditorContext& context) override;

private:
    void EnsureToolsResolved();
    void PollBuildTask();
    void StartNativeBuild(const char* configName);
    void StartManagedBuild();

    [[nodiscard]] static std::string DetectProjectRoot();
    [[nodiscard]] static std::string ReadTail(const std::string& path, std::size_t maxLines);
    [[nodiscard]] static std::string Quote(const std::string& value);

    std::string m_projectRoot;
    std::string m_cmakeExecutable;
    std::string m_dotnetExecutable = "dotnet";
    std::string m_lastStatus = "Idle.";
    std::string m_lastLogPath;
    std::string m_lastTaskName;
    int m_lastExitCode = 0;
    bool m_buildInProgress = false;
    std::future<int> m_buildTask;
};
} // namespace rg::editor

