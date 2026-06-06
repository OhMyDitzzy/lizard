cmake_minimum_required(VERSION 3.20)
project(Lizard VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCES
    src/main.cpp
    src/core/lexer.cpp
    src/core/parser.cpp
    src/core/interpreter.cpp
    src/core/environment.cpp
    src/error/error.cpp
    src/builtins/io/show.cpp
)

add_executable(lizard ${SOURCES})

target_include_directories(lizard PRIVATE src)

# LIZARD_DEBUG is only defined in Debug builds.
# Wrap development-only output with #ifdef LIZARD_DEBUG or LZ_DEBUG().
target_compile_definitions(lizard PRIVATE
    $<$<CONFIG:Debug>:LIZARD_DEBUG>
)

if(MSVC)
    target_compile_options(lizard PRIVATE /W4)
else()
    target_compile_options(lizard PRIVATE -Wall -Wextra -Wpedantic)
endif()