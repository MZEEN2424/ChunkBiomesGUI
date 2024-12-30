cmake_minimum_required(VERSION 3.10)
project(chunkbiomes)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# OpenGL
find_package(OpenGL REQUIRED)

# Add GLFW subdirectory
add_subdirectory(glfw)

# Add cubiomes subdirectory
add_subdirectory(cubiomes)

# Create ImGui library
add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    imgui
    imgui/backends
    ${OPENGL_INCLUDE_DIR}
)

target_link_libraries(imgui PUBLIC
    glfw
    ${OPENGL_LIBRARIES}
)

# Create Bfinders library
add_library(bfinders STATIC
    Bfinders.c
    Bfinders.h
)

target_include_directories(bfinders PUBLIC
    ${CMAKE_SOURCE_DIR}/cubiomes
)

target_link_libraries(bfinders PUBLIC
    cubiomes
)

# Original chunkbiomes executable
add_executable(chunkbiomes test.c)
target_link_libraries(chunkbiomes PRIVATE 
    cubiomes
    bfinders
)
target_include_directories(chunkbiomes PRIVATE ${CMAKE_SOURCE_DIR}/cubiomes)

# GUI executable
add_executable(chunkbiomesgui ChunkBiomesGUI.cpp)
target_link_libraries(chunkbiomesgui PUBLIC
    imgui
    glfw
    cubiomes
    bfinders
    ${OPENGL_LIBRARIES}
)
target_include_directories(chunkbiomesgui PRIVATE 
    ${CMAKE_SOURCE_DIR}/cubiomes
    ${CMAKE_SOURCE_DIR}/imgui
    ${CMAKE_SOURCE_DIR}/imgui/backends
)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)
