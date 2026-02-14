# RaiderEngine (CryEngine-like prototype)

This repository contains a game-engine prototype with:

- C++ core runtime (engine loop, ECS world, resources, input/window, renderer backends, scripting host)
- C# gameplay scripting runtime
- CMake build for native side and managed build integration for scripts

## Documentation

- Docs index (RU): `docs/README_RU.md`
- Quick start (RU): `docs/GETTING_STARTED_RU.md`
- Architecture (RU): `docs/ARCHITECTURE_RU.md`
- Engine extensibility (RU): `docs/ENGINE_EXTENSIBILITY_RU.md`
- Editor guide (RU): `docs/EDITOR_GUIDE_RU.md`
- Game development guide (RU): `docs/GAME_DEVELOPMENT_RU.md`

## Architecture

- `src/Engine/Core`: logging and core utilities
- `src/Engine/ECS`: entity-component storage (`Registry`)
- `src/Engine/Scene`: high-level entity/world API on top of ECS
- `src/Engine/Resources`: asset loading and caching
- `src/Engine/Input`: key state tracking
- `src/Engine/Platform`: window abstraction with `Win32` backend and `Null` fallback backend
- `src/Engine/Rendering`: renderer abstraction with backends:
  - `Null`
  - `DirectX12` (real runtime initialization: device/queue/swapchain/RTV/fence)
  - `Vulkan` (stub backend scaffold)
- `src/Engine/Scripting`: C++ host that invokes managed C# script runtime
- `src/Engine/Systems`: ECS-driven systems (`VoxelGameplay`, `Script`, `Physics`, `Render`)
- `src/Game/Minecraft`: voxel world, terrain generation, block access/raycast
- `src/Editor`: modular editor UI layer (`Viewport`, `Scene Objects`, `Inspector`, `World Settings`, `Level Classes`, `Content Browser`, `Stats`, `Voxel World`, `Build`)
- `scripts/ScriptRuntime`: C# runtime that executes script behaviors
- `src/Sandbox`: sample project using the engine
- `assets/shaders`: shader assets loaded by renderer backends
- `third_party/imgui-1.90.9`: Dear ImGui sources used by editor UI

The scripting flow per frame:

1. C++ writes scripted entities into a temp input file.
2. C++ runs `dotnet ScriptRuntime.dll <input> <output>`.
3. C# applies script logic and writes new transforms to output.
4. C++ reads output and updates scene transforms.

Renderer flow:

1. Engine selects API (`Null`, `DirectX12`, or `Vulkan`) from `EngineConfig`.
2. Renderer creates backend implementation.
3. Backend loads relevant shader assets from `ResourceManager`.
4. `DirectX12` path initializes GPU objects (device, command queue, swapchain, RTV heap, command list, fence).
5. Each frame executes command list, clears backbuffer, and presents.
6. If editor UI is enabled, ImGui panels are rendered into the same frame command list.

System flow per frame:

1. `VoxelGameplaySystem` updates player movement and block interactions.
2. `ScriptSystem` updates scripted entities (C# runtime bridge).
3. `PhysicsSystem` integrates rigid bodies over ECS components.
4. `RenderSystem` submits the current world to the active renderer backend.

Editor UI is panel-based:

1. `PanelRegistry` controls panel lifecycle and visibility.
2. Each panel is an isolated module implementing `IEditorPanel`.
3. Built-in panels: viewport, scene objects, inspector, world settings, level classes, content browser, runtime stats, voxel world, build.

Voxel sandbox controls:

1. `W/A/S/D` move
2. `Space` jump
3. `Q` break block
4. `E` place block
5. `F` switch selected block

## Build

Requirements:

- CMake 3.22+
- C++20 compiler (MSVC, Clang, GCC)
- .NET SDK 8.0+ (`dotnet`)
- For DirectX12 backend: Windows SDK with `d3d12`/`dxgi` libraries
- Dear ImGui sources present at `third_party/imgui-1.90.9`

Configure and build:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Run sandbox:

```powershell
.\build\Release\Sandbox.exe
```

If `dotnet` is not found, engine still builds, but C# scripting is disabled.

## Status

- ECS, resources, Win32 window/input, and multi-backend renderer architecture are implemented.
- DirectX12 backend performs real swapchain-based rendering initialization and present path.
- Editor UI is modular and rendered via Dear ImGui on Win32 + DirectX12.
- Vulkan backend remains a scaffold path in this revision.
