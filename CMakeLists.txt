cmake_minimum_required(VERSION 3.22)

project(RaiderEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ENGINE_SOURCES
    src/Engine/Core/Log.cpp
    src/Engine/Input/InputState.cpp
    src/Engine/Platform/Window.cpp
    src/Engine/Resources/ResourceManager.cpp
    src/Engine/Scene/Entity.cpp
    src/Engine/Scene/World.cpp
    src/Engine/Rendering/Backends/NullRenderBackend.cpp
    src/Engine/Rendering/Backends/DirectX12RenderBackend.cpp
    src/Engine/Rendering/Backends/VulkanRenderBackend.cpp
    src/Engine/Rendering/Renderer.cpp
    src/Engine/Scripting/ScriptHost.cpp
    src/Engine/Systems/VoxelGameplaySystem.cpp
    src/Engine/Systems/ScriptSystem.cpp
    src/Engine/Systems/PhysicsSystem.cpp
    src/Engine/Systems/RenderSystem.cpp
    src/Game/Minecraft/VoxelWorld.cpp
    src/Game/Minecraft/VoxelMesher.cpp
    src/Editor/EditorUI.cpp
    src/Editor/Panels/PanelRegistry.cpp
    src/Editor/Panels/SceneHierarchyPanel.cpp
    src/Editor/Panels/InspectorPanel.cpp
    src/Editor/Panels/AssetsPanel.cpp
    src/Editor/Panels/StatsPanel.cpp
    src/Editor/Panels/VoxelWorldPanel.cpp
    src/Editor/Panels/ViewportPanel.cpp
    src/Editor/Panels/WorldSettingsPanel.cpp
    src/Editor/Panels/LevelClassesPanel.cpp
    src/Editor/Panels/BuildPanel.cpp
    src/Engine/Engine.cpp
)

set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/third_party/imgui-1.90.9")
set(RG_WITH_IMGUI OFF)

if(WIN32 AND EXISTS "${IMGUI_DIR}/imgui.cpp")
    add_library(DearImGui STATIC
        "${IMGUI_DIR}/imgui.cpp"
        "${IMGUI_DIR}/imgui_draw.cpp"
        "${IMGUI_DIR}/imgui_tables.cpp"
        "${IMGUI_DIR}/imgui_widgets.cpp"
        "${IMGUI_DIR}/imgui_demo.cpp"
        "${IMGUI_DIR}/backends/imgui_impl_win32.cpp"
        "${IMGUI_DIR}/backends/imgui_impl_dx12.cpp"
    )
    target_include_directories(DearImGui PUBLIC
        "${IMGUI_DIR}"
        "${IMGUI_DIR}/backends"
    )
    target_link_libraries(DearImGui PUBLIC d3d12 dxgi user32)
    set(RG_WITH_IMGUI ON)
endif()

add_library(RaiderEngine STATIC ${ENGINE_SOURCES})
target_include_directories(RaiderEngine PUBLIC src)

if(MSVC)
    target_compile_options(RaiderEngine PRIVATE /W4 /permissive-)
else()
    target_compile_options(RaiderEngine PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WIN32)
    target_link_libraries(RaiderEngine PRIVATE d3d12 dxgi dxguid d3dcompiler user32 gdi32)
endif()

if(RG_WITH_IMGUI)
    target_link_libraries(RaiderEngine PUBLIC DearImGui)
    target_compile_definitions(RaiderEngine PUBLIC RG_WITH_IMGUI=1)
else()
    target_compile_definitions(RaiderEngine PUBLIC RG_WITH_IMGUI=0)
    message(WARNING "Dear ImGui was not found in third_party/imgui-1.90.9. Editor UI will be disabled.")
endif()

add_executable(Sandbox src/Sandbox/main.cpp)
target_link_libraries(Sandbox PRIVATE RaiderEngine)

find_program(DOTNET_EXECUTABLE dotnet)
if(DOTNET_EXECUTABLE)
    set(MANAGED_OUTPUT_DIR "${CMAKE_BINARY_DIR}/managed/ScriptRuntime")
    set(SCRIPT_RUNTIME_DLL "${MANAGED_OUTPUT_DIR}/ScriptRuntime.dll")

    add_custom_target(ManagedScripts ALL
        COMMAND ${DOTNET_EXECUTABLE} build
            "${CMAKE_SOURCE_DIR}/scripts/ScriptRuntime/ScriptRuntime.csproj"
            -c Release
            -o "${MANAGED_OUTPUT_DIR}"
        COMMENT "Building C# script runtime"
        VERBATIM
    )

    add_dependencies(Sandbox ManagedScripts)
    target_compile_definitions(RaiderEngine PRIVATE RG_SCRIPT_RUNTIME_DLL="${SCRIPT_RUNTIME_DLL}")
else()
    message(WARNING "dotnet was not found. C# scripting will be disabled.")
    target_compile_definitions(RaiderEngine PRIVATE RG_DISABLE_SCRIPTING=1)
endif()
