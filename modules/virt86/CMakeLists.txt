# virt86 master project.
#
# Include this to automatically include all platforms supported by your
# operating system. The project also defines a handy header file which includes
# all platform headers supported by your configuration:
#
#   #include "virt86/virt86.hpp"
#
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
project(virt86 VERSION ${CMAKE_PROJECT_VERSION} LANGUAGES CXX)

##############################
# Source files
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

# Export PDBs
if(MSVC)
    set(pdb_output_dir "${CMAKE_CURRENT_BINARY_DIR}/pdb-files")

    set(CMAKE_PDB_OUTPUT_DIRECTORY "${pdb_output_dir}")
    set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY "${pdb_output_dir}")

    get_cmake_property(is_multi GENERATOR_IS_MULTI_CONFIG)
    if(is_multi)
        set(config_suffix "$<CONFIG>")
    else()
        set(config_suffix "")
    endif()

    # Introduce variables:
    # * CMAKE_INSTALL_LIBDIR
    # * CMAKE_INSTALL_BINDIR
    include(GNUInstallDirs)

    if(BUILD_SHARED_LIBS)
        set(pdb_dst ${CMAKE_INSTALL_BINDIR})
    else()
        set(pdb_dst ${CMAKE_INSTALL_LIBDIR})
    endif()

    install(
        DIRECTORY "${pdb_output_dir}/${config_suffix}/"
        DESTINATION ${pdb_dst}
    )
endif()

add_library(virt86 STATIC ${sources} ${private_headers} ${public_headers})

set_target_properties(virt86 PROPERTIES DEBUG_POSTFIX "-debug")

target_include_directories(virt86
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Include core
if(TARGET virt86-core)
    add_library(virt86::virt86-core ALIAS virt86-core)
else()
    find_package(virt86-core CONFIG REQUIRED)
endif()
target_link_libraries(virt86 PUBLIC virt86::virt86-core)

# Include HAXM on all systems
if(TARGET virt86-haxm)
    add_library(virt86::virt86-haxm ALIAS virt86-haxm)
else()
    find_package(virt86-haxm CONFIG REQUIRED)
endif()
target_link_libraries(virt86 PUBLIC virt86::virt86-haxm)

# Include Windows Hypervisor Platform, KVM and Hypervisor.Framework on the respective systems
# Also include system-specific modules if available
if(WIN32)
    # WHPX requires Windows 10 SDK 10.0.17134.0 or later; link against it only if available
    if(TARGET virt86-whpx)
        add_library(virt86::virt86-whpx ALIAS virt86-whpx)
        set(virt86-whpx_FOUND TRUE)
    else()
        find_package(virt86-whpx CONFIG)
    endif()
    if(virt86-whpx_FOUND)
        target_link_libraries(virt86 PUBLIC virt86::virt86-whpx)
    endif()

    if(TARGET virt86-sys-windows)
        add_library(virt86::virt86-sys-windows ALIAS virt86-sys-windows)
    else()
        find_package(virt86-sys-windows CONFIG REQUIRED)
    endif()
    target_link_libraries(virt86 PUBLIC virt86::virt86-sys-windows)
elseif(LINUX)
    if(TARGET virt86-kvm)
        add_library(virt86::virt86-kvm ALIAS virt86-kvm)
    else()
        find_package(virt86-kvm CONFIG REQUIRED)
    endif()
    target_link_libraries(virt86 PUBLIC virt86::virt86-kvm)

    if(TARGET virt86-sys-linux)
        add_library(virt86::virt86-sys-linux ALIAS virt86-sys-linux)
    else()
        find_package(virt86-sys-linux CONFIG REQUIRED)
    endif()
    target_link_libraries(virt86 PUBLIC virt86::virt86-sys-linux)
elseif(APPLE)
    # TODO: Add support for Hypervisor.Framework
    #if(TARGET virt86-hvf)
    #    add_library(virt86::virt86-hvf ALIAS virt86-hvf)
    #else()
    #    find_package(virt86-hvf CONFIG REQUIRED)
    #endif()
    #target_link_libraries(virt86 PUBLIC virt86::virt86-hvf)

    if(TARGET virt86-sys-darwin)
        add_library(virt86::virt86-sys-darwin ALIAS virt86-sys-darwin)
    else()
        find_package(virt86-sys-darwin CONFIG REQUIRED)
    endif()
    target_link_libraries(virt86 PUBLIC virt86::virt86-sys-darwin)
endif()

if(MSVC)
    set_target_properties(virt86 PROPERTIES FOLDER Libraries)
endif()

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
install(TARGETS virt86
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
