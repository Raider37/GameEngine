#include "Engine/Engine.h"
#include "Engine/Rendering/RenderAPI.h"

int main()
{
    rg::EngineConfig config;
    config.projectName = "VoxelCraftPrototype";
    config.window.title = "VoxelCraft Prototype";
    config.window.width = 1920;
    config.window.height = 1080;
    config.renderAPI = rg::RenderAPI::DirectX12;
    config.enableEditorUI = true;
    config.enableVoxelSandbox = true;
    config.voxelWorldRadiusInChunks = 10;
    config.voxelWorldSeed = 4201337;
    config.maxFrames = 1000000;
    config.fixedDeltaSeconds = 1.0f / 60.0f;

    rg::Engine engine(config);
    if (!engine.Initialize())
    {
        return 1;
    }

    engine.Run();
    return 0;
}
