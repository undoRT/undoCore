cmake_minimum_required(VERSION 3.20)

project(undoCore
    VERSION 0.2.0
    DESCRIPTION "undoRT shared core - header-only"
    LANGUAGES CXX
)

# Target INTERFACE: no compile, header-only
add_library(undoCore INTERFACE)
add_library(undoCore::undoCore ALIAS undoCore)

target_include_directories(undoCore INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(undoCore INTERFACE cxx_std_20)

# Installation (for find_package in other repos)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS undoCore EXPORT undoCoreTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_package_config_file(
    cmake/undoCoreConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/undoCoreConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/undoCore
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/undoCoreConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMinorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/undoCoreConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/undoCoreConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/undoCore
)

install(EXPORT undoCoreTargets
    NAMESPACE undoCore::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/undoCore
)