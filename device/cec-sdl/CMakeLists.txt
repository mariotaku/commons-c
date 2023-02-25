pkg_check_modules(LIBCEC libcec)

if (NOT LIBCEC_FOUND)
    message("libcec not found, disabling support")
    return()
endif ()

pkg_check_modules(SDL2 REQUIRED sdl2)

add_library(commons-cec-sdl cec_sdl.c cec_key.c)
target_include_directories(commons-cec-sdl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(commons-cec-sdl SYSTEM PRIVATE ${LIBCEC_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS})
target_link_libraries(commons-cec-sdl PUBLIC ${LIBCEC_LIBRARIES} ${SDL2_LIBRARIES})