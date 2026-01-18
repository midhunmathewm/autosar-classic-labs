#------------------------------------------------------------------------------
# Top-level CMake configuration for the ESP-IDF project
#
# - Sets the minimum required CMake version
# - Loads ESP-IDF CMake build system from IDF_PATH
# - Defines the project name used by ESP-IDF and build outputs
#
# All component-level build rules are defined in each component's
# CMakeLists.txt. This file only bootstraps the ESP-IDF build.
#------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(autosar_os_conformance)

