cmake_minimum_required(VERSION 3.25)

if(DEFINED ENV{VCPKG_PATH})
  set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_PATH}/scripts/buildsystems/vcpkg.cmake
    CACHE STRING "Vcpkg toolchain file")
else()
  message(FATAL_ERROR "VCPKG_PATH environment variable is not set")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

project(tiny-coin)

add_subdirectory(tiny-lib)
add_subdirectory(tiny-sandbox)
