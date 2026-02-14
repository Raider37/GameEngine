# Расширение движка и модулей

Документ про развитие самого RaiderEngine: как добавлять подсистемы, интегрировать их в цикл кадра и не ломать текущий runtime.

## 1. Общие принципы расширения

1. Сначала определить слой:
   - данные (`Component`);
   - логика (`System`);
   - визуализация (`RenderBackend`/Editor panel);
   - внешняя интеграция (`ScriptHost`, input/window, ресурсы).
2. Добавлять минимальными вертикальными срезами:
   - сначала compile + runtime стабильность;
   - затем редакторные инструменты;
   - потом оптимизации.
3. Каждую фичу проводить через один сценарий smoke test:
   - сборка `Release`;
   - запуск `Sandbox`;
   - проверка лога и UI.

## 2. Добавление нового `Component`

Файл: `src/Engine/Scene/Components.h`.

Пример:

```cpp
struct HealthComponent
{
    float current = 100.0f;
    float max = 100.0f;
};
```

Дальше:

1. Добавить/обновить editor-отображение (обычно `InspectorPanel`).
2. Обновить сериализацию уровня в `EditorUI::SaveLevel/LoadLevel`, если компонент должен жить в level-файле.
3. Использовать в системах через:

```cpp
world.ForEach<HealthComponent, TransformComponent>(...);
```

## 3. Добавление новой `System`

### 3.1 Создание

Создать `src/Engine/Systems/MySystem.h/.cpp`, унаследоваться от `ISystem`.

```cpp
class MySystem final : public rg::ISystem
{
public:
    const char* Name() const override { return "MySystem"; }
    bool Initialize(rg::SystemContext& context) override;
    void Update(rg::SystemContext& context, float deltaSeconds) override;
};
```

### 3.2 Подключение

1. Добавить `.cpp` в `ENGINE_SOURCES` в `CMakeLists.txt`.
2. Подключить заголовок в `src/Engine/Engine.cpp`.
3. Зарегистрировать в `Engine::RegisterSystems()` в нужном порядке.

### 3.3 Правильный порядок

Рекомендуемый порядок для игровых систем:

1. Input-driven gameplay.
2. Скрипты.
3. Физика.
4. Пост-обработка данных (AI/debug transforms и т.п.).
5. Рендер.

Если система должна писать данные для рендера в том же кадре, она должна идти до `RenderSystem`.

## 4. Расширение `SystemContext`

`SystemContext` (`src/Engine/Systems/SystemContext.h`) — канал передачи сервисов в системы.

Добавляйте туда только runtime-зависимости, которые реально нужны нескольким системам:

- новый менеджер ресурсов;
- глобальная конфигурация;
- доступ к модулю игры.

После расширения `SystemContext` нужно обновить обе точки создания в `Engine`:

- `Engine::Initialize` (контекст для `Initialize` систем);
- `Engine::Update` (контекст на каждый кадр).

## 5. Добавление нового runtime-модуля движка

Пример структуры:

- `src/Engine/Audio/AudioDevice.h/.cpp`
- `src/Engine/Audio/AudioSystem.h/.cpp` (если нужно кадро-обновление)

Интеграция:

1. Добавить модуль как поле в `Engine` (если global lifecycle).
2. Инициализировать в `Engine::Initialize`.
3. Передать в `SystemContext` при необходимости.
4. Добавить систему, если модуль требует per-frame update.

Критерий хорошей интеграции:

- модуль можно выключить флагом;
- ошибка инициализации логируется явно;
- fallback-поведение определено.

## 6. Расширение рендера

## 6.1 Добавить новый backend

1. Создать backend класс в `src/Engine/Rendering/Backends`.
2. Реализовать `IRenderBackend`.
3. Добавить создание backend в `Renderer.cpp`.
4. Добавить новое значение в `RenderAPI`.

Минимальный контракт backend:

- `Initialize(...)` не должен падать молча;
- `Render(...)` должен быть устойчив к `nullptr` voxelWorld;
- возвращать понятное `Name()`.

## 6.2 Расширение DirectX12 backend

Текущий `DirectX12RenderBackend` уже содержит:

- swapchain path;
- depth target;
- voxel mesh rebuild по revision;
- frustum culling;
- ImGui pass.

При добавлении нового render path:

1. Не ломать текущий frame lifecycle:
   - `BeginFrame` -> draw -> `EndFrame`.
2. Отделять upload/rebuild от draw-фазы.
3. Учитывать window resize и resource lifetime.
4. Логировать только существенные ошибки (не spam на каждый кадр).

## 7. Расширение input/window

### 7.1 Новая клавиша

1. Добавить `KeyCode` в `InputState.h`.
2. Промапить клавишу в Win32 `Window.cpp` (`ApplyKey`).
3. Использовать в системах через `IsDown/WasPressed/WasReleased`.

### 7.2 Новый оконный backend

Добавить реализацию `IWindow` и выбрать ее в `WindowSystem::Initialize`.

Обязательные методы:

- `Initialize`;
- `PollEvents`;
- `ShouldClose/RequestClose`;
- `NativeHandle/Width/Height`.

## 8. Расширение C# скриптового runtime

## 8.1 Добавление script behavior

Файл: `scripts/ScriptRuntime/EngineScripts.cs`.

1. Реализовать `IScriptBehavior`.
2. Зарегистрировать в `ScriptFactory.Create`.
3. Назначить сущности через `AttachScript("ClassName")`.

## 8.2 Изменение протокола C++ <-> C#

Если меняется формат ввода/вывода:

1. Обновить `ScriptHost::WriteInput/ReadOutput`.
2. Обновить `Program.cs` (парсинг и формат output).
3. Сохранить обратную совместимость или синхронно обновить оба конца.

Важно: на продвинутом этапе лучше перейти с plain text на бинарный/JSON протокол и versioning.

## 9. Расширение editor UI

## 9.1 Новая панель

1. Создать `IEditorPanel` реализацию (`Name`, `Draw`).
2. Добавить `.cpp` в `CMakeLists.txt`.
3. Зарегистрировать в `EditorUI::Initialize` через `m_panelRegistry.Add(...)`.
4. Выбрать дефолтный docking-слот (`Left/Center/Right/Bottom/Floating`).

## 9.2 Меню editor

Меню живут в `EditorUI.cpp`:

- `DrawFileMenu`
- `DrawEditMenu`
- `DrawLevelMenu`
- `DrawDisplayMenu`
- `DrawGameMenu`
- `DrawDebugMenu`

Если добавляете команды, учитывайте:

- сообщение пользователю в `m_lastActionMessage`;
- лог при важных действиях;
- проверку валидности выделенной сущности/ресурсов.

## 9.3 Сериализация уровня

Уровни сейчас пишутся в текстовый формат `RAIDER_LEVEL_V1` (`EditorUI::SaveLevel/LoadLevel`).

При добавлении сериализуемого компонента:

1. Добавьте секцию записи в `SaveLevel`.
2. Добавьте парсинг секции в `LoadLevel`.
3. Обеспечьте дефолтные значения при отсутствии старого поля.

## 10. CMake и сборка новых модулей

Каждый новый `.cpp` должен попасть в `ENGINE_SOURCES`.

Если добавляется внешняя зависимость:

1. Добавить `target_include_directories`/`target_link_libraries`.
2. Ограничить платформой через `if(WIN32)` или нужный guard.
3. Добавить compile definition при необходимости.

Проверка после изменений:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

## 11. Чеклист “добавил модуль — что проверить”

1. Собирается `Release`.
2. Запускается `Sandbox.exe`.
3. Нет критических ошибок в логах.
4. Изменения корректно работают без editor UI.
5. Изменения корректно работают с editor UI.
6. Сценарий выключенного модуля (feature flag/fallback) тоже стабилен.

## 12. Частые ошибки и как избежать

- Несогласованный `SystemContext` в `Initialize` и `Update`.
- Добавили компонент, но не добавили сериализацию, и данные теряются после `Save/Load`.
- Изменили input enum, но забыли Win32 mapping.
- Добавили backend в `RenderAPI`, но забыли `CreateBackend`.
- Добавили panel, но не включили `.cpp` в `CMakeLists.txt`.
- Рендер-ресурсы пересоздаются каждый кадр вместо invalidate/rebuild по версии.

## 13. Рекомендуемый workflow разработки движка

1. Описать целевую фичу в терминах данных + систем + editor tooling.
2. Сделать минимальный рабочий путь без UI.
3. Добавить editor-поддержку.
4. Добавить debug-команды (меню `Debug`) и логирование.
5. Прогнать smoke test.
6. Обновить документацию (этот файл + профильный гайд).

