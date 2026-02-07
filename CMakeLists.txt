cmake_minimum_required(VERSION 3.30)

project(tiny-coin LANGUAGES CXX)

if(APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

add_subdirectory(tiny-lib)
add_subdirectory(tiny-sandbox)
add_subdirectory(tiny-test)
add_subdirectory(tiny-bench)
