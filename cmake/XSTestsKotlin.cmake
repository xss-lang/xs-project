# SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
# SPDX-License-Identifier: Apache-2.0

add_test(NAME kotlin_project_call_build COMMAND xs build)
set_tests_properties(kotlin_project_call_build PROPERTIES TIMEOUT 60
  WORKING_DIRECTORY "${XS_PROJECT_NATIVE_FIXTURE_DIR}/native_call"
  ENVIRONMENT "XS_PROJECT_DRIVER=${XS_PROJECT_TEST_DRIVER}"
  FIXTURES_REQUIRED kotlin_project_resolver FIXTURES_SETUP kotlin_project_call_native
  PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME kotlin_project_call_artifacts COMMAND xs_xse_artifact_tests
  ${XS_PROJECT_NATIVE_FIXTURE_DIR}/native_call/sources/main.ll
  ${XS_PROJECT_NATIVE_FIXTURE_DIR}/native_call/sources/main.o
  ${XS_PROJECT_NATIVE_FIXTURE_DIR}/native_call/sources/main.xse 7)
set_tests_properties(kotlin_project_call_artifacts PROPERTIES TIMEOUT 5
  FIXTURES_REQUIRED kotlin_project_call_native)
add_test(NAME kotlin_project_recursive_build COMMAND xs build)
set_tests_properties(kotlin_project_recursive_build PROPERTIES TIMEOUT 60
  WORKING_DIRECTORY "${XS_PROJECT_NATIVE_FIXTURE_DIR}/recursive"
  ENVIRONMENT "XS_PROJECT_DRIVER=${XS_PROJECT_TEST_DRIVER}"
  FIXTURES_REQUIRED kotlin_project_resolver FIXTURES_SETUP kotlin_project_recursive
  PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME kotlin_project_recursive_artifacts COMMAND xs_xse_artifact_tests
  ${XS_PROJECT_NATIVE_FIXTURE_DIR}/recursive/sources/main.ll
  ${XS_PROJECT_NATIVE_FIXTURE_DIR}/recursive/sources/main.o
  ${XS_PROJECT_NATIVE_FIXTURE_DIR}/recursive/sources/main.xse 7
  "call i1 @is_even" "call i1 @is_odd")
set_tests_properties(kotlin_project_recursive_artifacts PROPERTIES
  TIMEOUT 5 FIXTURES_REQUIRED kotlin_project_recursive)
add_test(NAME kotlin_project_multi_file_native_build COMMAND xs build)
set_tests_properties(kotlin_project_multi_file_native_build PROPERTIES TIMEOUT 60
  WORKING_DIRECTORY "${XS_PROJECT_NATIVE_FIXTURE_DIR}/multi_file"
  ENVIRONMENT "XS_PROJECT_DRIVER=${XS_PROJECT_TEST_DRIVER}"
  FIXTURES_REQUIRED kotlin_project_resolver FIXTURES_SETUP kotlin_project_multi_file_native
  PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME kotlin_project_multi_file_native_artifacts COMMAND xs_xse_artifact_tests
  ${XS_PROJECT_NATIVE_FIXTURE_DIR}/multi_file/sources/main.ll
  ${XS_PROJECT_NATIVE_FIXTURE_DIR}/multi_file/sources/main.o
  ${XS_PROJECT_NATIVE_FIXTURE_DIR}/multi_file/sources/main.xse 7
  "call i32 @add")
set_tests_properties(kotlin_project_multi_file_native_artifacts PROPERTIES
  TIMEOUT 5 FIXTURES_REQUIRED kotlin_project_multi_file_native)
add_test(NAME kotlin_project_integer_widths_build COMMAND xs build)
set_tests_properties(kotlin_project_integer_widths_build PROPERTIES TIMEOUT 60
  WORKING_DIRECTORY "${XS_PROJECT_NATIVE_FIXTURE_DIR}/integer_widths"
  ENVIRONMENT "XS_PROJECT_DRIVER=${XS_PROJECT_TEST_DRIVER}"
  FIXTURES_REQUIRED kotlin_project_resolver FIXTURES_SETUP kotlin_project_integer_widths
  PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME kotlin_project_integer_widths_artifacts COMMAND xs_xse_artifact_tests
  ${XS_PROJECT_NATIVE_FIXTURE_DIR}/integer_widths/sources/main.ll
  ${XS_PROJECT_NATIVE_FIXTURE_DIR}/integer_widths/sources/main.o
  ${XS_PROJECT_NATIVE_FIXTURE_DIR}/integer_widths/sources/main.xse 0
  "define i128 @integer_min" "ret i128 -1")
set_tests_properties(kotlin_project_integer_widths_artifacts PROPERTIES
  TIMEOUT 5 FIXTURES_REQUIRED kotlin_project_integer_widths)
add_test(NAME kotlin_project_integer_operators_build COMMAND xs build)
set_tests_properties(kotlin_project_integer_operators_build PROPERTIES TIMEOUT 60
  WORKING_DIRECTORY "${XS_PROJECT_NATIVE_FIXTURE_DIR}/integer_operators"
  ENVIRONMENT "XS_PROJECT_DRIVER=${XS_PROJECT_TEST_DRIVER}"
  FIXTURES_REQUIRED kotlin_project_resolver FIXTURES_SETUP kotlin_project_integer_operators
  PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME kotlin_project_integer_operators_artifacts COMMAND xs_xse_artifact_tests
  ${XS_PROJECT_NATIVE_FIXTURE_DIR}/integer_operators/sources/main.ll
  ${XS_PROJECT_NATIVE_FIXTURE_DIR}/integer_operators/sources/main.o
  ${XS_PROJECT_NATIVE_FIXTURE_DIR}/integer_operators/sources/main.xse 0
  "sdiv i8" "udiv i64" "icmp ult i128" "icmp slt i128")
set_tests_properties(kotlin_project_integer_operators_artifacts PROPERTIES
  TIMEOUT 5 FIXTURES_REQUIRED kotlin_project_integer_operators)
add_test(NAME kotlin_project_module_check COMMAND xs check --module ./Modules)
set_tests_properties(kotlin_project_module_check PROPERTIES TIMEOUT 60
  WORKING_DIRECTORY "${XS_PROJECT_NATIVE_FIXTURE_DIR}/modules"
  ENVIRONMENT "XS_PROJECT_DRIVER=${XS_PROJECT_TEST_DRIVER}"
  FIXTURES_REQUIRED kotlin_project_resolver
  PASS_REGULAR_EXPRESSION "source\\[1\\].*Modules/Math/add.xs")
add_test(NAME kotlin_project_module_requires_root COMMAND xs check)
set_tests_properties(kotlin_project_module_requires_root PROPERTIES TIMEOUT 60 WILL_FAIL TRUE
  WORKING_DIRECTORY "${XS_PROJECT_NATIVE_FIXTURE_DIR}/modules"
  ENVIRONMENT "XS_PROJECT_DRIVER=${XS_PROJECT_TEST_DRIVER}"
  FIXTURES_REQUIRED kotlin_project_resolver)
set_tests_properties(
  kotlin_project_resolver_build
  kotlin_project_call_build kotlin_project_call_artifacts
  kotlin_project_recursive_build kotlin_project_recursive_artifacts
  kotlin_project_multi_file_native_build kotlin_project_multi_file_native_artifacts
  kotlin_project_integer_widths_build kotlin_project_integer_widths_artifacts
  kotlin_project_integer_operators_build kotlin_project_integer_operators_artifacts
  kotlin_project_module_check
  kotlin_project_module_requires_root
  PROPERTIES LABELS jvm)
