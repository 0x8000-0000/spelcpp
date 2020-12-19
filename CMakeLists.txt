cmake_minimum_required(VERSION 3.18.0 FATAL_ERROR)

project(
  spelcpp
  VERSION 0.0.1
  LANGUAGES CXX
  DESCRIPTION "Spelling Check for C/C++ Source Code")

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_CXX_EXTENSIONS False)

add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_17)

add_library(project_warnings INTERFACE)
target_compile_options(project_warnings INTERFACE -Wall -Wextra -Werror
                                                  -pedantic -Wunused -Wshadow)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include(GoogleTest)
enable_testing()

add_subdirectory(src)
