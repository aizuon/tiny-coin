find_package(Boost REQUIRED COMPONENTS program_options)

file(GLOB_RECURSE TINY_SANDBOX_SOURCES src/*.cpp)

add_executable(tiny-sandbox ${TINY_SANDBOX_SOURCES})

target_link_libraries(tiny-sandbox
    PRIVATE
        tiny-lib
        Boost::program_options
)

if(MSVC)
    target_compile_options(tiny-sandbox PRIVATE /W3 /permissive-)
    target_compile_options(tiny-sandbox PRIVATE
        $<$<CONFIG:Release>:/O2 /Oi /GL /fp:fast /GS /Gy /arch:AVX2>
    )
else()
    target_compile_options(tiny-sandbox PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(tiny-sandbox PRIVATE
        $<$<CONFIG:Release>:-O3 -march=native -ffast-math>
    )
endif()
