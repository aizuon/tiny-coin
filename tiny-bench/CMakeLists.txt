file(GLOB_RECURSE TINY_BENCH_SOURCES src/*.cpp)

add_executable(tiny-bench ${TINY_BENCH_SOURCES})

target_link_libraries(tiny-bench
    PRIVATE
        tiny-lib
)

if(MSVC)
    target_compile_options(tiny-bench PRIVATE /W3 /permissive-)
else()
    target_compile_options(tiny-bench PRIVATE -Wall -Wextra -Wpedantic)
endif()
