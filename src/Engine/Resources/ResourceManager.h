#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace rg
{
class ResourceManager
{
public:
    void SetRoot(std::filesystem::path root);

    [[nodiscard]] const std::filesystem::path& Root() const;
    [[nodiscard]] bool Exists(const std::string& relativePath) const;

    [[nodiscard]] std::optional<std::string> LoadText(const std::string& relativePath) const;
    [[nodiscard]] std::optional<std::vector<std::uint8_t>> LoadBinary(const std::string& relativePath) const;

    void ClearCache();

private:
    [[nodiscard]] std::filesystem::path Resolve(const std::string& relativePath) const;

    std::filesystem::path m_root = "assets";
    mutable std::unordered_map<std::string, std::string> m_textCache;
    mutable std::unordered_map<std::string, std::vector<std::uint8_t>> m_binaryCache;
};
} // namespace rg
