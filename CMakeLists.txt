cmake_minimum_required(VERSION 3.24)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED)

project(sound_sdl3)

# Create an option to switch between a system sdl library and a vendored SDL library
option(MYGAME_VENDORED "Use vendored libraries" ON)

if(MYGAME_VENDORED)
    # This assumes you have added SDL as a submodule in vendored/SDL
    add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
	# Statically link SDL3
	set(SDL_SHARED OFF CACHE BOOL "Build SDL shared library" FORCE)
	set(SDL_STATIC ON CACHE BOOL "Build SDL static library" FORCE)
else()
    # 1. Look for a SDL3 package,
    # 2. look for the SDL3-shared component, and
    # 3. fail if the shared component cannot be found.
    find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
endif()

add_executable(scanlines_pixelart src/scanlines_pixelart.c)
add_executable(sound_keyboard src/sound_keyboard.c)

target_link_libraries(scanlines_pixelart PRIVATE SDL3::SDL3)
target_link_libraries(sound_keyboard PRIVATE SDL3::SDL3)
