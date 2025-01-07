cmake_minimum_required(VERSION 3.10)
project(chunkbiomesgui)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Set output directories relative to build directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# OpenGL
find_package(OpenGL REQUIRED)

# Add GLFW subdirectory
add_subdirectory(glfw)

# Add Chunkbiomes subdirectory
add_subdirectory(chunkbiomes)

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
    ${OPENGL_INCLUDE_DIR}
)

target_link_libraries(imgui PUBLIC
    glfw
    ${OPENGL_LIBRARIES}
)

# Create GUI executable
add_executable(chunkbiomesgui "${CMAKE_CURRENT_SOURCE_DIR}/ChunkBiomesGUI.cpp")
target_link_libraries(chunkbiomesgui PRIVATE
    chunkbiomes
    imgui
    glfw
    ${OPENGL_LIBRARIES}
)
target_include_directories(chunkbiomesgui PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/Chunkbiomes"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends"
)

target_compile_definitions(chunkbiomesgui PRIVATE
    IMGUI_IMPL_OPENGL_LOADER_GLAD
)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

# Remove the imgui.ini copy section since it's handled by the application
# The application will create the file if it doesn't exist

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