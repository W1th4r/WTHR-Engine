# Collect all source and header files
file(GLOB_RECURSE WTHR_SRC
    src/*.cpp
    src/*.cxx
)
file(GLOB_RECURSE WTHR_HDR
    include/*.hpp
    include/*.h
)
# Create executable
add_executable(Editor
    ${WTHR_SRC}
    ${WTHR_HDR}
    src/pch.cpp      # precompiled header source
    include/pch.hpp  # precompiled header
 "src/Editor.cpp")

# Include directories
target_include_directories(Editor PUBLIC include)

# Set C++ standard (inherits from root)
target_compile_features(Editor PUBLIC cxx_std_20)

# MSVC multi-processor compile
if(MSVC)
    target_compile_options(Editor PRIVATE /MP)
endif()

# Precompiled header
target_precompile_headers(Editor PRIVATE include/pch.hpp)

# Link libraries
target_link_libraries(Editor
    PRIVATE
        WTHR
        Physics
        glfw
        glm
        spdlog
        nlohmann_json::nlohmann_json
        assimp
        imgui
        glad
        ImGuizmo
)
add_custom_command(TARGET Editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:Editor>
)