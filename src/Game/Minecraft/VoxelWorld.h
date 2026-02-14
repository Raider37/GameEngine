#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <unordered_map>
#include <vector>

#include "Engine/Math/Vector3.h"

namespace rg::minecraft
{
enum class BlockType : std::uint8_t
{
    Air = 0,
    Grass = 1,
    Dirt = 2,
    Stone = 3,
    Sand = 4,
    Wood = 5,
    Leaves = 6
};

[[nodiscard]] bool IsSolid(BlockType type);
[[nodiscard]] const char* ToString(BlockType type);

struct BlockHit
{
    bool hit = false;
    int x = 0;
    int y = 0;
    int z = 0;
    int normalX = 0;
    int normalY = 0;
    int normalZ = 0;
    float distance = 0.0f;
    BlockType block = BlockType::Air;
};

class VoxelWorld
{
public:
    static constexpr int kChunkSize = 16;
    static constexpr int kWorldHeight = 64;

    void Generate(int radiusInChunks, int seed);

    [[nodiscard]] BlockType GetBlock(int x, int y, int z) const;
    [[nodiscard]] bool SetBlock(int x, int y, int z, BlockType type);
    [[nodiscard]] int SurfaceHeight(int x, int z) const;
    [[nodiscard]] bool Raycast(const Vector3& origin, const Vector3& direction, float maxDistance, BlockHit& outHit) const;

    [[nodiscard]] std::size_t LoadedChunkCount() const;
    [[nodiscard]] std::vector<std::pair<int, int>> ChunkCoordinates() const;
    [[nodiscard]] std::uint64_t Revision() const;
    [[nodiscard]] int RadiusInChunks() const;
    [[nodiscard]] int Seed() const;

    [[nodiscard]] int MinWorldX() const;
    [[nodiscard]] int MaxWorldX() const;
    [[nodiscard]] int MinWorldZ() const;
    [[nodiscard]] int MaxWorldZ() const;

private:
    struct ChunkCoord
    {
        int x = 0;
        int z = 0;

        [[nodiscard]] bool operator==(const ChunkCoord& other) const
        {
            return (x == other.x) && (z == other.z);
        }
    };

    struct ChunkCoordHasher
    {
        [[nodiscard]] std::size_t operator()(const ChunkCoord& coord) const;
    };

    struct Chunk
    {
        std::vector<std::uint8_t> blocks;
    };

    [[nodiscard]] static int FloorDiv(int value, int divisor);
    [[nodiscard]] static int PositiveMod(int value, int divisor);
    [[nodiscard]] static std::size_t Index(int localX, int y, int localZ);
    [[nodiscard]] static std::uint32_t Hash2D(int x, int z, int seed);

    [[nodiscard]] const Chunk* FindChunk(int chunkX, int chunkZ) const;
    [[nodiscard]] Chunk* FindChunk(int chunkX, int chunkZ);
    Chunk& EnsureChunk(int chunkX, int chunkZ);
    void GenerateChunk(const ChunkCoord& coord, Chunk& chunk);
    [[nodiscard]] int ComputeTerrainHeight(int worldX, int worldZ) const;

    int m_radiusInChunks = 0;
    int m_seed = 1337;
    std::uint64_t m_revision = 0;
    std::unordered_map<ChunkCoord, Chunk, ChunkCoordHasher> m_chunks;
};
} // namespace rg::minecraft
