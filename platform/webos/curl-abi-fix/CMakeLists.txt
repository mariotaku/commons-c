add_subdirectory(libcurl.so.5)

add_library(commons-curl-abi-fix SHARED lib.c)

target_link_libraries(commons-curl-abi-fix PUBLIC curl-abi-5)
set_target_properties(commons-curl-abi-fix PROPERTIES LIBRARY_OUTPUT_NAME curl SOVERSION 4)
