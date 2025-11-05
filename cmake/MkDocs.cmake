find_program(MKDOCS mkdocs REQUIRED)

add_custom_target(mkdocs
  ALL
  COMMAND ${MKDOCS} build
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/doc/mkdocs
  COMMENT "Running mkdocs..."
)