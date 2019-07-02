# Top-level CMake project definitions.
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

# CMake 3.12 is required for target_link_libraries with OBJECT libraries
cmake_minimum_required(VERSION 3.12)

project(virt86 VERSION 1.2.0)

# Differentiate between Linux and macOS
if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

# Define properties and include helpers for Visual Studio
if(MSVC)
    include(cmake/VSHelpers.cmake)

    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    # Configure startup project
    set_property(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}" PROPERTY VS_STARTUP_PROJECT virt86-platform-check)
endif()

include(cmake/PrecompiledHeader.cmake)
include(GNUInstallDirs)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

# Require C++17 features
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Add modules and apps
add_subdirectory(modules)
add_subdirectory(apps)
