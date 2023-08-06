set(BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "${BASE_DIR}/*.cpp")
file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS "${BASE_DIR}/*.hpp")
list(FILTER HEADER_FILES EXCLUDE REGEX "${BASE_DIR}/pch.hpp")

add_library(tiny-lib STATIC ${SOURCE_FILES} ${HEADER_FILES})

find_package(Boost REQUIRED COMPONENTS thread)
find_package(spdlog CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
target_link_libraries(tiny-lib PUBLIC Boost::boost Boost::thread spdlog::spdlog OpenSSL::SSL OpenSSL::Crypto)

target_include_directories(tiny-lib PUBLIC
    $<BUILD_INTERFACE:${BASE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_precompile_headers(tiny-lib PRIVATE "${BASE_DIR}/pch.hpp")

set(TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test")

file(GLOB_RECURSE TEST_SOURCE_FILES CONFIGURE_DEPENDS "${TEST_DIR}/*.cpp")
file(GLOB_RECURSE TEST_HEADER_FILES CONFIGURE_DEPENDS "${TEST_DIR}/*.hpp")
list(FILTER TEST_HEADER_FILES EXCLUDE REGEX "${TEST_DIR}/pch.hpp")

add_executable(tiny-test "${TEST_DIR}/main.cpp" ${TEST_SOURCE_FILES})

find_package(GTest REQUIRED)
target_link_libraries(tiny-test PRIVATE GTest::gtest GTest::gtest_main)

target_include_directories(tiny-test PUBLIC "${TEST_DIR}")

target_link_libraries(tiny-test PUBLIC tiny-lib)

target_precompile_headers(tiny-test PRIVATE "${TEST_DIR}/pch.hpp")

include(GoogleTest)
gtest_discover_tests(tiny-test)
