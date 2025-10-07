######################################################################
# @author      : Ruan E. Formigoni (ruanformigoni@gmail.com)
# @file        : CMakeLists
######################################################################

cmake_minimum_required(VERSION 3.20)

project(FlatImage DESCRIPTION "FlatImage" LANGUAGES CXX)

# Include cmake files
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(Variables)
include(Doxygen)
include(MkDocs)

# Compiler defaults for all targets
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER /usr/bin/c++)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")

# Avoid in-source builds
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(FATAL_ERROR "In-source builds are not allowed")
endif()

# Tools
## XXD
find_program(XXD_COMMAND xxd)
if(NOT XXD_COMMAND)
  message(FATAL_ERROR "xxd command not found")
endif()
## Git
find_package(Git REQUIRED)

# Required variables
## Distribution
require_variable_env(FIM_DIST)
## Reserved size for persistent flatimage configuration
require_variable_env(FIM_RESERVED_SIZE)
## Metadata information about binary dependencies
require_variable_env(FIM_FILE_META)
## File with json tools included in the final FlatImage
require_variable_env(FIM_FILE_TOOLS)
## Version
execute_process(
  COMMAND ${GIT_EXECUTABLE} -c safe.directory=${CMAKE_SOURCE_DIR} describe --tags --abbrev=0
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE FIM_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
require_variable_cmake(FIM_VERSION)
## Timestamp
string(TIMESTAMP FIM_TIMESTAMP "%Y%m%d%H%M%S")
require_variable_cmake(FIM_TIMESTAMP)
## Commit
execute_process(
  COMMAND ${GIT_EXECUTABLE} -c safe.directory=${CMAKE_SOURCE_DIR} rev-parse --short HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE FIM_GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
require_variable_cmake(FIM_GIT_COMMIT_HASH)

# External libraries
find_package(nlohmann_json REQUIRED)
cmake_policy(SET CMP0167 NEW)
find_package(Boost REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)

# Main executable
add_executable(boot src/boot/boot.cpp)
target_link_libraries(boot PRIVATE
  nlohmann_json::nlohmann_json
  /usr/lib/libturbojpeg.a
  /usr/lib/libpng.a
  /usr/lib/libcom_err.a
  /usr/lib/libz.a
)
target_compile_options(boot PRIVATE --std=gnu++23 -static -Wall -Os -Wextra
  -Wformat=2 -Wold-style-cast -Wcast-align -Wnull-dereference -Wdouble-promotion
  -Wlogical-op -Wduplicated-cond -Wuseless-cast -Wredundant-decls
  -Wmissing-include-dirs -Wmissing-field-initializers -Wempty-body
  -Wimplicit-fallthrough -Wswitch-enum -Wzero-as-null-pointer-constant
)
target_link_options(boot PRIVATE -static)
target_compile_definitions(boot PRIVATE
  FIM_DIST="$ENV{FIM_DIST}"
  FIM_RESERVED_SIZE=${FIM_RESERVED_SIZE}
  FIM_FILE_TOOLS="${FIM_FILE_TOOLS}"
  FIM_FILE_META="${FIM_FILE_META}"
  FIM_VERSION="${FIM_VERSION}"
  FIM_TIMESTAMP="${FIM_TIMESTAMP}"
  FIM_COMMIT="${FIM_GIT_COMMIT_HASH}"
)

# Magic patcher
add_executable(magic src/magic/magic.cpp)
target_compile_options(magic PRIVATE --std=gnu++23 -Wall -Os -Wextra)
target_link_options(magic PRIVATE -static)