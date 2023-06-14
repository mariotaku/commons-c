find_package(CURL)

if (NOT CURL_FOUND)
    return()
endif ()

pkg_check_modules(SDL2 REQUIRED sdl2)

add_library(commons-gamecontrollerdb-updater gamecontrollerdb_updater.c)
target_link_libraries(commons-gamecontrollerdb-updater PUBLIC ${CURL_LIBRARIES} ${SDL2_LIBRARIES} PRIVATE commons-logging)
target_include_directories(commons-gamecontrollerdb-updater SYSTEM PUBLIC ${CURL_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})