# Разработка игры на RaiderEngine

Документ о том, как на текущем движке собирать игровую логику, уровни и сборку проекта.

## 1. Базовый pipeline

1. Настроить `EngineConfig`.
2. Инициализировать движок.
3. Создать начальные сущности и компоненты.
4. Добавить ECS-системы под механики.
5. При необходимости подключить C# скрипты.
6. Настроить editor workflow для уровня.
7. Собрать и прогнать smoke test.

## 2. Точка входа игры

Сейчас пример — `src/Sandbox/main.cpp`.

Минимальная конфигурация:

```cpp
rg::EngineConfig config;
config.projectName = "MyGame";
config.window.title = "MyGame";
config.window.width = 1920;
config.window.height = 1080;
config.renderAPI = rg::RenderAPI::DirectX12;
config.enableEditorUI = true;
config.enableVoxelSandbox = true;  // или false для не-voxel игры
config.maxFrames = 1000000;
config.fixedDeltaSeconds = 1.0f / 60.0f;
```

Далее:

```cpp
rg::Engine engine(config);
if (!engine.Initialize()) return 1;

auto& world = engine.GetWorld();
// bootstrap сущности

engine.Run();
```

## 3. Создание сущностей

Используйте `World::CreateEntity` и API `Entity`.

Пример:

```cpp
auto& world = engine.GetWorld();
auto enemy = world.CreateEntity("Enemy");
enemy.Transform().position = rg::Vector3{10.0f, 0.0f, 5.0f};
enemy.AddComponent<rg::RigidbodyComponent>();
enemy.AttachScript("OrbitYScript");
```

Рекомендация:

- для стабильности всегда задавайте `Transform` явно;
- не полагайтесь на auto-created компоненты, если поведение критично.

## 4. Механики через ECS-системы

Системы — основной способ писать runtime механику.

### Пример шаблона

```cpp
void MyCombatSystem::Update(rg::SystemContext& context, float dt)
{
    context.world.ForEach<HealthComponent, TransformComponent>(
        [&](rg::Entity entity, HealthComponent& health, TransformComponent& transform)
        {
            (void)entity;
            (void)transform;
            if (health.current <= 0.0f)
            {
                // ...
            }
        });
}
```

### Где подключать

`Engine::RegisterSystems()` в `src/Engine/Engine.cpp`.

Порядок влияет на геймплей, поэтому фиксируйте его сознательно.

## 5. Скрипты на C#

Скрипты удобны для data-driven поведения без перекомпиляции native части.

### 5.1 Добавление нового script class

Файл: `scripts/ScriptRuntime/EngineScripts.cs`.

```csharp
public sealed class BobbingScript : IScriptBehavior
{
    public void OnUpdate(ref Vec3 position, float deltaTime)
    {
        position.Y += MathF.Sin(position.X + position.Z) * 0.1f * deltaTime;
    }
}
```

Зарегистрировать в `ScriptFactory.Create`:

```csharp
nameof(BobbingScript) => new BobbingScript(),
```

Назначить в C++:

```cpp
entity.AttachScript("BobbingScript");
```

### 5.2 Build managed части

- автоматом через target `ManagedScripts` при сборке `Sandbox`;
- или из editor панели `Build`.

## 6. Работа с уровнями

Editor уже умеет:

- `New Level`;
- `Save Level`;
- `Load Level`;
- редактирование компонентов через `Inspector`.

### 6.1 Что сериализуется

- `Name`, `Transform`;
- `ScriptComponent`, `MeshComponent`;
- `RigidbodyComponent`;
- `VoxelPlayerComponent`.

Если вы добавили новый важный компонент, расширьте сериализацию вручную.

## 7. Использование voxel sandbox как базовой игры

Если `enableVoxelSandbox = true`, включаются:

- `VoxelWorld` генерация;
- `VoxelGameplaySystem`:
  - `W/A/S/D` движение;
  - `Space` прыжок;
  - `Q` ломать блок;
  - `E` ставить блок;
  - `F` переключать блок.

Это удобная отправная точка для survival/sandbox прототипа.

## 8. Добавление своего игрового target (вместо Sandbox)

В `CMakeLists.txt` можно добавить отдельный executable:

```cmake
add_executable(MyGame src/MyGame/main.cpp)
target_link_libraries(MyGame PRIVATE RaiderEngine)
add_dependencies(MyGame ManagedScripts)
```

Так вы разделяете runtime игры и тестовый `Sandbox`.

## 9. Типовой workflow “новая фича”

1. Добавить компоненты данных.
2. Добавить/обновить систему логики.
3. Добавить editor-поддержку (панель, inspector fields, debug menu).
4. Обновить сериализацию уровня.
5. Собрать `Release`, проверить запуск.
6. Проверить поведение без editor UI (`enableEditorUI = false`).

## 10. Практический пример: “враг патрулирует”

Шаги:

1. Добавить `PatrolComponent`:
   - `pointA`, `pointB`, `speed`, `toB`.
2. Добавить `PatrolSystem`:
   - обновляет `Transform.position` к целевой точке.
3. Добавить инспектор редактирование `PatrolComponent`.
4. Добавить сериализацию `PATROL` секции в level.
5. Создать врагов в уровне через editor.

## 11. Build и релизный цикл

Локальная сборка:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Запуск:

```powershell
.\build\Release\Sandbox.exe
```

Если `cmake` не в `PATH`, используйте путь из Visual Studio CMake.

## 12. Ограничения текущей версии (важно для планирования)

- Основной графический путь — `DirectX12` на Windows.
- `Vulkan` пока не production-ready.
- Нет полноценного asset importer pipeline.
- Нет пакетирования релизных билдов в installer/launcher.
- Нет сетевой подсистемы.

## 13. Рекомендованный roadmap для игры

1. Стабилизировать core-loop игры через ECS.
2. Закрыть serialization всех критичных компонентов.
3. Добавить editor tooling под контент-команду.
4. Вынести gameplay settings в data/config.
5. Добавить автотесты для чисто data-логики (где возможно).
6. Только после этого масштабировать рендер/контент.

