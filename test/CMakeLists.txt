######################################################################
# @author      : Ruan E. Formigoni (ruanformigoni@gmail.com)
# @file        : CMakeLists (test)
######################################################################

cmake_minimum_required(VERSION 4.0)

# Compiler settings
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER /usr/bin/c++)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "Test Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Test CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

# Enable testing
enable_testing()

# Fetch doctest
include(FetchContent)
FetchContent_Declare(
  doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG v2.4.12
)
FetchContent_MakeAvailable(doctest)

# Find optional packages (needed for some library headers)
# Most tests only need standard library, so make these optional
find_package(nlohmann_json REQUIRED)
find_package(Boost REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)

# Include directories for source code
include_directories(${CMAKE_SOURCE_DIR}/src)

# Common compile options for all tests
set(TEST_COMPILE_OPTIONS
  -Wall -Wextra -g
  -Wformat=2 -Wcast-align -Wnull-dereference
  -Wlogical-op -Wduplicated-cond
  -Wmissing-include-dirs -Wempty-body
  -Wimplicit-fallthrough -Wswitch-enum
)

# Function to create a test executable
function(add_doctest_executable test_name test_file)
  add_executable(${test_name} ${test_file})
  target_link_libraries(${test_name} PRIVATE doctest::doctest)
  # Link libraries
  target_link_libraries(${test_name} PRIVATE nlohmann_json::nlohmann_json)
  target_link_libraries(${test_name} PRIVATE Boost::boost)
  target_link_libraries(${test_name} PRIVATE JPEG::JPEG)
  target_link_libraries(${test_name} PRIVATE PNG::PNG)
  target_link_libraries(${test_name} PRIVATE ZLIB::ZLIB)
  # Set compiler options
  target_compile_options(${test_name} PRIVATE ${TEST_COMPILE_OPTIONS} --std=gnu++23)
  # Add test
  add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

# src/std/ tests
add_doctest_executable(test_vector src/std/test_vector.cpp)
add_doctest_executable(test_string src/std/test_string.cpp)
add_doctest_executable(test_filesystem src/std/test_filesystem.cpp)
add_doctest_executable(test_expected src/std/test_expected.cpp)
add_doctest_executable(test_enum src/std/test_enum.cpp)
add_doctest_executable(test_concept src/std/test_concept.cpp)

# src/lib/ tests
add_doctest_executable(test_env src/lib/test_env.cpp)
add_doctest_executable(test_log src/lib/test_log.cpp)
add_doctest_executable(test_elf src/lib/test_elf.cpp)
add_doctest_executable(test_linux src/lib/test_linux.cpp)
add_doctest_executable(test_subprocess src/lib/test_subprocess.cpp)
add_doctest_executable(test_fuse src/lib/test_fuse.cpp)
add_doctest_executable(test_image src/lib/test_image.cpp)

# Print test information
message(STATUS "Test executables will be built in: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "Run tests with: ctest --output-on-failure")
message(STATUS "Or run individual tests: ./test_<name>")
