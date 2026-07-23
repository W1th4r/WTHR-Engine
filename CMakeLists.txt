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

#asio{
find_package(Threads REQUIRED)

# 2. Define the Asio interface target directly from the top level
add_library(vendor_asio INTERFACE)

# 3. Point to the vendor folder path (adjust the 'asio/asio/include' path if needed)
target_include_directories(vendor_asio INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/asio/include
)

# 4. Attach the required thread library and standalone flag
target_link_libraries(vendor_asio INTERFACE Threads::Threads)
target_compile_definitions(vendor_asio INTERFACE 
    ASIO_STANDALONE
    WIN32_LEAN_AND_MEAN    # Prevents windows.h from including old WinSock.h
    _WIN32_WINNT=0x0A00    # Explicitly targets Windows 10/11 (Asio requires this)
)

# Now, add your core subdirectory
#asio}


# Vendor libraries
add_subdirectory(vendor/glfw)
add_subdirectory(vendor/glm)
add_subdirectory(vendor/spdlog)
add_subdirectory(vendor/entt)
add_subdirectory(vendor/json)
add_subdirectory(vendor/assimp)
add_subdirectory(vendor/Catch2)
add_subdirectory(vendor/sol2)
add_subdirectory(vendor/miniaudio)

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
    vendor/imgui/misc/cpp/imgui_stdlib.cpp
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
# 2. Add Catch2 tests target
option(WTHR_BUILD_TESTS "Build WTHR Engine Unit Tests" ON)

if(WTHR_BUILD_TESTS)
    enable_testing()
    
    # Grab test files
    file(GLOB_RECURSE TEST_SOURCES "Tests/*.cpp")
    
    add_executable(WTHR_Tests ${TEST_SOURCES})
    
    # Link against Catch2, your core engine library, and glad/glfw for GL context
    target_link_libraries(WTHR_Tests PRIVATE 
        Catch2::Catch2 
        WTHR 
    )
    
    # Enable `ctest` command-line runner
    add_test(NAME EngineUnitTests COMMAND WTHR_Tests)
endif()
target_link_libraries(WTHR_Tests PRIVATE 
    Catch2::Catch2
    WTHR # Your core engine library
    glad
    glfw
)