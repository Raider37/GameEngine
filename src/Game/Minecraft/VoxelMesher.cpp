#include "Game/Minecraft/VoxelMesher.h"

#include <algorithm>
#include <array>

namespace rg::minecraft
{
namespace
{
struct MaskCell
{
    BlockType block = BlockType::Air;
    int normal = 0;

    [[nodiscard]] bool operator==(const MaskCell& other) const
    {
        return (block == other.block) && (normal == other.normal);
    }
};

[[nodiscard]] bool IsInside(const std::array<int, 3>& p, const std::array<int, 3>& dims)
{
    return (p[0] >= 0) && (p[0] < dims[0]) &&
           (p[1] >= 0) && (p[1] < dims[1]) &&
           (p[2] >= 0) && (p[2] < dims[2]);
}
} // namespace

std::uint32_t BlockColor(const BlockType type)
{
    switch (type)
    {
    case BlockType::Grass:
        return 0xff4caa57u;
    case BlockType::Dirt:
        return 0xff705338u;
    case BlockType::Stone:
        return 0xff90939au;
    case BlockType::Sand:
        return 0xffd7ca89u;
    case BlockType::Wood:
        return 0xff7f5a30u;
    case BlockType::Leaves:
        return 0xff5a9648u;
    case BlockType::Air:
    default:
        return 0x00000000u;
    }
}

VoxelChunkMesh VoxelMesher::BuildChunkMesh(const VoxelWorld& world, const int chunkX, const int chunkZ)
{
    VoxelChunkMesh mesh;
    mesh.chunkX = chunkX;
    mesh.chunkZ = chunkZ;

    const std::array<int, 3> dims {VoxelWorld::kChunkSize, VoxelWorld::kWorldHeight, VoxelWorld::kChunkSize};
    const int baseX = chunkX * VoxelWorld::kChunkSize;
    const int baseZ = chunkZ * VoxelWorld::kChunkSize;

    auto sampleBlock = [&](const std::array<int, 3>& local) -> BlockType
    {
        return world.GetBlock(baseX + local[0], local[1], baseZ + local[2]);
    };

    for (int d = 0; d < 3; ++d)
    {
        const int u = (d + 1) % 3;
        const int v = (d + 2) % 3;

        std::array<int, 3> x {0, 0, 0};
        std::array<int, 3> q {0, 0, 0};
        q[d] = 1;

        std::vector<MaskCell> mask(static_cast<std::size_t>(dims[u] * dims[v]));

        for (x[d] = -1; x[d] < dims[d];)
        {
            std::size_t n = 0;
            for (x[v] = 0; x[v] < dims[v]; ++x[v])
            {
                for (x[u] = 0; x[u] < dims[u]; ++x[u])
                {
                    const std::array<int, 3> a = x;
                    const std::array<int, 3> b = {x[0] + q[0], x[1] + q[1], x[2] + q[2]};

                    const BlockType blockA = sampleBlock(a);
                    const BlockType blockB = sampleBlock(b);
                    const bool aSolid = IsSolid(blockA);
                    const bool bSolid = IsSolid(blockB);

                    MaskCell cell;
                    if (aSolid && !bSolid && IsInside(a, dims))
                    {
                        cell.block = blockA;
                        cell.normal = 1;
                    }
                    else if (!aSolid && bSolid && IsInside(b, dims))
                    {
                        cell.block = blockB;
                        cell.normal = -1;
                    }

                    mask[n++] = cell;
                }
            }

            ++x[d];
            n = 0;

            for (int j = 0; j < dims[v]; ++j)
            {
                for (int i = 0; i < dims[u];)
                {
                    const MaskCell cell = mask[n];
                    if (cell.normal == 0)
                    {
                        ++i;
                        ++n;
                        continue;
                    }

                    int w = 1;
                    while ((i + w < dims[u]) && (mask[n + static_cast<std::size_t>(w)] == cell))
                    {
                        ++w;
                    }

                    int h = 1;
                    bool done = false;
                    while ((j + h) < dims[v] && !done)
                    {
                        for (int k = 0; k < w; ++k)
                        {
                            if (!(mask[n + static_cast<std::size_t>(k) + static_cast<std::size_t>(h * dims[u])] == cell))
                            {
                                done = true;
                                break;
                            }
                        }

                        if (!done)
                        {
                            ++h;
                        }
                    }

                    x[u] = i;
                    x[v] = j;

                    std::array<int, 3> du {0, 0, 0};
                    std::array<int, 3> dv {0, 0, 0};
                    du[u] = w;
                    dv[v] = h;

                    const std::array<int, 3> p0 {x[0], x[1], x[2]};
                    const std::array<int, 3> p1 {x[0] + du[0], x[1] + du[1], x[2] + du[2]};
                    const std::array<int, 3> p2 {x[0] + du[0] + dv[0], x[1] + du[1] + dv[1], x[2] + du[2] + dv[2]};
                    const std::array<int, 3> p3 {x[0] + dv[0], x[1] + dv[1], x[2] + dv[2]};

                    const std::uint32_t color = BlockColor(cell.block);
                    const std::uint32_t baseIndex = static_cast<std::uint32_t>(mesh.vertices.size());

                    auto pushVertex = [&](const std::array<int, 3>& p)
                    {
                        mesh.vertices.push_back(VoxelVertex {
                            static_cast<float>(baseX + p[0]),
                            static_cast<float>(p[1]),
                            static_cast<float>(baseZ + p[2]),
                            color});
                    };

                    pushVertex(p0);
                    pushVertex(p1);
                    pushVertex(p2);
                    pushVertex(p3);

                    if (cell.normal > 0)
                    {
                        mesh.indices.push_back(baseIndex + 0U);
                        mesh.indices.push_back(baseIndex + 1U);
                        mesh.indices.push_back(baseIndex + 2U);
                        mesh.indices.push_back(baseIndex + 0U);
                        mesh.indices.push_back(baseIndex + 2U);
                        mesh.indices.push_back(baseIndex + 3U);
                    }
                    else
                    {
                        mesh.indices.push_back(baseIndex + 0U);
                        mesh.indices.push_back(baseIndex + 2U);
                        mesh.indices.push_back(baseIndex + 1U);
                        mesh.indices.push_back(baseIndex + 0U);
                        mesh.indices.push_back(baseIndex + 3U);
                        mesh.indices.push_back(baseIndex + 2U);
                    }

                    for (int l = 0; l < h; ++l)
                    {
                        for (int k = 0; k < w; ++k)
                        {
                            mask[n + static_cast<std::size_t>(k) + static_cast<std::size_t>(l * dims[u])] = MaskCell {};
                        }
                    }

                    i += w;
                    n += static_cast<std::size_t>(w);
                }
            }
        }
    }

    if (mesh.vertices.empty())
    {
        mesh.boundsMin = Vector3 {static_cast<float>(baseX), 0.0f, static_cast<float>(baseZ)};
        mesh.boundsMax = Vector3 {
            static_cast<float>(baseX + VoxelWorld::kChunkSize),
            static_cast<float>(VoxelWorld::kWorldHeight),
            static_cast<float>(baseZ + VoxelWorld::kChunkSize)};
        return mesh;
    }

    Vector3 minV {mesh.vertices.front().x, mesh.vertices.front().y, mesh.vertices.front().z};
    Vector3 maxV = minV;
    for (const auto& vertex : mesh.vertices)
    {
        minV.x = std::min(minV.x, vertex.x);
        minV.y = std::min(minV.y, vertex.y);
        minV.z = std::min(minV.z, vertex.z);
        maxV.x = std::max(maxV.x, vertex.x);
        maxV.y = std::max(maxV.y, vertex.y);
        maxV.z = std::max(maxV.z, vertex.z);
    }

    mesh.boundsMin = minV;
    mesh.boundsMax = maxV;
    return mesh;
}
} // namespace rg::minecraft
