cmake_minimum_required(VERSION 3.10)
project(sdl-celeste C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)

set(PROJECT_ROOT ${CMAKE_SOURCE_DIR}/..)
set(LIBFIXMATH_DIR ${PROJECT_ROOT}/components/libfixmath)
set(GAME_DIR ${PROJECT_ROOT}/main)

add_library(libfixmath STATIC
    ${LIBFIXMATH_DIR}/fix16.c
    ${LIBFIXMATH_DIR}/fix16_exp.c
    ${LIBFIXMATH_DIR}/fix16_sqrt.c
    ${LIBFIXMATH_DIR}/fix16_str.c
    ${LIBFIXMATH_DIR}/fix16_trig.c
    ${LIBFIXMATH_DIR}/fract32.c
    ${LIBFIXMATH_DIR}/uint32.c
)
target_include_directories(libfixmath PUBLIC ${LIBFIXMATH_DIR})
target_compile_definitions(libfixmath PRIVATE FIXMATH_NO_CACHE)

add_executable(sdl-celeste
    main.c
    platform.c
    ${GAME_DIR}/classic.c
    ${GAME_DIR}/pico8sim.c
    ${GAME_DIR}/sprite.c
    ${GAME_DIR}/tilemap.c
)
target_include_directories(sdl-celeste PRIVATE ${GAME_DIR} ${CMAKE_SOURCE_DIR})
target_compile_options(sdl-celeste PRIVATE -include ${CMAKE_SOURCE_DIR}/compat.h)
target_link_libraries(sdl-celeste PRIVATE SDL2::SDL2 libfixmath)
