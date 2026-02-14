# Руководство по редактору RaiderEngine

Документ описывает работу текущего editor UI, layout и инструменты для редактирования уровня.

## 1. Включение редактора

В `src/Sandbox/main.cpp`:

```cpp
config.enableEditorUI = true;
```

Также нужен ImGui в проекте (`third_party/imgui-1.90.9`), иначе editor отключится на этапе инициализации.

## 2. Верхнее меню

Меню реализовано в `src/Editor/EditorUI.cpp`.

Разделы:

- `File`:
  - `New Level`
  - `Save Level`
  - `Load Level`
  - `Reload Asset Cache`
  - `Exit`
- `Edit`:
  - `Duplicate Selected`
  - `Delete Selected`
  - `Deselect`
- `Level`:
  - создание базовых сущностей
  - regenerate voxel world
- `Display`:
  - ImGui demo/metrics/style
  - управление панелями
  - reset layout
- `Game`:
  - `Play/Pause/Stop` (editor state)
- `Tools`:
  - quick save
  - clear asset cache
  - открыть style editor
- `Debug`:
  - scene summary в лог
  - спавн test entities
  - проверка shader assets
- `Help`:
  - краткая подсказка по управлению.

## 3. Layout и docking

Система docking реализована в `PanelRegistry` через слоты:

- `Left`
- `Center`
- `Right`
- `Bottom`
- `Floating`

## 3.1 Дефолтный layout

- `Viewport` — центр (закреплен, не закрывается/не переносится).
- Справа: `Scene Objects`, `Inspector`, `World Settings`, `Level Classes`.
- Снизу: `Content Browser`, `Stats`, `Voxel World`, `Build`.

Если layout сбился:

- `Display -> Reset Editor Layout`
- или `Display -> Panels -> Reset All Panel Slots`.

## 3.2 Перенос панелей

Для любой панели:

- через `Display -> Panels -> <Panel Name>` можно выбрать slot;
- через контекст-меню вкладки панели (ПКМ на табе) можно сменить dock slot.

`Viewport` как core-panel всегда возвращается в `Center`.

## 4. Панели редактора

## 4.1 Viewport

Текущий viewport — 2D top-down обзор сцены:

- выбор entity ЛКМ;
- drag выделенной entity ЛКМ;
- pan средней кнопкой мыши;
- zoom колесиком;
- overlay grid и границы voxel мира.

## 4.2 Scene Objects

- список сущностей уровня;
- создание пустой сущности;
- выбор текущей сущности для инспектора.

## 4.3 Inspector

Редактирует компоненты выбранной сущности:

- `Name`, `Transform`;
- `ScriptComponent`;
- `MeshComponent`;
- `RigidbodyComponent`;
- `VoxelPlayerComponent`.

Есть быстрые действия:

- `Add Rigidbody`;
- `Add VoxelPlayer`;
- `Delete Entity`.

## 4.4 World Settings

Содержит:

- статистику уровня (entities/scripts/rigidbodies);
- voxel параметры (`Radius`, `Seed`);
- `Regenerate World` и `Reload Current`;
- revision/chunk counters и границы мира.

## 4.5 Level Classes

Показывает:

- количество сущностей с ключевыми компонентами;
- script class usage на уровне;
- кнопку `Apply To Selected` для назначения script class выделенной сущности.

## 4.6 Content Browser

Показывает дерево файлов из `ResourceManager::Root()`.

Назначение:

- быстрый обзор assets;
- проверка, что файлы доступны движку по relative path.

## 4.7 Stats

Runtime-метрики:

- frame index;
- backend рендера;
- frame time / FPS;
- entity count;
- voxel sandbox stats.

## 4.8 Voxel World

2D срез/карта voxel мира для debug/ориентации:

- просмотр surface/slice;
- follow player;
- tooltip по блоку.

## 4.9 Build

Сборка из редактора:

- `Configure + Build Release`
- `Configure + Build Debug`
- `Build Managed Scripts`

Показывает:

- путь до `cmake`/`dotnet`;
- статус;
- exit code;
- tail build log.

Логи складываются в `build/editor_*.log`.

## 5. Сериализация уровня

Сейчас реализована в `EditorUI::SaveLevel/LoadLevel`:

- формат: текстовый `RAIDER_LEVEL_V1`;
- файл по умолчанию: `build/editor_level.rglvl`.

Сохраняются:

- `Name`, `Transform`;
- `ScriptComponent`;
- `MeshComponent`;
- `RigidbodyComponent`;
- `VoxelPlayerComponent`.

## 6. Интеграция editor в runtime

- Рендер editor UI вызывается из `RenderSystem` через callback в renderer.
- `File -> Exit` выставляет флаг, который обрабатывается в `Engine::Update()` и закрывает окно.

## 7. Как добавить свою панель

1. Создайте `src/Editor/Panels/MyPanel.h/.cpp`.
2. Реализуйте `IEditorPanel`.
3. Добавьте `.cpp` в `CMakeLists.txt`.
4. Зарегистрируйте:

```cpp
m_panelRegistry.Add(std::make_unique<MyPanel>(), true, PanelDockSlot::Right);
```

5. Проверьте, что панель доступна через `Display -> Panels`.

## 8. Ограничения текущего editor UI

- Нет полноценного 3D render-to-texture viewport (сейчас top-down canvas).
- Нет undo/redo стека.
- Нет drag&drop ассетов в сцену.
- Нет project/level browser с множеством файлов уровней.

