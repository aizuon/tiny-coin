find_package(GTest CONFIG REQUIRED)

file(GLOB_RECURSE TINY_TEST_SOURCES src/*.cpp)

add_executable(tiny-test ${TINY_TEST_SOURCES})

target_link_libraries(tiny-test
    PRIVATE
        tiny-lib
        GTest::gtest
)

include(GoogleTest)
gtest_discover_tests(tiny-test)

if(MSVC)
    target_compile_options(tiny-test PRIVATE /W3 /permissive-)
else()
    target_compile_options(tiny-test PRIVATE -Wall -Wextra -Wpedantic)
endif()
