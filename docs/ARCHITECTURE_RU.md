# Архитектура RaiderEngine

Документ описывает текущую архитектуру проекта и связи между ключевыми подсистемами.

## 1. Высокоуровневая схема

RaiderEngine состоит из:

- `Engine` (инициализация и главный цикл);
- `World` + `Registry` (ECS-данные уровня);
- набора `Systems` (логика кадра);
- `Renderer` + backend (`DirectX12` / `Vulkan` / `Null`);
- `ScriptHost` (мост к C# runtime);
- `EditorUI` (модульный ImGui-редактор);
- игрового модуля `VoxelWorld` + `VoxelMesher`.

Поток кадра:

1. Опрос ввода и оконных событий.
2. Последовательный вызов ECS-систем.
3. Рендер мира + editor UI.
4. Present.

## 2. Структура исходников

- `src/Engine/Core` — логирование.
- `src/Engine/ECS` — хранилище сущностей/компонентов (`Registry`).
- `src/Engine/Scene` — API `World`/`Entity` поверх ECS.
- `src/Engine/Systems` — системы кадра.
- `src/Engine/Rendering` — выбор API и backend.
- `src/Engine/Platform` — окно/события (Win32 + fallback).
- `src/Engine/Input` — состояния клавиш.
- `src/Engine/Resources` — доступ к assets и кеш.
- `src/Engine/Scripting` — C++ -> C# мост.
- `src/Game/Minecraft` — voxel-мир и meshing.
- `src/Editor` — editor UI и панели.
- `src/Sandbox` — пример проекта/точка входа.

## 3. Инициализация движка

Основная конфигурация задается в `EngineConfig` (`src/Engine/Engine.h`):

- окно (`WindowSpec`);
- API рендера (`RenderAPI`);
- editor on/off;
- voxel sandbox on/off и параметры мира;
- корень assets;
- лимит кадров и fixed delta.

Инициализация (`Engine::Initialize`, `src/Engine/Engine.cpp`) идет в таком порядке:

1. `ResourceManager::SetRoot`.
2. `WindowSystem::Initialize`.
3. `Renderer::Initialize`.
4. `ScriptHost::Initialize`.
5. Генерация voxel мира (если включен).
6. `EditorUI::Initialize` (если включен).
7. Регистрация систем.
8. `Initialize()` каждой системы.

Если любой критичный этап падает, `Initialize()` возвращает `false`.

## 4. Главный цикл

`Engine::Run` выполняется пока:

- не превышен `maxFrames`;
- окно не запросило закрытие.

На каждой итерации:

1. `input.BeginFrame()`.
2. `window.PollEvents(input)`.
3. `Escape` закрывает окно.
4. `Engine::Update()`.
5. Sleep ~16ms (базовый throttle).

`Engine::Update()`:

- формирует `SystemContext`;
- вызывает `Update()` систем по порядку;
- проверяет `EditorUI::ConsumeCloseRequest()` (выход из editor меню);
- увеличивает индекс кадра.

## 5. ECS: `Registry`, `World`, `Entity`

### 5.1 Registry

`src/Engine/ECS/Registry.h`:

- entity id (`uint32_t`);
- хранение компонентных store-ов через type-erasure;
- `AddComponent`, `GetComponent`, `HasComponent`, `RemoveComponent`;
- `ForEach<Components...>` по живым entity.

Принцип: данные принадлежат `Registry`, а `Entity` — это удобный handle.

### 5.2 World

`src/Engine/Scene/World.h`:

- создание/удаление сущностей;
- перечисление сущностей;
- доступ к `Registry`;
- `ForEach`-обертка, которая передает `Entity` + компоненты.

### 5.3 Entity

`src/Engine/Scene/Entity.h`:

- имя и transform;
- скрипт и mesh convenience API;
- generic `AddComponent`/`GetComponent`/`HasComponent`.

## 6. Системы и порядок обновления

Регистрация систем в `Engine::RegisterSystems`:

1. `VoxelGameplaySystem` (если voxel sandbox включен).
2. `ScriptSystem`.
3. `PhysicsSystem`.
4. `RenderSystem`.

Это важно для поведения:

- gameplay и скрипты обновляют мир до физики/рендера;
- рендер всегда идет последним.

## 7. Рендер-подсистема

### 7.1 Абстракция

- `Renderer` (`src/Engine/Rendering/Renderer.h`) выбирает backend по `RenderAPI`.
- Backend реализует `IRenderBackend` (`Initialize`, `Render`, `Name`).
- В `Render` передаются:
  - `World`;
  - опционально `VoxelWorld`;
  - frame index;
  - callback для UI рендера.

### 7.2 DirectX12

`src/Engine/Rendering/Backends/DirectX12RenderBackend.cpp`:

- реальная инициализация GPU:
  - DXGI factory/adapter/device;
  - command queue;
  - swapchain;
  - RTV/DSV;
  - fence;
  - command list/allocators.
- voxel render path:
  - построение chunk meshes через `VoxelMesher`;
  - обновление на изменение `VoxelWorld::Revision()`;
  - frustum culling (`BoundingFrustum`);
  - draw indexed чанков.
- ImGui рендерится в том же кадре через `ImGui_ImplDX12_*`.

### 7.3 Vulkan и Null

- `VulkanRenderBackend` — scaffold/stub.
- `NullRenderBackend` — логический fallback без GPU рендера.

## 8. Окно и ввод

- `WindowSystem` выбирает backend окна.
- На Windows используется `Win32Window`.
- Есть fallback `NullWindow`.

`InputState` поддерживает:

- `Escape`, `Space`, `W`, `A`, `S`, `D`, `Q`, `E`, `F`.

Доступные запросы:

- `IsDown`;
- `WasPressed`;
- `WasReleased`.

## 9. Скрипты C#

`ScriptHost` (`src/Engine/Scripting/ScriptHost.cpp`):

1. Находит `ScriptRuntime.dll`.
2. Пишет входной файл (delta + сущности со скриптами).
3. Запускает `dotnet ScriptRuntime.dll <input> <output>`.
4. Читает выход и обновляет позиции сущностей.

C# runtime расположен в `scripts/ScriptRuntime`:

- `Program.cs` — протокол ввода/вывода;
- `EngineScripts.cs` — `IScriptBehavior` и фабрика скриптов.

## 10. Voxel-модуль

`VoxelWorld` (`src/Game/Minecraft/VoxelWorld.h`):

- чанковая генерация мира;
- get/set блока;
- raycast;
- surface height;
- revision/version для invalidation render-данных.

`VoxelMesher`:

- генерация chunk mesh (greedy meshing);
- цвет вершин по типу блока;
- bounds для culling.

## 11. Editor

`EditorUI`:

- верхнее меню (`File`, `Edit`, `Level`, `Display`, `Game`, `Tools`, `Debug`, `Help`);
- panel registry;
- overlay состояния;
- взаимодействие с runtime (закрытие окна, сериализация уровня и т.д.).

Панели реализуют `IEditorPanel`.

`PanelRegistry` отвечает за:

- видимость панелей;
- docking-слоты (`Left/Center/Right/Bottom/Floating`);
- вкладки регионов;
- reset layout.

`Viewport` закреплен в центре как основная рабочая область.

## 12. Сборка и зависимости

Определены в `CMakeLists.txt`:

- `RaiderEngine` (static library);
- `Sandbox` (executable);
- `ManagedScripts` (custom target, если найден `dotnet`);
- `DearImGui` (если есть `third_party/imgui-1.90.9`).

Платформенные линковки Windows:

- `d3d12`, `dxgi`, `dxguid`, `d3dcompiler`, `user32`, `gdi32`.

## 13. Точки расширения

Базовые точки расширения:

- новые компоненты: `Components.h`;
- новые системы: `src/Engine/Systems`;
- новые editor-панели: `src/Editor/Panels`;
- новые backend-ы рендера: `src/Engine/Rendering/Backends`;
- новые C# скрипты: `scripts/ScriptRuntime/EngineScripts.cs`.

Подробные инструкции по расширению вынесены в `docs/ENGINE_EXTENSIBILITY_RU.md`.

