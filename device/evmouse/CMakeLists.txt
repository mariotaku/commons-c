find_path(LINUX_INPUT_HEADER_DIR linux/input.h)

if (NOT LINUX_INPUT_HEADER_DIR)
    return()
endif ()

pkg_check_modules(SDL2 REQUIRED sdl2)

add_library(commons-evmouse evmouse.c)
target_include_directories(commons-evmouse PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(commons-evmouse SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(commons-evmouse PUBLIC commons-logging ${SDL2_LIBRARIES})

if (NOT CMAKE_TESTING_ENABLED)
    return()
endif ()

# The test #includes evmouse.c so it can construct an evmouse_t with no fds
# (the public evmouse_open_default returns NULL without a /dev/input device).
# Linking against commons-evmouse would duplicate the symbols, so the test
# compiles evmouse.c via the include and only links the logging / SDL deps.
add_executable(tests_evmouse evmouse_tests.c)
target_include_directories(tests_evmouse SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(tests_evmouse PRIVATE commons-logging ${SDL2_LIBRARIES})
add_test(tests_evmouse tests_evmouse)