find_path(LINUX_INPUT_HEADER_DIR linux/input.h)

if (NOT LINUX_INPUT_HEADER_DIR)
    return()
endif ()

pkg_check_modules(SDL2 REQUIRED sdl2)

add_library(commons-gamepad-touchpad gamepad_touchpad.c)
target_include_directories(commons-gamepad-touchpad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(commons-gamepad-touchpad SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(commons-gamepad-touchpad PUBLIC commons-logging ${SDL2_LIBRARIES})
