cmake_minimum_required(VERSION 3.20)

project(xs_compiler VERSION 0.1.0 LANGUAGES C)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  message(FATAL_ERROR "The X# compiler does not support the GNU C compiler; use the Clang preset.")
endif()

if(CMAKE_GENERATOR MATCHES "Makefiles")
  message(FATAL_ERROR "The X# compiler does not use Makefiles; use the Clang + Ninja preset.")
endif()

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(xs_compiler
  sources/ast.c
  sources/diagnostic.c
  sources/lexer.c
  sources/parser.c
  sources/project.c
  sources/token.c
)

target_include_directories(xs_compiler PUBLIC include)
target_compile_options(xs_compiler PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

add_executable(xs sources/main.c)
target_link_libraries(xs PRIVATE xs_compiler)

include(CTest)
if(BUILD_TESTING)
  add_executable(xs_lexer_tests tests/lexer_tests.c)
  target_link_libraries(xs_lexer_tests PRIVATE xs_compiler)
  add_test(NAME lexer COMMAND xs_lexer_tests)
  set_tests_properties(lexer PROPERTIES TIMEOUT 5)

  add_executable(xs_parser_tests tests/parser_tests.c)
  target_link_libraries(xs_parser_tests PRIVATE xs_compiler)
  add_test(NAME parser COMMAND xs_parser_tests)
  set_tests_properties(parser PROPERTIES TIMEOUT 5)

  add_executable(xs_project_tests tests/project_tests.c)
  target_link_libraries(xs_project_tests PRIVATE xs_compiler)
  add_test(NAME project COMMAND xs_project_tests)
  set_tests_properties(project PROPERTIES TIMEOUT 5)

  add_test(NAME example_project COMMAND xs check -proj ${CMAKE_SOURCE_DIR}/XS/example/MyApp.xsproj)
  set_tests_properties(example_project PROPERTIES TIMEOUT 5)
endif()
