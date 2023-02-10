if (NOT TARGET inih)
    return()
endif()

add_library(commons-ss4s-modules-list ss4s_modules.c)
target_include_directories(commons-ss4s-modules-list PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(commons-ss4s-modules-list PUBLIC ss4s inih commons-os-info commons-array-list)