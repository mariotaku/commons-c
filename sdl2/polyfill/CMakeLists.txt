include(CheckSymbolExists)

set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
check_symbol_exists(RTLD_NEXT "dlfcn.h" HAVE_RTLD_NEXT)

if (NOT HAVE_RTLD_NEXT)
    message(STATUS "GNU compiler and glibc are required for sdl2-polyfill")
    return()
endif ()

find_package(PkgConfig REQUIRED)

pkg_check_modules(SDL2 REQUIRED sdl2)

add_library(commons-sdl2-polyfill STATIC SDL_polyfill_unresolved.c)

if (NOT DEFINED COMMONS_SDL2_POLYFILL_TARGET)
    set(COMMONS_SDL2_POLYFILL_TARGET "2.0.0")
endif ()

string(REPLACE "." ";" VERSION_LIST "${COMMONS_SDL2_POLYFILL_TARGET}")
list(GET VERSION_LIST 0 COMMONS_SDL2_POLYFILL_TARGET_MAJOR)
list(GET VERSION_LIST 1 COMMONS_SDL2_POLYFILL_TARGET_MINOR)
list(GET VERSION_LIST 2 COMMONS_SDL2_POLYFILL_TARGET_PATCH)


configure_file(SDL_polyfill_target.h.in SDL_polyfill_target.h @ONLY)

target_include_directories(commons-sdl2-polyfill PRIVATE ${SDL2_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(commons-sdl2-polyfill PRIVATE ${SDL2_LIBRARIES} dl)
target_compile_definitions(commons-sdl2-polyfill PRIVATE _GNU_SOURCE)
target_compile_options(commons-sdl2-polyfill PRIVATE -Wno-unused-variable)

add_subdirectory(joystick)