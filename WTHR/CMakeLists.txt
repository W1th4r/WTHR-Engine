cmake_minimum_required(VERSION 3.16)
project(WTHR LANGUAGES CXX)

# --------------------------
# Sources and headers
# --------------------------
file(GLOB_RECURSE RENDERER_SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE RENDERER_HEADERS CONFIGURE_DEPENDS include/*.hpp)

# Create static library
add_library(WTHR STATIC ${RENDERER_SOURCES} ${RENDERER_HEADERS})

# --------------------------
# Lua (compile from source)
# --------------------------
set(LUA_SRC
    # Core
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/lapi.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/lcode.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/lctype.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/ldebug.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/ldo.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/ldump.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/lfunc.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/lgc.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/llex.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/lmem.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/lobject.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/lopcodes.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/lparser.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/lstate.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/lstring.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/ltable.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/ltm.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/lundump.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/lvm.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/lzio.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/lauxlib.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/lbaselib.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/lcorolib.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/ltablib.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/lstrlib.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/liolib.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/loslib.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/lmathlib.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/ldblib.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/lutf8lib.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/loadlib.c
       ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua/linit.c
)


add_library(Lua STATIC ${LUA_SRC})
target_include_directories(Lua PUBLIC vendor/lua)

# --------------------------
# Include directories
# --------------------------
target_include_directories(WTHR
    PUBLIC
        include
        ${CMAKE_CURRENT_SOURCE_DIR}/../Editor/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/glad/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/glfw/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/glm
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/spdlog/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/assimp/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/entt/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/json/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/lua
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/sol2/include
)

# --------------------------
# Link libraries
# --------------------------
target_link_libraries(WTHR
    PUBLIC
        glad
        glfw
        imgui
        ImGuizmo
        spdlog::spdlog
        assimp
        nlohmann_json
        Lua
        sol2
        EnTT
        vendor_asio
        miniaudio
)

# --------------------------
# Compiler options
# --------------------------
if(MSVC)
    target_compile_options(WTHR PRIVATE /MP)
endif()

# --------------------------
# Definitions
# --------------------------
target_compile_definitions(WTHR PUBLIC LEAN_AND_MEAN)

# --------------------------
# Precompiled header
# --------------------------
target_precompile_headers(WTHR PRIVATE ../Editor/include/pch.hpp)
