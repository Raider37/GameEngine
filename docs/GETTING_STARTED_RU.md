# RaiderEngine: быстрый старт

Это краткая точка входа. Подробные материалы:

- индекс docs: `docs/README_RU.md`
- архитектура: `docs/ARCHITECTURE_RU.md`
- развитие движка: `docs/ENGINE_EXTENSIBILITY_RU.md`
- редактор: `docs/EDITOR_GUIDE_RU.md`
- разработка игры: `docs/GAME_DEVELOPMENT_RU.md`

## 1. Что есть “из коробки”

- C++20 runtime: ECS, window/input, renderer abstraction, systems.
- Рендер backend-ы:
  - `DirectX12` (основной рабочий путь);
  - `Vulkan` (заглушка);
  - `Null` (fallback).
- C# script runtime (`scripts/ScriptRuntime`) и мост из C++.
- Модульный editor UI на ImGui (панели + docking layout).
- Voxel sandbox модуль.

## 2. Быстрый запуск (Windows)

Требования:

- Visual Studio 2022 + C++ toolchain + Windows SDK;
- CMake 3.22+;
- .NET SDK 8.0+;
- `third_party/imgui-1.90.9`.

Сборка:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Запуск:

```powershell
.\build\Release\Sandbox.exe
```

Если `cmake` не найден в `PATH`, используйте полный путь до CMake из Visual Studio.

## 3. Базовая настройка проекта

Файл: `src/Sandbox/main.cpp`.

Ключевые флаги:

- `config.renderAPI`
- `config.enableEditorUI`
- `config.enableVoxelSandbox`
- `config.voxelWorldRadiusInChunks`
- `config.voxelWorldSeed`
- `config.fixedDeltaSeconds`

## 4. Минимальный пример кода игры

```cpp
rg::Engine engine(config);
if (!engine.Initialize())
{
    return 1;
}

auto& world = engine.GetWorld();
auto player = world.CreateEntity("Player");
player.Transform().position = rg::Vector3{0.0f, 20.0f, 0.0f};
player.AddComponent<rg::RigidbodyComponent>();
player.AttachScript("MoveForwardScript");

engine.Run();
```

## 5. Voxel sandbox controls

- `W/A/S/D` — движение
- `Space` — прыжок
- `Q` — сломать блок
- `E` — поставить блок
- `F` — сменить блок
- `Esc` — закрыть окно

## 6. Следующий шаг

Если нужно расширять движок: сначала `docs/ARCHITECTURE_RU.md`, затем `docs/ENGINE_EXTENSIBILITY_RU.md`.

Если нужно делать игру: переходите в `docs/GAME_DEVELOPMENT_RU.md`.
