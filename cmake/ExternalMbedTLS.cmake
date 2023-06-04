include(ExternalProject)

set(EXT_MBEDTLS_TOOLCHAIN_ARGS)
if (CMAKE_TOOLCHAIN_FILE)
    list(APPEND EXT_MBEDTLS_TOOLCHAIN_ARGS "-DCMAKE_TOOLCHAIN_FILE:string=${CMAKE_TOOLCHAIN_FILE}")
endif ()
if (CMAKE_TOOLCHAIN_ARGS)
    list(APPEND EXT_MBEDTLS_TOOLCHAIN_ARGS "-DCMAKE_TOOLCHAIN_ARGS:string=${CMAKE_TOOLCHAIN_ARGS}")
endif ()

ExternalProject_Add(ext_mbedtls
        URL https://github.com/Mbed-TLS/mbedtls/archive/refs/tags/v3.4.0.tar.gz
        CMAKE_ARGS ${EXT_MBEDTLS_TOOLCHAIN_ARGS}
        -DCMAKE_BUILD_TYPE:string=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DENABLE_PROGRAMS=OFF
        -DENABLE_TESTING=OFF
        -DUSE_SHARED_MBEDTLS_LIBRARY=ON
        -DUSE_STATIC_MBEDTLS_LIBRARY=OFF
        )
ExternalProject_Get_Property(ext_mbedtls INSTALL_DIR)

add_library(ext_mbedcrypto_target SHARED IMPORTED)
set_target_properties(ext_mbedcrypto_target PROPERTIES IMPORTED_LOCATION ${INSTALL_DIR}/lib/libmbedcrypto.so)
add_dependencies(ext_mbedcrypto_target ext_mbedtls)
set(MBEDCRYPTO_LIBRARY ext_mbedcrypto_target)

add_library(ext_mbedx509_target SHARED IMPORTED)
set_target_properties(ext_mbedx509_target PROPERTIES IMPORTED_LOCATION ${INSTALL_DIR}/lib/libmbedx509.so)
add_dependencies(ext_mbedx509_target ext_mbedtls)
set(MBEDX509_LIBRARY ext_mbedx509_target)

add_library(ext_mbedtls_target SHARED IMPORTED)
set_target_properties(ext_mbedtls_target PROPERTIES IMPORTED_LOCATION ${INSTALL_DIR}/lib/libmbedtls.so)
add_dependencies(ext_mbedtls_target ext_mbedtls)
set(MBEDTLS_LIBRARY ext_mbedtls_target)

set(MBEDTLS_INCLUDE_DIRS ${INSTALL_DIR}/include)

install(DIRECTORY ${INSTALL_DIR}/lib/ DESTINATION lib)
