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

# Each test includes evmouse.c to exercise its private listener state.
foreach (test_name evmouse evmouse_hotplug)
    add_executable(tests_${test_name} ${test_name}_tests.c)
    target_include_directories(tests_${test_name} SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(tests_${test_name} PRIVATE commons-logging ${SDL2_LIBRARIES})
    add_test(tests_${test_name} tests_${test_name})
endforeach ()
set_tests_properties(tests_evmouse_hotplug PROPERTIES TIMEOUT 10)
