enable_testing()

add_executable(all_tests
        test_elias_fano.cpp
        test_s8b.cpp
        test_main.cpp
)

target_link_libraries(all_tests
        PRIVATE
        gcis_headers
        gtest
        gtest_main
)

include(GoogleTest)
gtest_discover_tests(all_tests)