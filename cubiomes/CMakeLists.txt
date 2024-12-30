cmake_minimum_required(VERSION 3.10)
project(cubiomes C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set compiler flags based on compiler
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

add_library(cubiomes STATIC
    biomes.c
    layers.c
    generator.c
    finders.c
    util.c
    noise.c
    biomenoise.c
    quadbase.c
)

target_include_directories(cubiomes PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
