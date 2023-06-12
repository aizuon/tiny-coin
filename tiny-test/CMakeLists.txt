set(BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "${BASE_DIR}/*.cpp")
file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS "${BASE_DIR}/*.hpp")
list(FILTER HEADER_FILES EXCLUDE REGEX "${BASE_DIR}/pch.hpp")

add_executable(tiny-test "${BASE_DIR}/main.cpp" ${SOURCE_FILES})

find_package(GTest REQUIRED)
target_link_libraries(tiny-test PRIVATE GTest::gtest GTest::gtest_main)
enable_testing()

target_include_directories(tiny-test PUBLIC "${BASE_DIR}")

target_link_libraries(tiny-test PUBLIC tiny-lib)

target_precompile_headers(tiny-test PRIVATE "${BASE_DIR}/pch.hpp")
