cmake_minimum_required(VERSION 3.12)
project(GCIS_REPAIR C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-O3)

# 1. Use modern CMake target for libdl
find_package(Threads REQUIRED)

include_directories(
    include
    include/malloc_count-0.7.1
)

set(COMMON_SOURCES
    include/bitvector.cpp
    include/malloc_count-0.7.1/malloc_count.c
)

add_executable(gcis gcis/gcis.cpp ${COMMON_SOURCES})

# 2. Add -rdynamic to export symbols so malloc_count interposing works correctly
target_link_options(gcis PRIVATE -rdynamic)

# 3. Link dynamic linking library and threads
target_link_libraries(gcis PRIVATE ${CMAKE_DL_LIBS} Threads::Threads)