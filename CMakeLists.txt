cmake_minimum_required(VERSION 3.20)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(XSMonorepo)
xs_monorepo_validate_projects()
xs_monorepo_validate_runtimes()
xs_monorepo_project_enabled(xs XS_BUILD_PROJECT_XS)
xs_monorepo_project_enabled(xsproj XS_BUILD_PROJECT_XSPROJ)
if(NOT XS_BUILD_PROJECT_XS AND NOT XS_BUILD_PROJECT_XSPROJ)
  message(FATAL_ERROR "This checkout currently has buildable monorepo projects: 'xs' and 'xsproj'.")
endif()

find_program(XS_LLVM_AR_EXECUTABLE NAMES llvm-ar REQUIRED)
find_program(XS_LLVM_RANLIB_EXECUTABLE NAMES llvm-ranlib REQUIRED)
find_program(XS_LLVM_NM_EXECUTABLE NAMES llvm-nm REQUIRED)
find_program(XS_LLVM_OBJCOPY_EXECUTABLE NAMES llvm-objcopy REQUIRED)
find_program(XS_LLVM_OBJDUMP_EXECUTABLE NAMES llvm-objdump REQUIRED)
find_program(XS_LLVM_STRIP_EXECUTABLE NAMES llvm-strip REQUIRED)
find_program(XS_LLD_EXECUTABLE NAMES ld.lld REQUIRED)

function(xs_require_tool_basename variable expected)
  get_filename_component(tool_name "${${variable}}" NAME)
  if(NOT tool_name STREQUAL "${expected}")
    message(FATAL_ERROR "The X# compiler refuses GNU-compatible fallback tool '${tool_name}'; expected '${expected}'.")
  endif()
endfunction()

xs_require_tool_basename(XS_LLVM_AR_EXECUTABLE llvm-ar)
xs_require_tool_basename(XS_LLVM_RANLIB_EXECUTABLE llvm-ranlib)
xs_require_tool_basename(XS_LLVM_NM_EXECUTABLE llvm-nm)
xs_require_tool_basename(XS_LLVM_OBJCOPY_EXECUTABLE llvm-objcopy)
xs_require_tool_basename(XS_LLVM_OBJDUMP_EXECUTABLE llvm-objdump)
xs_require_tool_basename(XS_LLVM_STRIP_EXECUTABLE llvm-strip)
xs_require_tool_basename(XS_LLD_EXECUTABLE ld.lld)

set(CMAKE_AR "${XS_LLVM_AR_EXECUTABLE}" CACHE FILEPATH "LLVM archiver" FORCE)
set(CMAKE_RANLIB "${XS_LLVM_RANLIB_EXECUTABLE}" CACHE FILEPATH "LLVM archive indexer" FORCE)
set(CMAKE_NM "${XS_LLVM_NM_EXECUTABLE}" CACHE FILEPATH "LLVM symbol tool" FORCE)
set(CMAKE_OBJCOPY "${XS_LLVM_OBJCOPY_EXECUTABLE}" CACHE FILEPATH "LLVM object copier" FORCE)
set(CMAKE_OBJDUMP "${XS_LLVM_OBJDUMP_EXECUTABLE}" CACHE FILEPATH "LLVM object dumper" FORCE)
set(CMAKE_STRIP "${XS_LLVM_STRIP_EXECUTABLE}" CACHE FILEPATH "LLVM strip tool" FORCE)
set(CMAKE_LINKER "${XS_LLD_EXECUTABLE}" CACHE FILEPATH "LLVM linker" FORCE)
set(CMAKE_C_COMPILER_AR "${XS_LLVM_AR_EXECUTABLE}" CACHE FILEPATH "Clang archiver" FORCE)
set(CMAKE_C_COMPILER_RANLIB "${XS_LLVM_RANLIB_EXECUTABLE}" CACHE FILEPATH "Clang archive indexer" FORCE)

project(xs_project VERSION 0.1.0 LANGUAGES C)

file(RELATIVE_PATH XS_SOURCE_FROM_BINARY "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
cmake_path(NORMAL_PATH XS_SOURCE_FROM_BINARY)
if(XS_SOURCE_FROM_BINARY STREQUAL "")
  set(XS_SOURCE_FROM_BINARY ".")
endif()
set(XS_COMPILER_CHECK_HEADER "${XS_SOURCE_FROM_BINARY}/include/xs/compiler_check.h")
set(XS_BACKEND_TEST_OBJECT "xs_backend_test.o")

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  message(FATAL_ERROR "The X# compiler does not support the GNU C compiler; use the Clang preset.")
endif()

get_filename_component(XS_C_COMPILER_NAME "${CMAKE_C_COMPILER}" NAME)
if(XS_C_COMPILER_NAME MATCHES "^(gcc|g\\+\\+|cc)$")
  message(FATAL_ERROR "The X# compiler refuses GNU-style compiler driver '${XS_C_COMPILER_NAME}'; use clang.")
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

if(CMAKE_C_FLAGS MATCHES "(^| )-std=gnu" OR CMAKE_C_FLAGS_DEBUG MATCHES "(^| )-std=gnu" OR
   CMAKE_C_FLAGS_RELEASE MATCHES "(^| )-std=gnu")
  message(FATAL_ERROR "GNU C dialect flags are unsupported; use strict C23.")
endif()

add_link_options(-fuse-ld=lld)

if(XS_BUILD_PROJECT_XS OR XS_BUILD_PROJECT_XSPROJ)
  add_library(xsproj
    xs/sources/diagnostic.c
    xsproj/sources/lexer.c
    xsproj/sources/model.c
    xsproj/sources/parser.c
  )
  target_include_directories(xsproj PUBLIC include xsproj/include)
  target_compile_options(xsproj PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
  target_compile_options(xsproj PUBLIC -include "${XS_COMPILER_CHECK_HEADER}")
endif()

if(XS_BUILD_PROJECT_XS)
  find_package(LLVM REQUIRED CONFIG)
  find_library(XS_LLVM_LIBRARY NAMES LLVM-${LLVM_VERSION_MAJOR} LLVM HINTS ${LLVM_LIBRARY_DIRS} REQUIRED)

  add_library(xs_compiler
    xs/sources/ast.c
    xs/sources/codegen/units.c
    xs/sources/driver/cli.c
    xs/sources/lexer.c
    xs/sources/macro/expanded_view.c
    xs/sources/macro/expansion.c
    xs/sources/macro/fragment.c
    xs/sources/macro/reparse.c
    xs/sources/macro/validation.c
    xs/sources/mir/borrow_checker.c
    xs/sources/mir/hir_lowering.c
    xs/sources/mir/model_blocks.c
    xs/sources/mir/model.c
    xs/sources/mir/model_writer.c
    xs/sources/mir/optimizer.c
    xs/sources/mir/xlil_lowering.c
    xs/sources/mono/plan.c
    xs/sources/xlil/writer.c
    xs/sources/hir/dump.c
    xs/sources/hir/module_graph.c
    xs/sources/hir/module_model.c
    xs/sources/hir/module_registry.c
    xs/sources/hir/import_resolver.c
    xs/sources/hir/name_resolution.c
    xs/sources/hir/symbol_table.c
    xs/sources/hir/syntax_helpers.c
    xs/sources/hir/type_info.c
    xs/sources/hir/type_resolution.c
    xs/sources/parser.c
    xs/sources/syntax_ast.c
    xs/sources/syntax_parser.c
    xs/sources/syntax/parser_macro.c
    xs/sources/syntax/parser_declaration.c
    xs/sources/syntax/parser_expression.c
    xs/sources/syntax/parser_statement.c
    xs/sources/syntax/parser_type.c
    xs/sources/token.c
    xs/sources/xlil/model.c
  )

  target_include_directories(xs_compiler PUBLIC include xs/include xsproj/include)
  target_link_libraries(xs_compiler PUBLIC xsproj)
  target_compile_options(xs_compiler PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
  target_compile_options(xs_compiler PUBLIC -include "${XS_COMPILER_CHECK_HEADER}")

  add_executable(xs xs/sources/main.c)
  target_link_libraries(xs PRIVATE xs_compiler)

  add_library(xs_backend_llvm
    xs/sources/backend/linker.c
    xs/sources/backend/llvm_backend.c
  )
  target_include_directories(xs_backend_llvm PUBLIC include xs/include PRIVATE ${LLVM_INCLUDE_DIRS})
  target_link_libraries(xs_backend_llvm PRIVATE ${XS_LLVM_LIBRARY})
  target_compile_options(xs_backend_llvm PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
  target_compile_options(xs_backend_llvm PUBLIC -include "${XS_COMPILER_CHECK_HEADER}")
endif()

include(CTest)
if(BUILD_TESTING)
  if(XS_BUILD_PROJECT_XS OR XS_BUILD_PROJECT_XSPROJ)
    add_executable(xs_project_tests tests/project_tests.c)
    target_link_libraries(xs_project_tests PRIVATE xsproj)
    add_test(NAME project COMMAND xs_project_tests)
    set_tests_properties(project PROPERTIES TIMEOUT 5)
  endif()

  if(XS_BUILD_PROJECT_XS)
    add_executable(xs_lexer_tests tests/lexer_tests.c)
    target_link_libraries(xs_lexer_tests PRIVATE xs_compiler)
    add_test(NAME lexer COMMAND xs_lexer_tests)
    set_tests_properties(lexer PROPERTIES TIMEOUT 5)

    add_executable(xs_parser_tests tests/parser_tests.c)
    target_link_libraries(xs_parser_tests PRIVATE xs_compiler)
    add_test(NAME parser COMMAND xs_parser_tests)
    set_tests_properties(parser PROPERTIES TIMEOUT 5)

    add_test(NAME example_project COMMAND xs check -proj ${XS_SOURCE_FROM_BINARY}/XS/example/MyApp.xsproj)
    set_tests_properties(example_project PROPERTIES TIMEOUT 5)
    add_test(NAME macro_project COMMAND xs check -proj ${XS_SOURCE_FROM_BINARY}/tests/fixtures/macro_project/MacroApp.xsproj)
    set_tests_properties(macro_project PROPERTIES TIMEOUT 5)

    add_test(NAME build_output_hir COMMAND xs build --output hir -proj ${XS_SOURCE_FROM_BINARY}/XS/example/MyApp.xsproj)
    set_tests_properties(build_output_hir PROPERTIES TIMEOUT 5 WILL_FAIL TRUE)

    add_test(NAME build_output_mir COMMAND xs build --output mir -proj ${XS_SOURCE_FROM_BINARY}/XS/example/MyApp.xsproj)
    set_tests_properties(build_output_mir PROPERTIES TIMEOUT 5 WILL_FAIL TRUE)

    add_test(NAME build_output_xlil COMMAND xs build --output xlil -proj ${XS_SOURCE_FROM_BINARY}/XS/example/MyApp.xsproj)
    set_tests_properties(build_output_xlil PROPERTIES TIMEOUT 5 WILL_FAIL TRUE)

    add_executable(xs_backend_tests tests/backend_tests.c)
    target_link_libraries(xs_backend_tests PRIVATE xs_backend_llvm)
    add_test(NAME backend COMMAND xs_backend_tests ${XS_BACKEND_TEST_OBJECT} ${XS_LLD_EXECUTABLE})
    set_tests_properties(backend PROPERTIES TIMEOUT 10)

    add_executable(xs_module_tests tests/module_tests.c)
    target_link_libraries(xs_module_tests PRIVATE xs_compiler)
    add_test(NAME modules COMMAND xs_module_tests ${XS_SOURCE_FROM_BINARY}/tests/fixtures/module_project
                                        ${XS_SOURCE_FROM_BINARY}/tests/fixtures/duplicate_modules)
    set_tests_properties(modules PROPERTIES TIMEOUT 5)

    add_executable(xs_hir_tests tests/hir_tests.c)
    target_link_libraries(xs_hir_tests PRIVATE xs_compiler)
    add_test(NAME hir COMMAND xs_hir_tests)
    set_tests_properties(hir PROPERTIES TIMEOUT 5)

    add_executable(xs_hir_macro_tests tests/hir_macro_tests.c)
    target_link_libraries(xs_hir_macro_tests PRIVATE xs_compiler)
    add_test(NAME hir_macro COMMAND xs_hir_macro_tests)
    set_tests_properties(hir_macro PROPERTIES TIMEOUT 5)

    add_executable(xs_hir_type_tests tests/hir_type_tests.c)
    target_link_libraries(xs_hir_type_tests PRIVATE xs_compiler)
    add_test(NAME hir_types COMMAND xs_hir_type_tests)
    set_tests_properties(hir_types PROPERTIES TIMEOUT 5)

    add_executable(xs_xlil_tests tests/xlil_tests.c)
    target_link_libraries(xs_xlil_tests PRIVATE xs_compiler)
    add_test(NAME xlil COMMAND xs_xlil_tests)
    set_tests_properties(xlil PROPERTIES TIMEOUT 5)

    add_executable(xs_mir_tests tests/mir_tests.c)
    target_link_libraries(xs_mir_tests PRIVATE xs_compiler)
    add_test(NAME mir COMMAND xs_mir_tests)
    set_tests_properties(mir PROPERTIES TIMEOUT 5)

    add_executable(xs_syntax_ast_tests tests/syntax_ast_tests.c)
    target_link_libraries(xs_syntax_ast_tests PRIVATE xs_compiler)
    add_test(NAME syntax_ast COMMAND xs_syntax_ast_tests)
    set_tests_properties(syntax_ast PROPERTIES TIMEOUT 5)

    add_executable(xs_syntax_decl_tests tests/syntax_decl_tests.c)
    target_link_libraries(xs_syntax_decl_tests PRIVATE xs_compiler)
    add_test(NAME syntax_decl COMMAND xs_syntax_decl_tests)
    set_tests_properties(syntax_decl PROPERTIES TIMEOUT 5)

    add_executable(xs_syntax_macro_tests tests/syntax_macro_tests.c)
    target_link_libraries(xs_syntax_macro_tests PRIVATE xs_compiler)
    add_test(NAME syntax_macro COMMAND xs_syntax_macro_tests)
    set_tests_properties(syntax_macro PROPERTIES TIMEOUT 5)

    add_executable(xs_syntax_macro_view_tests tests/syntax_macro_view_tests.c)
    target_link_libraries(xs_syntax_macro_view_tests PRIVATE xs_compiler)
    add_test(NAME syntax_macro_view COMMAND xs_syntax_macro_view_tests)
    set_tests_properties(syntax_macro_view PROPERTIES TIMEOUT 5)
  endif()
endif()
