set(BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "${BASE_DIR}/*.cpp")
file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS "${BASE_DIR}/*.hpp")
list(FILTER HEADER_FILES EXCLUDE REGEX "${BASE_DIR}/pch.hpp")

add_library(tiny-lib STATIC ${SOURCE_FILES} ${HEADER_FILES})

find_package(Boost REQUIRED COMPONENTS thread)
find_package(spdlog CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
target_link_libraries(tiny-lib PRIVATE Boost::boost Boost::thread spdlog::spdlog OpenSSL::SSL OpenSSL::Crypto)

target_include_directories(tiny-lib PUBLIC
    $<BUILD_INTERFACE:${BASE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_precompile_headers(tiny-lib PRIVATE "${BASE_DIR}/pch.hpp")
