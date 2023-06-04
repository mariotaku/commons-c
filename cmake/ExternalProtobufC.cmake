include(ExternalProject)

set(EXT_PROTOBUF_C_TOOLCHAIN_ARGS)
if (CMAKE_TOOLCHAIN_FILE)
    list(APPEND EXT_PROTOBUF_C_TOOLCHAIN_ARGS "-DCMAKE_TOOLCHAIN_FILE:string=${CMAKE_TOOLCHAIN_FILE}")
endif ()
if (CMAKE_TOOLCHAIN_ARGS)
    list(APPEND EXT_PROTOBUF_C_TOOLCHAIN_ARGS "-DCMAKE_TOOLCHAIN_ARGS:string=${CMAKE_TOOLCHAIN_ARGS}")
endif ()

ExternalProject_Add(ext_protobuf_c
        URL https://github.com/protobuf-c/protobuf-c/releases/download/v1.4.1/protobuf-c-1.4.1.tar.gz
        SOURCE_SUBDIR build-cmake
        CMAKE_ARGS ${EXT_PROTOBUF_C_TOOLCHAIN_ARGS}
        -DBUILD_PROTOC=OFF
        -DBUILD_SHARED_LIBS=ON
        -DCMAKE_BUILD_TYPE:string=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        )
ExternalProject_Get_Property(ext_protobuf_c INSTALL_DIR)

add_library(ext_protobuf_c_target SHARED IMPORTED)
set_target_properties(ext_protobuf_c_target PROPERTIES IMPORTED_LOCATION ${INSTALL_DIR}/lib/libprotobuf-c.so)

add_dependencies(ext_protobuf_c_target ext_protobuf_c)

set(PROTOBUF_C_INCLUDE_DIRS ${INSTALL_DIR}/include)
set(PROTOBUF_C_INCLUDEDIR ${INSTALL_DIR}/include)
set(PROTOBUF_C_LIBRARIES ext_protobuf_c_target)
set(PROTOBUF_C_FOUND TRUE)

install(DIRECTORY ${INSTALL_DIR}/lib/ DESTINATION lib)