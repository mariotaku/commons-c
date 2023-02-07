pkg_check_modules(PBNJSON_C REQUIRED pbnjson_c)

add_library(commons-luna-sync lunasynccall.c)
target_include_directories(commons-luna-sync PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${PBNJSON_C_INCLUDE_DIRS})
target_link_libraries(commons-luna-sync PUBLIC ${PBNJSON_C_LIBRARIES} helpers)