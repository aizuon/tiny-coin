set(BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "${BASE_DIR}/*.cpp")
file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS "${BASE_DIR}/*.hpp")
list(FILTER HEADER_FILES EXCLUDE REGEX "${BASE_DIR}/pch.hpp")

add_executable(tiny-sandbox "${BASE_DIR}/main.cpp")

find_package(Boost REQUIRED COMPONENTS program_options)
target_link_libraries(tiny-sandbox PRIVATE Boost::program_options)

target_include_directories(tiny-sandbox PUBLIC "${BASE_DIR}")

target_link_libraries(tiny-sandbox PUBLIC tiny-lib)

target_precompile_headers(tiny-sandbox PRIVATE "${BASE_DIR}/pch.hpp")
