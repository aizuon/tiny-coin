cmake_minimum_required(VERSION 3.25)

set(CMAKE_TOOLCHAIN_FILE "/Users/dorukhantokay/Developer/vcpkg/scripts/buildsystems/vcpkg.cmake"
  CACHE STRING "Vcpkg toolchain file")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(tiny-coin)

add_subdirectory(tiny-lib)
add_subdirectory(tiny-sandbox)
add_subdirectory(tiny-test)
