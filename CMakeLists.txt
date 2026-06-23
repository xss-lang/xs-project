cmake_minimum_required(VERSION 3.20)

find_program(XS_LLVM_AR_EXECUTABLE NAMES llvm-ar REQUIRED)
find_program(XS_LLVM_RANLIB_EXECUTABLE NAMES llvm-ranlib REQUIRED)
find_program(XS_LLVM_NM_EXECUTABLE NAMES llvm-nm REQUIRED)
find_program(XS_LLVM_OBJCOPY_EXECUTABLE NAMES llvm-objcopy REQUIRED)
find_program(XS_LLVM_OBJDUMP_EXECUTABLE NAMES llvm-objdump REQUIRED)
find_program(XS_LLVM_STRIP_EXECUTABLE NAMES llvm-strip REQUIRED)
find_program(XS_LLD_EXECUTABLE NAMES ld.lld REQUIRED)

set(CMAKE_AR "${XS_LLVM_AR_EXECUTABLE}" CACHE FILEPATH "LLVM archiver" FORCE)
set(CMAKE_RANLIB "${XS_LLVM_RANLIB_EXECUTABLE}" CACHE FILEPATH "LLVM archive indexer" FORCE)
set(CMAKE_NM "${XS_LLVM_NM_EXECUTABLE}" CACHE FILEPATH "LLVM symbol tool" FORCE)
set(CMAKE_OBJCOPY "${XS_LLVM_OBJCOPY_EXECUTABLE}" CACHE FILEPATH "LLVM object copier" FORCE)
set(CMAKE_OBJDUMP "${XS_LLVM_OBJDUMP_EXECUTABLE}" CACHE FILEPATH "LLVM object dumper" FORCE)
set(CMAKE_STRIP "${XS_LLVM_STRIP_EXECUTABLE}" CACHE FILEPATH "LLVM strip tool" FORCE)
set(CMAKE_LINKER "${XS_LLD_EXECUTABLE}" CACHE FILEPATH "LLVM linker" FORCE)
set(CMAKE_C_COMPILER_AR "${XS_LLVM_AR_EXECUTABLE}" CACHE FILEPATH "Clang archiver" FORCE)
set(CMAKE_C_COMPILER_RANLIB "${XS_LLVM_RANLIB_EXECUTABLE}" CACHE FILEPATH "Clang archive indexer" FORCE)

project(xs_compiler VERSION 0.1.0 LANGUAGES C)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  message(FATAL_ERROR "The X# compiler does not support the GNU C compiler; use the Clang preset.")
endif()

if(NOT CMAKE_GENERATOR MATCHES "^Ninja")
  message(FATAL_ERROR "The X# compiler requires Ninja; GNU Make and other generators are unsupported.")
endif()

if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR "The X# compiler requires Clang.")
endif()

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(LLVM REQUIRED CONFIG)
find_library(XS_LLVM_LIBRARY NAMES LLVM-${LLVM_VERSION_MAJOR} LLVM HINTS ${LLVM_LIBRARY_DIRS} REQUIRED)

add_link_options(-fuse-ld=lld)

add_library(xs_compiler
  sources/ast.c
  sources/diagnostic.c
  sources/lexer.c
  sources/macro/validation.c
  sources/hir/module_graph.c
  sources/hir/module_model.c
  sources/hir/module_registry.c
  sources/parser.c
  sources/project.c
  sources/project/lexer.c
  sources/project/model.c
  sources/syntax_ast.c
  sources/syntax_parser.c
  sources/syntax/parser_macro.c
  sources/syntax/parser_declaration.c
  sources/syntax/parser_expression.c
  sources/syntax/parser_statement.c
  sources/syntax/parser_type.c
  sources/token.c
)

target_include_directories(xs_compiler PUBLIC include)
target_compile_options(xs_compiler PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
target_compile_options(xs_compiler PUBLIC -include $(CMAKE_SOURCE_DIR)/include/xs/compiler_check.h)

add_executable(xs sources/main.c)
target_link_libraries(xs PRIVATE xs_compiler)

add_library(xs_backend_llvm
  sources/backend/linker.c
  sources/backend/llvm_backend.c
)
target_include_directories(xs_backend_llvm PUBLIC include PRIVATE ${LLVM_INCLUDE_DIRS})
target_link_libraries(xs_backend_llvm PRIVATE ${XS_LLVM_LIBRARY})
target_compile_options(xs_backend_llvm PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
target_compile_options(xs_backend_llvm PUBLIC -include ${CMAKE_SOURCE_DIR}/include/xs/compiler_check.h)

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

  add_executable(xs_backend_tests tests/backend_tests.c)
  target_link_libraries(xs_backend_tests PRIVATE xs_backend_llvm)
  add_test(NAME backend COMMAND xs_backend_tests ${CMAKE_CURRENT_BINARY_DIR}/xs_backend_test.o ${XS_LLD_EXECUTABLE})
  set_tests_properties(backend PROPERTIES TIMEOUT 10)

  add_executable(xs_module_tests tests/module_tests.c)
  target_link_libraries(xs_module_tests PRIVATE xs_compiler)
  add_test(NAME modules COMMAND xs_module_tests ${CMAKE_SOURCE_DIR}/tests/fixtures/module_project
                                      ${CMAKE_SOURCE_DIR}/tests/fixtures/duplicate_modules)
  set_tests_properties(modules PROPERTIES TIMEOUT 5)

  add_executable(xs_syntax_ast_tests tests/syntax_ast_tests.c)
  target_link_libraries(xs_syntax_ast_tests PRIVATE xs_compiler)
  add_test(NAME syntax_ast COMMAND xs_syntax_ast_tests)
  set_tests_properties(syntax_ast PROPERTIES TIMEOUT 5)
endif()
