#pragma once

#include <cstdint>
#include <vector>

#include "Engine/Math/Vector3.h"
#include "Game/Minecraft/VoxelWorld.h"

namespace rg::minecraft
{
struct VoxelVertex
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    std::uint32_t color = 0xffffffffu;
};

struct VoxelChunkMesh
{
    int chunkX = 0;
    int chunkZ = 0;
    std::vector<VoxelVertex> vertices;
    std::vector<std::uint32_t> indices;
    Vector3 boundsMin {};
    Vector3 boundsMax {};
};

[[nodiscard]] std::uint32_t BlockColor(BlockType type);

class VoxelMesher
{
public:
    [[nodiscard]] static VoxelChunkMesh BuildChunkMesh(const VoxelWorld& world, int chunkX, int chunkZ);
};
} // namespace rg::minecraft
