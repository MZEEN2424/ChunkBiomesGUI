cmake_minimum_required(VERSION 3.10)
project(chunkbiomesgui)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)

# Add GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

# Add cubiomes subdirectory
add_subdirectory(cubiomes)

# Create Bfinders library
add_library(bfinders STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/Bfinders.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Bfinders.h"
)

target_include_directories(bfinders PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/cubiomes"
)

target_link_libraries(bfinders PUBLIC
    cubiomes
)

# Create ImGui library
add_library(imgui STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.cpp"
)

target_include_directories(imgui PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends"
)

target_link_libraries(imgui PUBLIC
    glfw
    OpenGL::GL
)

# Create executable
add_executable(chunkbiomesgui
    "${CMAKE_CURRENT_SOURCE_DIR}/ChunkBiomesGUI.cpp"
)

target_link_libraries(chunkbiomesgui PRIVATE
    cubiomes
    bfinders
    imgui
    glfw
    OpenGL::GL
)

target_include_directories(chunkbiomesgui PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends"
    "${CMAKE_CURRENT_SOURCE_DIR}/cubiomes"
)

target_compile_definitions(chunkbiomesgui PRIVATE
    IMGUI_IMPL_OPENGL_LOADER_GLAD
)

# Windows-specific settings
if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Install targets
install(TARGETS chunkbiomesgui
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Set default install prefix if not specified
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation prefix" FORCE)
endif()
