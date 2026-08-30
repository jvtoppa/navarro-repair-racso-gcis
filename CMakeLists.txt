cmake_minimum_required(VERSION 3.12)
project(GCIS_REPAIR C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-O3)

find_library(DL_LIBRARY dl)

include_directories(
    include
    include/malloc_count-0.7.1
)

set(COMMON_SOURCES
    include/bitvector.cpp
    include/malloc_count-0.7.1/malloc_count.c
)


add_executable(gcis gcis/gcis.cpp ${COMMON_SOURCES})
target_link_libraries(gcis PRIVATE ${DL_LIBRARY})
