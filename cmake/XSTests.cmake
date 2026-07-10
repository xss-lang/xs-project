# SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
# SPDX-License-Identifier: Apache-2.0

function(xs_add_c_test test_name source_file library_name)
  get_filename_component(target_stem "${source_file}" NAME_WE)
  string(REGEX REPLACE "_tests$" "" target_stem "${target_stem}")
  add_executable(xs_${target_stem}_tests "${source_file}")
  target_link_libraries(xs_${target_stem}_tests PRIVATE "${library_name}")
  add_test(NAME "${test_name}" COMMAND xs_${target_stem}_tests ${ARGN})
  set_tests_properties("${test_name}" PROPERTIES TIMEOUT 5)
endfunction()

if(XS_BUILD_PROJECT_XS OR XS_BUILD_PROJECT_XSPROJ)
  xs_add_c_test(project tests/project_tests.c xsproj)
endif()

if(NOT XS_BUILD_PROJECT_XS)
  return()
endif()

add_test(NAME cli_version COMMAND xs --version)
string(REPLACE "." "\\." XS_PROJECT_VERSION_REGEX "${PROJECT_VERSION}")
set_tests_properties(cli_version PROPERTIES TIMEOUT 5 PASS_REGULAR_EXPRESSION "xs ${XS_PROJECT_VERSION_REGEX}")

xs_add_c_test(lexer tests/lexer_tests.c xs_compiler)
xs_add_c_test(parser tests/parser_tests.c xs_compiler)

add_test(NAME example_project COMMAND xs check -proj ${XS_SOURCE_FROM_BINARY}/tests/fixtures/example_project/MyApp.xsproj)
set_tests_properties(example_project PROPERTIES TIMEOUT 5)
add_test(NAME macro_project COMMAND xs check -proj ${XS_SOURCE_FROM_BINARY}/tests/fixtures/macro_project/MacroApp.xsproj)
set_tests_properties(macro_project PROPERTIES TIMEOUT 5)

foreach(output hir mir xlil)
  add_test(NAME build_output_${output} COMMAND xs build --output ${output} -proj ${XS_SOURCE_FROM_BINARY}/tests/fixtures/example_project/MyApp.xsproj)
  set_tests_properties(build_output_${output} PROPERTIES TIMEOUT 5 WILL_FAIL TRUE)
  add_test(NAME build_file_output_${output} COMMAND xs build --output ${output} -file ${XS_SOURCE_FROM_BINARY}/tests/fixtures/example_project/source/Main.xs)
  set_tests_properties(build_file_output_${output} PROPERTIES TIMEOUT 5 WILL_FAIL TRUE)
  add_test(NAME build_file_short_${output} COMMAND xs build --${output} -file ${XS_SOURCE_FROM_BINARY}/tests/fixtures/example_project/source/Main.xs)
  set_tests_properties(build_file_short_${output} PROPERTIES TIMEOUT 5 WILL_FAIL TRUE)
endforeach()

foreach(format xhir xmir xlil)
  add_test(NAME direct_${format}_unsupported_version COMMAND xs build --${format} -file ${XS_SOURCE_FROM_BINARY}/tests/fixtures/intermediate/Unsupported.${format})
  set_tests_properties(direct_${format}_unsupported_version PROPERTIES TIMEOUT 5 WILL_FAIL TRUE)
endforeach()

set(XS_DIRECT_XLIL_FIXTURE_DIR "${CMAKE_CURRENT_BINARY_DIR}/tests/fixtures/intermediate")
file(MAKE_DIRECTORY "${XS_DIRECT_XLIL_FIXTURE_DIR}")
foreach(entry_fixture Supported BranchExit CallExit MissingMain ExternMain ParameterizedMain VoidMain I64Main
                      DuplicateMain)
  configure_file(tests/fixtures/intermediate/${entry_fixture}.xlil
                 "${XS_DIRECT_XLIL_FIXTURE_DIR}/${entry_fixture}.xlil" COPYONLY)
endforeach()

add_test(NAME direct_xlil_supported_version COMMAND xs build --xlil -file ${XS_DIRECT_XLIL_FIXTURE_DIR}/Supported.xlil)
set_tests_properties(direct_xlil_supported_version PROPERTIES TIMEOUT 5
                     PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_executable(xs_xse_artifact_tests tests/xse_artifact_tests.c)
add_test(NAME direct_xlil_xse_artifacts COMMAND xs_xse_artifact_tests ${XS_DIRECT_XLIL_FIXTURE_DIR}/Supported.ll
                                             ${XS_DIRECT_XLIL_FIXTURE_DIR}/Supported.o
                                             ${XS_DIRECT_XLIL_FIXTURE_DIR}/Supported.xse 0)
set_tests_properties(direct_xlil_xse_artifacts PROPERTIES DEPENDS direct_xlil_supported_version TIMEOUT 5)
add_test(NAME direct_xlil_native_exit_code COMMAND ${XS_DIRECT_XLIL_FIXTURE_DIR}/Supported.xse)
set_tests_properties(direct_xlil_native_exit_code PROPERTIES DEPENDS direct_xlil_supported_version TIMEOUT 5)
add_test(NAME direct_xlil_branch_exit_build COMMAND xs build --xlil -file ${XS_DIRECT_XLIL_FIXTURE_DIR}/BranchExit.xlil)
set_tests_properties(direct_xlil_branch_exit_build PROPERTIES TIMEOUT 5
                     PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME direct_xlil_branch_exit_artifacts COMMAND xs_xse_artifact_tests
                                                    ${XS_DIRECT_XLIL_FIXTURE_DIR}/BranchExit.ll
                                                    ${XS_DIRECT_XLIL_FIXTURE_DIR}/BranchExit.o
                                                    ${XS_DIRECT_XLIL_FIXTURE_DIR}/BranchExit.xse 7)
set_tests_properties(direct_xlil_branch_exit_artifacts PROPERTIES DEPENDS direct_xlil_branch_exit_build TIMEOUT 5)
add_test(NAME direct_xlil_call_exit_build COMMAND xs build --xlil -file ${XS_DIRECT_XLIL_FIXTURE_DIR}/CallExit.xlil)
set_tests_properties(direct_xlil_call_exit_build PROPERTIES TIMEOUT 5
                     PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME direct_xlil_call_exit_artifacts COMMAND xs_xse_artifact_tests
                                                  ${XS_DIRECT_XLIL_FIXTURE_DIR}/CallExit.ll
                                                  ${XS_DIRECT_XLIL_FIXTURE_DIR}/CallExit.o
                                                  ${XS_DIRECT_XLIL_FIXTURE_DIR}/CallExit.xse 7)
set_tests_properties(direct_xlil_call_exit_artifacts PROPERTIES DEPENDS direct_xlil_call_exit_build TIMEOUT 5)
foreach(entry_fixture MissingMain ExternMain ParameterizedMain VoidMain I64Main DuplicateMain)
  add_test(NAME direct_xlil_invalid_${entry_fixture} COMMAND xs build --xlil -file ${XS_DIRECT_XLIL_FIXTURE_DIR}/${entry_fixture}.xlil)
  set_tests_properties(direct_xlil_invalid_${entry_fixture} PROPERTIES TIMEOUT 5 WILL_FAIL TRUE)
endforeach()

add_executable(xs_backend_tests tests/backend_tests.c)
target_link_libraries(xs_backend_tests PRIVATE xs_backend_llvm)
add_test(NAME backend COMMAND xs_backend_tests ${XS_BACKEND_TEST_OBJECT} ${XS_LLD_EXECUTABLE})
set_tests_properties(backend PROPERTIES TIMEOUT 10)

xs_add_c_test(modules tests/module_tests.c xs_compiler ${XS_SOURCE_FROM_BINARY}/tests/fixtures/module_project
              ${XS_SOURCE_FROM_BINARY}/tests/fixtures/duplicate_modules)
xs_add_c_test(hir tests/hir_tests.c xs_compiler)
xs_add_c_test(hir_members tests/hir_member_tests.c xs_compiler)
xs_add_c_test(hir_macro tests/hir_macro_tests.c xs_compiler)
xs_add_c_test(hir_types tests/hir_type_tests.c xs_compiler)
xs_add_c_test(hir_expressions tests/hir_expression_tests.c xs_compiler)
xs_add_c_test(xlil tests/xlil_tests.c xs_compiler)
xs_add_c_test(mir tests/mir_tests.c xs_compiler)
xs_add_c_test(syntax_ast tests/syntax_ast_tests.c xs_compiler)
xs_add_c_test(syntax_decl tests/syntax_decl_tests.c xs_compiler)
xs_add_c_test(syntax_macro tests/syntax_macro_tests.c xs_compiler)
xs_add_c_test(source_include tests/source_include_tests.c xs_compiler)
xs_add_c_test(syntax_macro_view tests/syntax_macro_view_tests.c xs_compiler)
