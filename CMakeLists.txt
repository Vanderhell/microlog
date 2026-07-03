cmake_minimum_required(VERSION 3.16)

project(microlog VERSION 1.0.0 LANGUAGES C)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(MICROLOG_BUILD_TESTS "Build microlog tests" OFF)

set(MLOG_MAX_BACKENDS 3 CACHE STRING "Maximum number of registered backends.")
set(MLOG_BUF_SIZE 128 CACHE STRING "Internal formatting buffer size.")
set(MLOG_LEVEL_MIN 0 CACHE STRING "Compile-time minimum log level.")
set(MLOG_ENABLE_COLOR 1 CACHE STRING "Enable ANSI color support.")
set(MLOG_ENABLE_TIMESTAMP 1 CACHE STRING "Enable timestamp support.")

add_library(microlog src/mlog.c)
add_library(microlog::microlog ALIAS microlog)

set_target_properties(microlog PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
    EXPORT_NAME microlog
)

target_include_directories(microlog
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_definitions(microlog
    PUBLIC
        MLOG_MAX_BACKENDS=${MLOG_MAX_BACKENDS}
        MLOG_BUF_SIZE=${MLOG_BUF_SIZE}
        MLOG_LEVEL_MIN=${MLOG_LEVEL_MIN}
        MLOG_ENABLE_COLOR=${MLOG_ENABLE_COLOR}
        MLOG_ENABLE_TIMESTAMP=${MLOG_ENABLE_TIMESTAMP}
)

if(MICROLOG_BUILD_TESTS)
    enable_testing()
    add_executable(microlog_test tests/test_all.c)
    target_link_libraries(microlog_test PRIVATE microlog)
    add_test(NAME microlog_test COMMAND microlog_test)
endif()

install(TARGETS microlog
    EXPORT micrologTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES include/mlog.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT micrologTargets
    NAMESPACE microlog::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/microlog
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/micrologConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/micrologConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/microlog
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/micrologConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/micrologConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/micrologConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/microlog
)
