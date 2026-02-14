#include "Engine/Resources/ResourceManager.h"

#include <fstream>
#include <iterator>
#include <utility>

namespace rg
{
void ResourceManager::SetRoot(std::filesystem::path root)
{
    m_root = std::move(root);
}

const std::filesystem::path& ResourceManager::Root() const
{
    return m_root;
}

bool ResourceManager::Exists(const std::string& relativePath) const
{
    return std::filesystem::exists(Resolve(relativePath));
}

std::optional<std::string> ResourceManager::LoadText(const std::string& relativePath) const
{
    const auto cacheIt = m_textCache.find(relativePath);
    if (cacheIt != m_textCache.end())
    {
        return cacheIt->second;
    }

    std::ifstream file(Resolve(relativePath), std::ios::in);
    if (!file.is_open())
    {
        return std::nullopt;
    }

    const std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    m_textCache.emplace(relativePath, text);
    return text;
}

std::optional<std::vector<std::uint8_t>> ResourceManager::LoadBinary(const std::string& relativePath) const
{
    const auto cacheIt = m_binaryCache.find(relativePath);
    if (cacheIt != m_binaryCache.end())
    {
        return cacheIt->second;
    }

    std::ifstream file(Resolve(relativePath), std::ios::binary);
    if (!file.is_open())
    {
        return std::nullopt;
    }

    const std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    m_binaryCache.emplace(relativePath, data);
    return data;
}

void ResourceManager::ClearCache()
{
    m_textCache.clear();
    m_binaryCache.clear();
}

std::filesystem::path ResourceManager::Resolve(const std::string& relativePath) const
{
    return m_root / relativePath;
}
} // namespace rg
