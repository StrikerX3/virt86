# Windows Hypervisor Platform adapter.
#
# Supported by Windows only.
# Supports Intel VT-x and AMD-V.
# -------------------------------------------------------------------------------
# MIT License
# 
# Copyright (c) 2019 Ivan Roberto de Oliveira
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
project(virt86-whpx VERSION ${CMAKE_PROJECT_VERSION} LANGUAGES CXX)

if(${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION} VERSION_LESS 10.0.17134.0)
    message(WARNING "WHPX requires Windows SDK version 10.0.17134.0 or later. virt86-whpx will not be available.")
    message(STATUS "CMake reports Windows target platform version ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}.")
    return()
endif()

##############################
# Source files
#
file(GLOB_RECURSE sources
    src/*.cpp
)

file(GLOB_RECURSE private_headers
    src/*.hpp
    src/*.h
)

file(GLOB_RECURSE public_headers
    include/*.hpp
    include/*.h
)

##############################
# Project structure
#

add_library(virt86-whpx OBJECT ${sources} ${private_headers} ${public_headers})

# Include core
if(TARGET virt86-core)
    add_library(virt86::virt86-core ALIAS virt86-core)
else()
    find_package(virt86-core CONFIG REQUIRED)
endif()
target_link_libraries(virt86-whpx PUBLIC virt86::virt86-core)

# Include Windows utilities
if(TARGET virt86-sys-windows)
    add_library(virt86::virt86-sys-windows ALIAS virt86-sys-windows)
else()
    find_package(virt86-sys-windows CONFIG REQUIRED)
endif()
target_link_libraries(virt86-whpx PUBLIC virt86::virt86-sys-windows)

# Add include directories
target_include_directories(virt86-whpx
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Configure MSVC and precompiled header
if(MSVC)
    add_precompiled_header(virt86-whpx pch.hpp SOURCE_CXX "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.cpp" FORCEINCLUDE)

    vs_set_filters(BASE_DIR src FILTER_ROOT "Sources" SOURCES ${sources})
    vs_set_filters(BASE_DIR src FILTER_ROOT "Private Headers" SOURCES ${private_headers})
    vs_set_filters(BASE_DIR include FILTER_ROOT "Public Headers" SOURCES ${public_headers})

    vs_use_edit_and_continue()

    set_target_properties(virt86-whpx PROPERTIES FOLDER Libraries)
else()
    add_precompiled_header(virt86-whpx src/pch.hpp PCH_PATH pch.hpp SOURCE_CXX "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.cpp" FORCEINCLUDE)
endif()

# Tell dependents that WHPX is available via compiler definition
target_compile_definitions(virt86-whpx PUBLIC VIRT86_WHPX_AVAILABLE=1)

##############################
# Installation
#

# Configuration
set(config_install_dir "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(version_config "${generated_dir}/${PROJECT_NAME}ConfigVersion.cmake")
set(project_config "${generated_dir}/${PROJECT_NAME}Config.cmake")
set(TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets")
set(namespace "virt86::")

# Include module with function 'write_basic_package_version_file'
include(CMakePackageConfigHelpers)

# Configure '<PROJECT-NAME>ConfigVersion.cmake'
# Use:
#   * PROJECT_VERSION
write_basic_package_version_file(
    "${version_config}" COMPATIBILITY SameMajorVersion
)

# Configure '<PROJECT-NAME>Config.cmake'
# Use variables:
#   * TARGETS_EXPORT_NAME
#   * PROJECT_NAME
configure_package_config_file(
    "cmake/Config.cmake.in"
    "${project_config}"
    INSTALL_DESTINATION "${config_install_dir}"
)

# Install target library
install(TARGETS virt86-whpx
    EXPORT "${TARGETS_EXPORT_NAME}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

# Install public headers
install(DIRECTORY "include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

# Install CMake Config modules
#   * <prefix>/lib/cmake/<PROJECT-NAME>/<PROJECT-NAME>Config.cmake
#   * <prefix>/lib/cmake/<PROJECT-NAME>/<PROJECT-NAME>ConfigVersion.cmake
install(FILES "${project_config}" "${version_config}"
    DESTINATION "${config_install_dir}"
)

# Install CMake Target module
#   * <prefix>/lib/cmake/<PROJECT-NAME>/<PROJECT-NAME>Targets.cmake
install(EXPORT "${TARGETS_EXPORT_NAME}"
    NAMESPACE "${namespace}"
    DESTINATION "${config_install_dir}"
)
