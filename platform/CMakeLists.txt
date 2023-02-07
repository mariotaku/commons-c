pkg_check_modules(LUNA_SERVICE2 luna-service2)
if (LUNA_SERVICE2_FOUND)
    add_subdirectory(webos)
endif ()