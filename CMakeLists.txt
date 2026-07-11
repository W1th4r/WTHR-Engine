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
set(ASSIMP_BUILD_WERROR OFF CACHE BOOL "Build assimp with -Werror" FORCE)
add_compile_options(-Wno-unknown-warning-option)

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
# --- ADD THIS BLOCK RIGHT AFTER YOUR ASSIMP SETTINGS ---
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-fexperimental-library)
    add_compile_options(-Wno-error=deprecated-declarations)
      add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>)
    add_link_options(-stdlib=libc++)
    add_compile_definitions(SOL_EXCEPTIONS_SAFE_PROPAGATION=1) 
    add_compile_definitions(LUA_VERSION_NUM=504)
endif()
if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
endif()
set(ASSIMP_BUILD_MINIZIP ON CACHE INTERNAL "Build Assimp with bundled minizip")
# Vendor libraries
add_subdirectory(vendor/glfw)
add_subdirectory(vendor/glm)
add_subdirectory(vendor/spdlog)
add_subdirectory(vendor/entt)
add_subdirectory(vendor/json)
add_subdirectory(vendor/assimp)
add_subdirectory(vendor/Catch2)
# Add this before you compile sol2 or targets depending on it
include_directories(SYSTEM /usr/include/lua5.4)
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
# Find Minizip on Arch/CachyOS
find_package(PkgConfig REQUIRED)
pkg_check_modules(MINIZIP REQUIRED minizip)

# Force the specific minizip subdirectory into the include paths
include_directories(/usr/include/minizip)

# Add WTHR app
add_subdirectory(WTHR)

if(UNIX AND NOT APPLE)
  target_link_libraries(WTHR PRIVATE ${X11_LIBRARIES})
endif()

add_subdirectory(Physics)
add_subdirectory(Editor)
add_subdirectory(Runtime)
