# 1. Update minimum version to 3.25+ so CMake actually knows what Hot Reload variables are
cmake_minimum_required(VERSION 3.25)

# 2. Enable the policy FIRST
if(POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
endif()

# 3. Now set the Hot Reload format
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT 
  "$<IF:$<CONFIG:Debug>,EditAndContinue,ProgramDatabase>"
)

project(WTHR LANGUAGES C CXX)

# Set C++ standard for the whole project
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC specific tweaks
if(MSVC)
    add_compile_options(/MP)
    # This matches CMP0141 behavior perfectly
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if(MSVC)
  set(CMAKE_C_COMPILER_LAUNCHER "")
  set(CMAKE_CXX_COMPILER_LAUNCHER "")
endif()

# Vendor libraries
add_subdirectory(vendor/glfw)
add_subdirectory(vendor/glm)
add_subdirectory(vendor/spdlog)
add_subdirectory(vendor/entt)
add_subdirectory(vendor/json)
add_subdirectory(vendor/assimp)
add_subdirectory(vendor/Catch2)
add_subdirectory(vendor/sol2)

# Glad
add_library(glad STATIC vendor/glad/src/src/glad.c)
target_include_directories(glad PUBLIC vendor/glad/src/include)


# ImGui (manually add source files)
set(IMGUI_SRC
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/backends/imgui_impl_glfw.cpp
    vendor/imgui/backends/imgui_impl_opengl3.cpp
)
add_library(imgui ${IMGUI_SRC})
target_include_directories(imgui PUBLIC vendor/imgui vendor/imgui/backends)
target_link_libraries(imgui PUBLIC glfw)

# ImGuizmo
add_library(ImGuizmo vendor/ImGuizmo/ImGuizmo.cpp)
target_include_directories(ImGuizmo PUBLIC vendor/ImGuizmo vendor/imgui)
target_link_libraries(ImGuizmo PUBLIC imgui glm)

# Add WTHR app
add_subdirectory(WTHR)
add_subdirectory(Physics)
add_subdirectory(Editor)
add_subdirectory(Runtime)
