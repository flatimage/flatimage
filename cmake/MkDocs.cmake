# Require mkdocs from venv
set(PYTHON_VENV "${CMAKE_SOURCE_DIR}/doc/mkdocs/env/bin/python3")
set(MKDOCS_VENV "${CMAKE_SOURCE_DIR}/doc/mkdocs/env/bin/mkdocs")

if(NOT EXISTS ${PYTHON_VENV})
  message(FATAL_ERROR "PYTHON_VENV not found at: ${PYTHON_VENV}")
endif()

if(NOT EXISTS ${MKDOCS_VENV})
  message(FATAL_ERROR "MKDOCS_VENV not found at: ${MKDOCS_VENV}")
endif()

if(NOT IS_EXECUTABLE ${MKDOCS_VENV})
  message(FATAL_ERROR "MKDOCS_VENV is not executable: ${MKDOCS_VENV}")
endif()

set(MKDOCS ${PYTHON_VENV} -m mkdocs)
message(STATUS "Using mkdocs from venv: ${PYTHON_VENV} -m mkdocs")

add_custom_target(mkdocs
  ALL
  COMMAND ${MKDOCS} build
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/doc/mkdocs
  COMMENT "Running mkdocs..."
)