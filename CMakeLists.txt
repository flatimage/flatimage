######################################################################
# @author      : Ruan E. Formigoni (ruanformigoni@gmail.com)
# @file        : CMakeLists
######################################################################

cmake_minimum_required(VERSION 3.20)

project(FlatImage DESCRIPTION "FlatImage" LANGUAGES CXX)

# Include cmake files
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(Variables)

# Conditional building based on FIM_TARGET
if(FIM_TARGET STREQUAL "boot")
  add_subdirectory(src)
elseif(FIM_TARGET STREQUAL "doxygen")
  include(Doxygen)
elseif(FIM_TARGET STREQUAL "mkdocs")
  include(MkDocs)
elseif(FIM_TARGET STREQUAL "test")
  enable_testing()
  add_subdirectory(test)
else()
  message(FATAL_ERROR "Invalid FIM_TARGET: ${FIM_TARGET}. Valid values are: boot, doxygen, mkdocs, test")
endif()