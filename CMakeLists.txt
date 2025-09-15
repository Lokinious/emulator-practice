cmake_minimum_required(VERSION 3.20)
project(gbaemu LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(GBAEMU_BUILD_TESTS "Build unit tests" ON)
option(GBAEMU_SDL2_FROM_FETCHCONTENT "Fetch SDL2 via CMake FetchContent" ON)

# SDL2
if(GBAEMU_SDL2_FROM_FETCHCONTENT)
  include(FetchContent)
  set(FETCHCONTENT_QUIET OFF)
  FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.30.8
    FIND_PACKAGE_ARGS NAMES SDL2
  )
  FetchContent_MakeAvailable(SDL2)
else()
  find_package(SDL2 REQUIRED CONFIG)
endif()

add_library(gba_core
    src/cpu/cpu.hpp
    src/cpu/cpu.cpp
    src/bus/bus.hpp
    src/bus/bus.cpp
    src/cart/rom.hpp
    src/cart/rom.cpp
    src/gba.hpp
    src/gba.cpp
)

target_include_directories(gba_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(gba_sdl
    src/frontend/sdl_main.cpp
)

target_link_libraries(gba_sdl PRIVATE gba_core SDL2::SDL2 SDL2::SDL2main)

# Ensure SDL2 runtime is next to the executable on Windows
if(WIN32)
  add_custom_command(TARGET gba_sdl POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> $<TARGET_FILE_DIR:gba_sdl>
  )
endif()

add_executable(romgen
    src/tools/romgen.cpp
)

target_link_libraries(romgen PRIVATE gba_core)

# Tests placeholder (add later)
if(GBAEMU_BUILD_TESTS)
  enable_testing()
endif()
