add_library(commons-logging logging_common.c)

pkg_check_modules(PMLOG PmLogLib)
if (PMLOG_FOUND)
    target_include_directories(commons-logging SYSTEM PRIVATE ${PMLOG_INCLUDE_DIRS})
    target_link_libraries(commons-logging PRIVATE ${PMLOG_LIBRARIES})
    target_sources(commons-logging PRIVATE logging_pmlog.c)
else ()
    target_sources(commons-logging PRIVATE logging_stdio.c)
endif ()

if (COMMONS_LOGGING_SDL)
    target_include_directories(commons-logging SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS})
    target_sources(commons-logging PRIVATE logging_ext_sdl.c)
    target_compile_definitions(commons-logging PUBLIC COMMONS_LOGGING_SDL=1)
endif ()

if (COMMONS_LOGGING_SS4S)
    target_link_libraries(commons-logging PRIVATE ss4s)
    target_sources(commons-logging PRIVATE logging_ext_ss4s.c)
    target_compile_definitions(commons-logging PUBLIC COMMONS_LOGGING_SS4S=1)
endif ()

if (COMMONS_LOGGING_LVGL)
    target_sources(commons-logging PRIVATE logging_ext_lvgl.c)
    target_compile_definitions(commons-logging PUBLIC COMMONS_LOGGING_LVGL=1)
endif ()

target_include_directories(commons-logging PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})