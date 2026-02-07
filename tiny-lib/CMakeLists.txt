find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS algorithm thread)

file(GLOB_RECURSE TINY_LIB_HEADERS *.hpp)
file(GLOB_RECURSE TINY_LIB_SOURCES *.cpp)

if(APPLE)
    file(GLOB_RECURSE TINY_LIB_OBJCXX_SOURCES *.mm)
    list(APPEND TINY_LIB_SOURCES ${TINY_LIB_OBJCXX_SOURCES})
endif()

list(FILTER TINY_LIB_SOURCES EXCLUDE REGEX ".*\\.metal$")

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

if(APPLE)
    find_library(METAL_FRAMEWORK Metal REQUIRED)
    find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)

    target_link_libraries(tiny-lib PUBLIC
        ${METAL_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
    )

    set(METAL_SHADER_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/mining/metal/mining_kernel.metal")
    set(METAL_SHADER_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/mining/metal/mining_kernel_source.inc")

    add_custom_command(
        OUTPUT ${METAL_SHADER_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/mining/metal"
        COMMAND python3 "${CMAKE_SOURCE_DIR}/cmake/embed_shader.py" "${METAL_SHADER_INPUT}" "${METAL_SHADER_OUTPUT}"
        DEPENDS ${METAL_SHADER_INPUT} "${CMAKE_SOURCE_DIR}/cmake/embed_shader.py"
        COMMENT "Embedding Metal shader source"
    )

    add_custom_target(embed_metal_shader DEPENDS ${METAL_SHADER_OUTPUT})
    add_dependencies(tiny-lib embed_metal_shader)

    target_include_directories(tiny-lib PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

    set_source_files_properties(
        "${CMAKE_CURRENT_SOURCE_DIR}/mining/metal/metal_mining_backend.mm"
        PROPERTIES
            SKIP_PRECOMPILE_HEADERS ON
            COMPILE_FLAGS "-fobjc-arc"
    )
endif()

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
