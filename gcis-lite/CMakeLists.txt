cmake_minimum_required(VERSION 3.2)
project(GCIS CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_VERBOSE_MAKEFILE ON)

if(CMAKE_BUILD_TYPE MATCHES DEBUG)
    message(STATUS "Debug mode")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
elseif(CMAKE_BUILD_TYPE MATCHES RELEASE)
    message(STATUS "Release mode")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native -Wall")
elseif(PROFILE)
    message(STATUS "Gprof Profile mode")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg -no-pie")
else()
    message(STATUS "Default mode")
endif()

# ============================================
# SDSL INTERFACE TARGET (NO find_package)
# ============================================

set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(SDSL_BUILD_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory(external/sdsl-lite)

target_include_directories(sdsl INTERFACE
        ${CMAKE_SOURCE_DIR}/include
)

# ============================================
# PROJECT HEADERS TARGET
# ============================================

add_library(gcis_headers INTERFACE)

target_include_directories(gcis_headers INTERFACE
        ${CMAKE_SOURCE_DIR}/headers
)

target_link_libraries(gcis_headers INTERFACE sdsl)

# ============================================
# ENABLE TESTING
# ============================================

enable_testing()
add_subdirectory(unit_tests)

# ============================================

add_executable(gcis_lite
        main.cpp
        ../include/malloc_count-0.7.1/malloc_count.c
)

target_link_libraries(gcis_lite PRIVATE gcis_headers dl)

target_include_directories(gcis_lite PRIVATE
        ${CMAKE_SOURCE_DIR}/../include/malloc_count-0.7.1
)