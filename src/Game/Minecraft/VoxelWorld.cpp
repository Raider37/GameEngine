#include "Game/Minecraft/VoxelWorld.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace rg::minecraft
{
namespace
{
constexpr float kRayStep = 0.1f;
}

bool IsSolid(const BlockType type)
{
    return type != BlockType::Air;
}

const char* ToString(const BlockType type)
{
    switch (type)
    {
    case BlockType::Air:
        return "Air";
    case BlockType::Grass:
        return "Grass";
    case BlockType::Dirt:
        return "Dirt";
    case BlockType::Stone:
        return "Stone";
    case BlockType::Sand:
        return "Sand";
    case BlockType::Wood:
        return "Wood";
    case BlockType::Leaves:
        return "Leaves";
    default:
        return "Unknown";
    }
}

void VoxelWorld::Generate(const int radiusInChunks, const int seed)
{
    m_radiusInChunks = std::max(1, radiusInChunks);
    m_seed = seed;
    m_chunks.clear();

    for (int chunkZ = -m_radiusInChunks; chunkZ <= m_radiusInChunks; ++chunkZ)
    {
        for (int chunkX = -m_radiusInChunks; chunkX <= m_radiusInChunks; ++chunkX)
        {
            Chunk& chunk = EnsureChunk(chunkX, chunkZ);
            GenerateChunk(ChunkCoord {chunkX, chunkZ}, chunk);
        }
    }

    ++m_revision;
}

BlockType VoxelWorld::GetBlock(const int x, const int y, const int z) const
{
    if ((y < 0) || (y >= kWorldHeight))
    {
        return BlockType::Air;
    }

    const int chunkX = FloorDiv(x, kChunkSize);
    const int chunkZ = FloorDiv(z, kChunkSize);
    const Chunk* chunk = FindChunk(chunkX, chunkZ);
    if (chunk == nullptr)
    {
        return BlockType::Air;
    }

    const int localX = PositiveMod(x, kChunkSize);
    const int localZ = PositiveMod(z, kChunkSize);
    return static_cast<BlockType>(chunk->blocks[Index(localX, y, localZ)]);
}

bool VoxelWorld::SetBlock(const int x, const int y, const int z, const BlockType type)
{
    if ((y < 0) || (y >= kWorldHeight))
    {
        return false;
    }

    const int chunkX = FloorDiv(x, kChunkSize);
    const int chunkZ = FloorDiv(z, kChunkSize);
    Chunk& chunk = EnsureChunk(chunkX, chunkZ);

    const int localX = PositiveMod(x, kChunkSize);
    const int localZ = PositiveMod(z, kChunkSize);
    const std::size_t index = Index(localX, y, localZ);
    const std::uint8_t value = static_cast<std::uint8_t>(type);

    if (chunk.blocks[index] == value)
    {
        return false;
    }

    chunk.blocks[index] = value;
    ++m_revision;
    return true;
}

int VoxelWorld::SurfaceHeight(const int x, const int z) const
{
    for (int y = kWorldHeight - 1; y >= 0; --y)
    {
        if (IsSolid(GetBlock(x, y, z)))
        {
            return y;
        }
    }
    return 0;
}

bool VoxelWorld::Raycast(const Vector3& origin, const Vector3& direction, const float maxDistance, BlockHit& outHit) const
{
    const float directionLength = direction.Length();
    if ((directionLength < 0.0001f) || (maxDistance <= 0.0f))
    {
        outHit = {};
        return false;
    }

    const float invLength = 1.0f / directionLength;
    const float dirX = direction.x * invLength;
    const float dirY = direction.y * invLength;
    const float dirZ = direction.z * invLength;

    int previousX = static_cast<int>(std::floor(origin.x));
    int previousY = static_cast<int>(std::floor(origin.y));
    int previousZ = static_cast<int>(std::floor(origin.z));

    for (float t = 0.0f; t <= maxDistance; t += kRayStep)
    {
        const float px = origin.x + (dirX * t);
        const float py = origin.y + (dirY * t);
        const float pz = origin.z + (dirZ * t);

        const int x = static_cast<int>(std::floor(px));
        const int y = static_cast<int>(std::floor(py));
        const int z = static_cast<int>(std::floor(pz));

        const BlockType block = GetBlock(x, y, z);
        if (block != BlockType::Air)
        {
            outHit.hit = true;
            outHit.x = x;
            outHit.y = y;
            outHit.z = z;
            outHit.block = block;
            outHit.distance = t;

            outHit.normalX = previousX - x;
            outHit.normalY = previousY - y;
            outHit.normalZ = previousZ - z;

            if ((outHit.normalX == 0) && (outHit.normalY == 0) && (outHit.normalZ == 0))
            {
                outHit.normalY = 1;
            }
            else
            {
                outHit.normalX = std::clamp(outHit.normalX, -1, 1);
                outHit.normalY = std::clamp(outHit.normalY, -1, 1);
                outHit.normalZ = std::clamp(outHit.normalZ, -1, 1);
            }

            return true;
        }

        previousX = x;
        previousY = y;
        previousZ = z;
    }

    outHit = {};
    return false;
}

std::size_t VoxelWorld::LoadedChunkCount() const
{
    return m_chunks.size();
}

std::vector<std::pair<int, int>> VoxelWorld::ChunkCoordinates() const
{
    std::vector<std::pair<int, int>> out;
    out.reserve(m_chunks.size());
    for (const auto& entry : m_chunks)
    {
        out.emplace_back(entry.first.x, entry.first.z);
    }
    return out;
}

std::uint64_t VoxelWorld::Revision() const
{
    return m_revision;
}

int VoxelWorld::RadiusInChunks() const
{
    return m_radiusInChunks;
}

int VoxelWorld::Seed() const
{
    return m_seed;
}

int VoxelWorld::MinWorldX() const
{
    return -m_radiusInChunks * kChunkSize;
}

int VoxelWorld::MaxWorldX() const
{
    return ((m_radiusInChunks + 1) * kChunkSize) - 1;
}

int VoxelWorld::MinWorldZ() const
{
    return -m_radiusInChunks * kChunkSize;
}

int VoxelWorld::MaxWorldZ() const
{
    return ((m_radiusInChunks + 1) * kChunkSize) - 1;
}

std::size_t VoxelWorld::ChunkCoordHasher::operator()(const ChunkCoord& coord) const
{
    const std::uint64_t ux = static_cast<std::uint32_t>(coord.x);
    const std::uint64_t uz = static_cast<std::uint32_t>(coord.z);
    return static_cast<std::size_t>((ux << 32U) ^ uz);
}

int VoxelWorld::FloorDiv(const int value, const int divisor)
{
    int quotient = value / divisor;
    const int remainder = value % divisor;
    if ((remainder != 0) && ((remainder < 0) != (divisor < 0)))
    {
        --quotient;
    }
    return quotient;
}

int VoxelWorld::PositiveMod(const int value, const int divisor)
{
    const int mod = value % divisor;
    return (mod < 0) ? (mod + divisor) : mod;
}

std::size_t VoxelWorld::Index(const int localX, const int y, const int localZ)
{
    return static_cast<std::size_t>((y * kChunkSize * kChunkSize) + (localZ * kChunkSize) + localX);
}

std::uint32_t VoxelWorld::Hash2D(const int x, const int z, const int seed)
{
    std::uint32_t h = static_cast<std::uint32_t>(seed);
    h ^= static_cast<std::uint32_t>(x) * 0x45d9f3bu;
    h ^= static_cast<std::uint32_t>(z) * 0x119de1f3u;
    h ^= h >> 16U;
    h *= 0x7feb352du;
    h ^= h >> 15U;
    h *= 0x846ca68bu;
    h ^= h >> 16U;
    return h;
}

const VoxelWorld::Chunk* VoxelWorld::FindChunk(const int chunkX, const int chunkZ) const
{
    const auto it = m_chunks.find(ChunkCoord {chunkX, chunkZ});
    return (it != m_chunks.end()) ? &it->second : nullptr;
}

VoxelWorld::Chunk* VoxelWorld::FindChunk(const int chunkX, const int chunkZ)
{
    const auto it = m_chunks.find(ChunkCoord {chunkX, chunkZ});
    return (it != m_chunks.end()) ? &it->second : nullptr;
}

VoxelWorld::Chunk& VoxelWorld::EnsureChunk(const int chunkX, const int chunkZ)
{
    const ChunkCoord coord {chunkX, chunkZ};
    const auto it = m_chunks.find(coord);
    if (it != m_chunks.end())
    {
        return it->second;
    }

    Chunk chunk;
    chunk.blocks.assign(static_cast<std::size_t>(kChunkSize * kWorldHeight * kChunkSize), static_cast<std::uint8_t>(BlockType::Air));
    auto [inserted, ignored] = m_chunks.emplace(coord, std::move(chunk));
    (void)ignored;
    return inserted->second;
}

void VoxelWorld::GenerateChunk(const ChunkCoord& coord, Chunk& chunk)
{
    for (int localZ = 0; localZ < kChunkSize; ++localZ)
    {
        for (int localX = 0; localX < kChunkSize; ++localX)
        {
            const int worldX = (coord.x * kChunkSize) + localX;
            const int worldZ = (coord.z * kChunkSize) + localZ;
            const int surface = ComputeTerrainHeight(worldX, worldZ);

            for (int y = 0; y < kWorldHeight; ++y)
            {
                BlockType block = BlockType::Air;
                if (y <= surface)
                {
                    if (y == surface)
                    {
                        block = (surface < 8) ? BlockType::Sand : BlockType::Grass;
                    }
                    else if (y >= (surface - 2))
                    {
                        block = BlockType::Dirt;
                    }
                    else
                    {
                        block = BlockType::Stone;
                    }
                }

                chunk.blocks[Index(localX, y, localZ)] = static_cast<std::uint8_t>(block);
            }

            const std::uint32_t treeNoise = Hash2D(worldX, worldZ, m_seed + 91);
            if ((surface > 10) && ((treeNoise % 97U) == 0U))
            {
                for (int trunk = 1; trunk <= 4; ++trunk)
                {
                    const int ty = surface + trunk;
                    if (ty < kWorldHeight)
                    {
                        const bool changed = SetBlock(worldX, ty, worldZ, BlockType::Wood);
                        (void)changed;
                    }
                }

                for (int oz = -2; oz <= 2; ++oz)
                {
                    for (int ox = -2; ox <= 2; ++ox)
                    {
                        for (int oy = 3; oy <= 5; ++oy)
                        {
                            if ((std::abs(ox) + std::abs(oz) + std::abs(oy - 4)) > 4)
                            {
                                continue;
                            }

                            const int ly = surface + oy;
                            if (ly >= kWorldHeight)
                            {
                                continue;
                            }

                            if (GetBlock(worldX + ox, ly, worldZ + oz) == BlockType::Air)
                            {
                                const bool changed = SetBlock(worldX + ox, ly, worldZ + oz, BlockType::Leaves);
                                (void)changed;
                            }
                        }
                    }
                }
            }
        }
    }
}

int VoxelWorld::ComputeTerrainHeight(const int worldX, const int worldZ) const
{
    const float fx = static_cast<float>(worldX);
    const float fz = static_cast<float>(worldZ);

    const float hills = std::sin(fx * 0.11f) * 4.0f;
    const float ridges = std::cos(fz * 0.09f) * 5.0f;
    const float rolling = std::sin((fx + fz) * 0.045f) * 3.0f;
    const float noise = static_cast<float>(Hash2D(worldX, worldZ, m_seed) & 0xFFu) / 255.0f;

    float height = 18.0f + hills + ridges + rolling + ((noise - 0.5f) * 4.0f);
    height = std::clamp(height, 2.0f, static_cast<float>(kWorldHeight - 2));
    return static_cast<int>(height);
}
} // namespace rg::minecraft
