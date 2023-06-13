find_package(PkgConfig REQUIRED)

pkg_check_modules(CURL_SYS REQUIRED libcurl)

add_library(curl-abi-5 SHARED lib.c)
set_target_properties(curl-abi-5 PROPERTIES LIBRARY_OUTPUT_NAME curl SOVERSION 5)
target_include_directories(curl-abi-5 SYSTEM PUBLIC ${CURL_SYS_INCLUDE_DIRS})
target_link_libraries(curl-abi-5 PRIVATE ${CURL_SYS_LIBRARIES})