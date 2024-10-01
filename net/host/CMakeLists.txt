add_library(commons-host host.c)
target_include_directories(commons-host PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(commons-host PUBLIC commons-sockaddr)

add_executable(tests_host host_tests.c)
target_link_libraries(tests_host PRIVATE commons-host)
add_test(tests_host tests_host)