cmake_minimum_required(VERSION 3.10)
project(ZephyrLog VERSION 1.0.0 LANGUAGES CXX)

# Generate compile_commands.json for IDE use
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ========== Options ==========
option(BUILD_SHARED_LIBS "Build shared library instead of static" ON)
option(ZEPHYRLOG_BUILD_EXAMPLES "Build example programs" ON)

# ========== Compile options ==========
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -O2)
endif()

# ========== Library ==========
set(ZEPHYRLOG_SOURCES
    src/zephyrlog.cpp
    src/logline.cpp
    src/logger.cpp
    src/asynclogger.cpp
    src/ringbuffer.cpp
    src/queuebuffer.cpp
)

add_library(zephyrlog ${ZEPHYRLOG_SOURCES})

target_include_directories(zephyrlog PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(zephyrlog PUBLIC pthread)

# Symbol visibility: hide all symbols by default, export only those
# annotated with ZEPHYRLOG_EXPORT. This keeps the shared-library ABI
# surface minimal.
set_target_properties(zephyrlog PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_compile_definitions(zephyrlog PRIVATE ZEPHYRLOG_BUILD)

# ========== Examples ==========
if(ZEPHYRLOG_BUILD_EXAMPLES)
    add_executable(test examples/test.cpp)
    target_link_libraries(test zephyrlog pthread)
endif()

# ========== Install (SDK packaging) ==========
include(GNUInstallDirs)

install(TARGETS zephyrlog
    EXPORT ZephyrLogTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}   # .a  (static)
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}   # .so (shared)
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}   # .dll
)

install(DIRECTORY include/zephyrlog
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT ZephyrLogTargets
    NAMESPACE zephyrlog::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ZephyrLog
)

# Generate version file for find_package
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/ZephyrLogConfig.cmake.in
    ${PROJECT_BINARY_DIR}/ZephyrLogConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ZephyrLog
)

write_basic_package_version_file(
    ${PROJECT_BINARY_DIR}/ZephyrLogConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${PROJECT_BINARY_DIR}/ZephyrLogConfig.cmake
    ${PROJECT_BINARY_DIR}/ZephyrLogConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ZephyrLog
)
