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

if(NOT APPLE)
    find_program(NVCC_EXECUTABLE nvcc PATHS "$ENV{CUDA_PATH}/bin" NO_CACHE)

    if(NVCC_EXECUTABLE)
        message(STATUS "Found CUDA compiler: ${NVCC_EXECUTABLE}")

        cmake_path(GET NVCC_EXECUTABLE PARENT_PATH _NVCC_BIN_DIR)
        cmake_path(GET _NVCC_BIN_DIR PARENT_PATH _CUDA_TOOLKIT_ROOT)
        if(NOT EXISTS "${_CUDA_TOOLKIT_ROOT}/include/cuda_runtime.h")
            cmake_path(GET _CUDA_TOOLKIT_ROOT PARENT_PATH _CUDA_TOOLKIT_ROOT)
        endif()

        set(_CUDA_INCLUDE_DIR "${_CUDA_TOOLKIT_ROOT}/include")
        set(_CUDA_KERNEL_SRC "${CMAKE_CURRENT_SOURCE_DIR}/mining/cuda/mining_kernel.cu")
        set(_CUDA_KERNEL_OBJ "${CMAKE_CURRENT_BINARY_DIR}/mining_kernel${CMAKE_CXX_OUTPUT_EXTENSION}")

        set(_NVCC_FLAGS -c -DTINY_COIN_CUDA --std=c++17 --allow-unsupported-compiler)
        list(APPEND _NVCC_FLAGS
            "$<$<CONFIG:Debug>:-G>"
            "$<$<CONFIG:Debug>:-g>"
            "$<$<NOT:$<CONFIG:Debug>>:-O2>"
        )

        if(MSVC)
            list(APPEND _NVCC_FLAGS
                -Xcompiler "/EHsc /W3"
                -Xcompiler "$<IF:$<CONFIG:Debug>,/MTd,/MT>"
            )
        endif()

        add_custom_command(
            OUTPUT "${_CUDA_KERNEL_OBJ}"
            COMMAND "${NVCC_EXECUTABLE}" ${_NVCC_FLAGS}
                -o "${_CUDA_KERNEL_OBJ}"
                "-I$<JOIN:$<TARGET_PROPERTY:tiny-lib,INCLUDE_DIRECTORIES>,;-I>"
                "-D$<JOIN:$<TARGET_PROPERTY:tiny-lib,COMPILE_DEFINITIONS>,;-D>"
                "-I${_CUDA_INCLUDE_DIR}"
                "${_CUDA_KERNEL_SRC}"
            DEPENDS "${_CUDA_KERNEL_SRC}"
                "${CMAKE_CURRENT_SOURCE_DIR}/mining/cuda/cuda_mining_backend.hpp"
                "${CMAKE_CURRENT_SOURCE_DIR}/mining/i_mining_backend.hpp"
                "${CMAKE_CURRENT_SOURCE_DIR}/util/log.hpp"
            COMMENT "Compiling CUDA mining kernel"
            VERBATIM
            COMMAND_EXPAND_LISTS
        )

        set_source_files_properties("${_CUDA_KERNEL_OBJ}" PROPERTIES
            EXTERNAL_OBJECT TRUE
            GENERATED TRUE
        )
        target_sources(tiny-lib PRIVATE "${_CUDA_KERNEL_OBJ}")

        set_source_files_properties("${_CUDA_KERNEL_SRC}" PROPERTIES
            HEADER_FILE_ONLY TRUE
        )
        target_sources(tiny-lib PRIVATE "${_CUDA_KERNEL_SRC}")

        target_compile_definitions(tiny-lib
            PUBLIC TINY_COIN_CUDA
            PRIVATE __NV_NO_HOST_COMPILER_CHECK
        )
        target_include_directories(tiny-lib PRIVATE "${_CUDA_INCLUDE_DIR}")

        if(WIN32)
            find_library(_CUDART_STATIC_LIB cudart_static
                PATHS "${_CUDA_TOOLKIT_ROOT}/lib/x64" NO_DEFAULT_PATH)
        else()
            find_library(_CUDART_STATIC_LIB cudart_static
                PATHS "${_CUDA_TOOLKIT_ROOT}/lib64" NO_DEFAULT_PATH)
        endif()

        if(_CUDART_STATIC_LIB)
            target_link_libraries(tiny-lib PUBLIC "${_CUDART_STATIC_LIB}")
        else()
            message(WARNING "CUDA runtime library (cudart_static) not found")
        endif()
    endif()
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
