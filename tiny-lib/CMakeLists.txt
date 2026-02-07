find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS algorithm thread)

file(GLOB_RECURSE TINY_LIB_HEADERS *.hpp)
file(GLOB_RECURSE TINY_LIB_SOURCES *.cpp)

add_library(tiny-lib STATIC ${TINY_LIB_SOURCES} ${TINY_LIB_HEADERS})

target_include_directories(tiny-lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_precompile_headers(tiny-lib PRIVATE pch.hpp)

target_compile_definitions(tiny-lib
    PUBLIC
        FMT_UNICODE=0
        SPDLOG_FMT_EXTERNAL
)

target_link_libraries(tiny-lib
    PUBLIC
        fmt::fmt
        spdlog::spdlog
        OpenSSL::SSL
        OpenSSL::Crypto
        Boost::algorithm
        Boost::thread
)

if(WIN32)
    target_compile_definitions(tiny-lib PUBLIC _WIN32_WINNT=0x0A00)
    target_link_libraries(tiny-lib PUBLIC crypt32 ws2_32)
endif()

if(MSVC)
    target_compile_options(tiny-lib PRIVATE /W3 /permissive-)
    target_compile_options(tiny-lib PRIVATE
        $<$<CONFIG:Release>:/O2 /Oi /GL /fp:fast /GS /Gy /arch:AVX2>
    )
else()
    target_compile_options(tiny-lib PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(tiny-lib PRIVATE
        $<$<CONFIG:Release>:-O3 -march=native -ffast-math>
    )
endif()
