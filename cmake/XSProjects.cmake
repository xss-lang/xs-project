# SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
# SPDX-License-Identifier: Apache-2.0

if(XS_BUILD_PROJECT_XS OR XS_BUILD_PROJECT_XSPROJ)
  add_library(xsproj
    xs/sources/diagnostic.c
    xsproj/sources/lexer.c
    xsproj/sources/model.c
    xsproj/sources/parser.c
    xsproj/sources/parser_main.c
  )
  target_include_directories(xsproj PUBLIC include xsproj/include)
  target_compile_options(xsproj PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
  target_compile_options(xsproj PUBLIC -include "${XS_COMPILER_CHECK_HEADER}")
endif()

if(NOT XS_BUILD_PROJECT_XS)
  return()
endif()

find_package(LLVM REQUIRED CONFIG)
find_library(XS_LLVM_LIBRARY NAMES LLVM-${LLVM_VERSION_MAJOR} LLVM HINTS ${LLVM_LIBRARY_DIRS} REQUIRED)
find_package(Threads REQUIRED)
find_program(XS_CARGO_EXECUTABLE NAMES cargo REQUIRED)

set(XS_XSLANG_TARGET_DIR "${CMAKE_CURRENT_BINARY_DIR}/xslang-target")
set(XS_XSLANG_STATIC_LIBRARY "${XS_XSLANG_TARGET_DIR}/debug/libxslang.a")
file(GLOB_RECURSE XS_XSLANG_RUST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/xslang/sources/*.rs")
add_custom_command(
  OUTPUT "${XS_XSLANG_STATIC_LIBRARY}"
  COMMAND "${XS_CARGO_EXECUTABLE}" build --lib --target-dir "${XS_XSLANG_TARGET_DIR}"
  DEPENDS ${XS_XSLANG_RUST_SOURCES} xslang/Cargo.toml xslang/rust-toolchain.toml
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/xslang"
  COMMENT "Building the Rust compiler core"
  VERBATIM
)
add_custom_target(xslang_compiler_core_build DEPENDS "${XS_XSLANG_STATIC_LIBRARY}")
add_library(xslang_compiler_core STATIC IMPORTED GLOBAL)
set_target_properties(xslang_compiler_core PROPERTIES IMPORTED_LOCATION "${XS_XSLANG_STATIC_LIBRARY}")
add_dependencies(xslang_compiler_core xslang_compiler_core_build)
target_link_libraries(xslang_compiler_core INTERFACE Threads::Threads ${CMAKE_DL_LIBS} m)

add_library(xs_compiler
  xs/sources/ast.c
  xs/sources/codegen/units.c
  xs/sources/compiler_core/syntax_packet.c
  xs/sources/driver/cli.c
  xs/sources/driver/compiler_core_native.c
  xs/sources/driver/direct_xlil.c
  xs/sources/driver/options.c
  xs/sources/driver/source_native.c
  xs/sources/driver/source_native_static.c
  xs/sources/driver/source_native_update.c
  xs/sources/driver/source_native_program.c
  xs/sources/driver/source_native_utils.c
  xs/sources/lexer.c
  xs/sources/macro/expanded_view.c
  xs/sources/macro/expansion.c
  xs/sources/macro/fragment.c
  xs/sources/macro/reparse.c
  xs/sources/macro/rewrite.c
  xs/sources/macro/validation.c
  xs/sources/mir/borrow_checker.c
  xs/sources/mir/hir_lowering.c
  xs/sources/mir/model_blocks.c
  xs/sources/mir/model.c
  xs/sources/mir/model_writer.c
  xs/sources/mir/optimizer.c
  xs/sources/mir/xlil_lowering.c
  xs/sources/mono/plan.c
  xs/sources/hir/cffi.c
  xs/sources/hir/dump.c
  xs/sources/hir/expression_check.c
  xs/sources/hir/expression_check_api.c
  xs/sources/hir/expression_check_string.c
  xs/sources/hir/result_constructor.c
  xs/sources/hir/module_graph.c
  xs/sources/hir/module_model.c
  xs/sources/hir/module_registry.c
  xs/sources/hir/import_resolver.c
  xs/sources/hir/inheritance.c
  xs/sources/hir/name_resolution.c
  xs/sources/hir/symbol_table.c
  xs/sources/hir/syntax_helpers.c
  xs/sources/hir/type_info.c
  xs/sources/hir/type_resolution.c
  xs/sources/hir/type_resolution_macro_view.c
  xs/sources/parser.c
  xs/sources/source_include.c
  xs/sources/syntax_ast.c
  xs/sources/syntax_parser.c
  xs/sources/syntax/parser_macro.c
  xs/sources/syntax/parser_declaration.c
  xs/sources/syntax/parser_expression.c
  xs/sources/syntax/parser_statement.c
  xs/sources/syntax/parser_type.c
  xs/sources/token.c
)

add_library(xs_lil
  xs/sources/xlil/memory.c
  xs/sources/xlil/model.c
  xs/sources/xlil/model_float.c
  xs/sources/xlil/parser.c
  xs/sources/xlil/verify.c
  xs/sources/xlil/writer.c
)

target_include_directories(xs_compiler PUBLIC include xs/include xsproj/include)
target_include_directories(xs_lil PUBLIC include xs/include)
target_link_libraries(xs_compiler PUBLIC xsproj xs_lil PRIVATE xslang_compiler_core)
target_compile_definitions(xs_compiler PRIVATE XS_PROJECT_VERSION="${PROJECT_VERSION}"
                                            XS_CLANG_EXECUTABLE="${CMAKE_C_COMPILER}")
if(CMAKE_C_COMPILER_TARGET)
  target_compile_definitions(xs_compiler PRIVATE XS_CONFIGURED_TARGET_TRIPLE="${CMAKE_C_COMPILER_TARGET}")
endif()
target_compile_options(xs_compiler PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
target_compile_options(xs_lil PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
target_compile_options(xs_compiler PUBLIC -include "${XS_COMPILER_CHECK_HEADER}")
target_compile_options(xs_lil PUBLIC -include "${XS_COMPILER_CHECK_HEADER}")

add_executable(xs xs/sources/main.c)
target_link_libraries(xs PRIVATE xs_compiler)

add_library(xs_backend_llvm
  xs/sources/backend/linker.c
  xs/sources/backend/llvm_backend.c
)
target_include_directories(xs_backend_llvm PUBLIC include xs/include PRIVATE ${LLVM_INCLUDE_DIRS})
target_link_libraries(xs_backend_llvm PUBLIC xs_lil PRIVATE ${XS_LLVM_LIBRARY})
target_compile_options(xs_backend_llvm PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
target_compile_options(xs_backend_llvm PUBLIC -include "${XS_COMPILER_CHECK_HEADER}")
target_link_libraries(xs PRIVATE xs_backend_llvm)
