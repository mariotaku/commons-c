cmake_minimum_required(VERSION 3.16)

project(commons-c C CXX)

find_package(PkgConfig REQUIRED)

include(CTest)

add_subdirectory(util)
add_subdirectory(platform)
add_subdirectory(os)
add_subdirectory(device)
add_subdirectory(media)
add_subdirectory(lvgl)
add_subdirectory(net)
add_subdirectory(io)