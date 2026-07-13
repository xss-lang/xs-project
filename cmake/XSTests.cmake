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
foreach(entry_fixture Supported BranchExit CallExit CompareExit CompareNotExit BitwiseExit StackSlotExit MissingMain ExternMain
                      ParameterizedMain VoidMain I64Main DuplicateMain)
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
add_test(NAME direct_xlil_compare_exit_build COMMAND xs build --xlil -file
                                               ${XS_DIRECT_XLIL_FIXTURE_DIR}/CompareExit.xlil)
set_tests_properties(direct_xlil_compare_exit_build PROPERTIES TIMEOUT 5
                     PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME direct_xlil_compare_exit_artifacts COMMAND xs_xse_artifact_tests
                                                     ${XS_DIRECT_XLIL_FIXTURE_DIR}/CompareExit.ll
                                                     ${XS_DIRECT_XLIL_FIXTURE_DIR}/CompareExit.o
                                                     ${XS_DIRECT_XLIL_FIXTURE_DIR}/CompareExit.xse 7)
set_tests_properties(direct_xlil_compare_exit_artifacts PROPERTIES DEPENDS direct_xlil_compare_exit_build TIMEOUT 5)
add_test(NAME direct_xlil_compare_not_exit_build COMMAND xs build --xlil -file
                                                   ${XS_DIRECT_XLIL_FIXTURE_DIR}/CompareNotExit.xlil)
set_tests_properties(direct_xlil_compare_not_exit_build PROPERTIES TIMEOUT 5
                     PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME direct_xlil_compare_not_exit_artifacts COMMAND xs_xse_artifact_tests
                                                         ${XS_DIRECT_XLIL_FIXTURE_DIR}/CompareNotExit.ll
                                                         ${XS_DIRECT_XLIL_FIXTURE_DIR}/CompareNotExit.o
                                                         ${XS_DIRECT_XLIL_FIXTURE_DIR}/CompareNotExit.xse 7)
set_tests_properties(direct_xlil_compare_not_exit_artifacts PROPERTIES DEPENDS direct_xlil_compare_not_exit_build
                                                                       TIMEOUT 5)
add_test(NAME direct_xlil_bitwise_exit_build COMMAND xs build --xlil -file
                                               ${XS_DIRECT_XLIL_FIXTURE_DIR}/BitwiseExit.xlil)
set_tests_properties(direct_xlil_bitwise_exit_build PROPERTIES TIMEOUT 5
                     PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME direct_xlil_bitwise_exit_artifacts COMMAND xs_xse_artifact_tests
                                                     ${XS_DIRECT_XLIL_FIXTURE_DIR}/BitwiseExit.ll
                                                     ${XS_DIRECT_XLIL_FIXTURE_DIR}/BitwiseExit.o
                                                     ${XS_DIRECT_XLIL_FIXTURE_DIR}/BitwiseExit.xse 6)
set_tests_properties(direct_xlil_bitwise_exit_artifacts PROPERTIES DEPENDS direct_xlil_bitwise_exit_build TIMEOUT 5)
add_test(NAME direct_xlil_stack_slot_exit_build COMMAND xs build --xlil -file
                                                ${XS_DIRECT_XLIL_FIXTURE_DIR}/StackSlotExit.xlil)
set_tests_properties(direct_xlil_stack_slot_exit_build PROPERTIES TIMEOUT 5
                     PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME direct_xlil_stack_slot_exit_artifacts COMMAND xs_xse_artifact_tests
                                                      ${XS_DIRECT_XLIL_FIXTURE_DIR}/StackSlotExit.ll
                                                      ${XS_DIRECT_XLIL_FIXTURE_DIR}/StackSlotExit.o
                                                      ${XS_DIRECT_XLIL_FIXTURE_DIR}/StackSlotExit.xse 7 "load i32")
set_tests_properties(direct_xlil_stack_slot_exit_artifacts PROPERTIES DEPENDS direct_xlil_stack_slot_exit_build
                                                                        TIMEOUT 5)
foreach(entry_fixture MissingMain ExternMain ParameterizedMain VoidMain I64Main DuplicateMain)
  add_test(NAME direct_xlil_invalid_${entry_fixture} COMMAND xs build --xlil -file ${XS_DIRECT_XLIL_FIXTURE_DIR}/${entry_fixture}.xlil)
  set_tests_properties(direct_xlil_invalid_${entry_fixture} PROPERTIES TIMEOUT 5 WILL_FAIL TRUE)
endforeach()

set(XS_SOURCE_NATIVE_FIXTURE_DIR "${CMAKE_CURRENT_BINARY_DIR}/tests/fixtures/source")
file(MAKE_DIRECTORY "${XS_SOURCE_NATIVE_FIXTURE_DIR}")
foreach(source_fixture MainReturn0 MainReturn7 MainArithmetic MainDivision MainRemainder MainNegative MainPositive
                       MainBitwise MainXor MainLocal MainLocalArithmetic MainLocalIf MainInferredLocal MainIf MainIfNot
                       MainIfFalse MainIfNotEqual MainBoolLocal MainBoolNotLocal MainInferredBoolLocal
                       MainInferredBoolNotLocal MainCall MainNestedCall MainLocalCall MainBoolCall MainBoolCallLocal
                       MainMutableLocal MainMutableBoolLocal MainIfAssignment MainCompoundAssignment
                       MainIfMultipleAssignments MainNestedIfAssignment MainWhile MainWhileControl MainDoWhile MainBlockLocals
                       MainEarlyReturn MainElseIf MainMatch MainMatchBool MainMatchExpression MainFor
                       MainPostfixDecrement MainUpdateValues
                       ImmutableLocalReassignment BlockLocalShadow SameScopeDuplicateLocal
                       MissingMain NonLiteralMain OutOfRangeMain ParameterizedMain WrongReturnMain UnknownCallMain
                       WrongCallArityMain BoolParameterCallMain NonLongReturnCallMain RecursiveCallMain
                       BoolCallAsLongMain MatchMissingElse MatchPatternTypeMismatch)
  configure_file(tests/fixtures/source/${source_fixture}.xs "${XS_SOURCE_NATIVE_FIXTURE_DIR}/${source_fixture}.xs"
                 COPYONLY)
endforeach()
set(XS_PROJECT_NATIVE_FIXTURE_DIR "${CMAKE_CURRENT_BINARY_DIR}/tests/fixtures/projects")
file(MAKE_DIRECTORY "${XS_PROJECT_NATIVE_FIXTURE_DIR}/source")
configure_file(tests/fixtures/projects/NativeMain.xsproj "${XS_PROJECT_NATIVE_FIXTURE_DIR}/NativeMain.xsproj"
               COPYONLY)
configure_file(tests/fixtures/projects/source/NativeMain.xs "${XS_PROJECT_NATIVE_FIXTURE_DIR}/source/NativeMain.xs"
               COPYONLY)

add_test(NAME source_native_return0_build COMMAND xs build -file ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainReturn0.xs)
set_tests_properties(source_native_return0_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_return0_artifacts COMMAND xs_xse_artifact_tests ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainReturn0.ll
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainReturn0.o
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainReturn0.xse 0)
set_tests_properties(source_native_return0_artifacts PROPERTIES DEPENDS source_native_return0_build TIMEOUT 5)
add_test(NAME source_native_return7_build COMMAND xs build -file ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainReturn7.xs)
set_tests_properties(source_native_return7_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_return7_artifacts COMMAND xs_xse_artifact_tests ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainReturn7.ll
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainReturn7.o
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainReturn7.xse 7)
set_tests_properties(source_native_return7_artifacts PROPERTIES DEPENDS source_native_return7_build TIMEOUT 5)
add_test(NAME source_native_arithmetic_build COMMAND xs build -file ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainArithmetic.xs)
set_tests_properties(source_native_arithmetic_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_arithmetic_artifacts COMMAND xs_xse_artifact_tests
                                                  ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainArithmetic.ll
                                                  ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainArithmetic.o
                                                  ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainArithmetic.xse 7 "ret i32 7")
set_tests_properties(source_native_arithmetic_artifacts PROPERTIES DEPENDS source_native_arithmetic_build TIMEOUT 5)
add_test(NAME source_native_division_build COMMAND xs build -file ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainDivision.xs)
set_tests_properties(source_native_division_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_division_artifacts COMMAND xs_xse_artifact_tests
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainDivision.ll
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainDivision.o
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainDivision.xse 7)
set_tests_properties(source_native_division_artifacts PROPERTIES DEPENDS source_native_division_build TIMEOUT 5)
add_test(NAME source_native_remainder_build COMMAND xs build -file ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainRemainder.xs)
set_tests_properties(source_native_remainder_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_remainder_artifacts COMMAND xs_xse_artifact_tests
                                                 ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainRemainder.ll
                                                 ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainRemainder.o
                                                 ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainRemainder.xse 7)
set_tests_properties(source_native_remainder_artifacts PROPERTIES DEPENDS source_native_remainder_build TIMEOUT 5)
add_test(NAME source_native_negative_build COMMAND xs build -file ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainNegative.xs)
set_tests_properties(source_native_negative_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_negative_artifacts COMMAND xs_xse_artifact_tests
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainNegative.ll
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainNegative.o
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainNegative.xse 7)
set_tests_properties(source_native_negative_artifacts PROPERTIES DEPENDS source_native_negative_build TIMEOUT 5)
add_test(NAME source_native_positive_build COMMAND xs build -file ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainPositive.xs)
set_tests_properties(source_native_positive_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_positive_artifacts COMMAND xs_xse_artifact_tests
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainPositive.ll
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainPositive.o
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainPositive.xse 7)
set_tests_properties(source_native_positive_artifacts PROPERTIES DEPENDS source_native_positive_build TIMEOUT 5)
add_test(NAME source_native_bitwise_build COMMAND xs build -file ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainBitwise.xs)
set_tests_properties(source_native_bitwise_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_bitwise_artifacts COMMAND xs_xse_artifact_tests
                                               ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainBitwise.ll
                                               ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainBitwise.o
                                               ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainBitwise.xse 6 "ret i32 6")
set_tests_properties(source_native_bitwise_artifacts PROPERTIES DEPENDS source_native_bitwise_build TIMEOUT 5)
add_test(NAME source_native_xor_build COMMAND xs build -file ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainXor.xs)
set_tests_properties(source_native_xor_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_xor_artifacts COMMAND xs_xse_artifact_tests ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainXor.ll
                                           ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainXor.o
                                           ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainXor.xse 5 "ret i32 5")
set_tests_properties(source_native_xor_artifacts PROPERTIES DEPENDS source_native_xor_build TIMEOUT 5)
add_test(NAME source_native_local_build COMMAND xs build -file ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainLocal.xs)
set_tests_properties(source_native_local_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_local_artifacts COMMAND xs_xse_artifact_tests
                                             ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainLocal.ll
                                             ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainLocal.o
                                             ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainLocal.xse 7 "load i32")
set_tests_properties(source_native_local_artifacts PROPERTIES DEPENDS source_native_local_build TIMEOUT 5)
add_test(NAME source_native_local_arithmetic_build COMMAND xs build -file
                                                     ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainLocalArithmetic.xs)
set_tests_properties(source_native_local_arithmetic_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_local_arithmetic_artifacts COMMAND xs_xse_artifact_tests
                                                        ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainLocalArithmetic.ll
                                                        ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainLocalArithmetic.o
                                                        ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainLocalArithmetic.xse 7
                                                        "add i32")
set_tests_properties(source_native_local_arithmetic_artifacts PROPERTIES DEPENDS source_native_local_arithmetic_build
                                                                         TIMEOUT 5)
add_test(NAME source_native_local_if_build COMMAND xs build -file ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainLocalIf.xs)
set_tests_properties(source_native_local_if_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_local_if_artifacts COMMAND xs_xse_artifact_tests
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainLocalIf.ll
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainLocalIf.o
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainLocalIf.xse 7)
set_tests_properties(source_native_local_if_artifacts PROPERTIES DEPENDS source_native_local_if_build TIMEOUT 5)
add_test(NAME source_native_inferred_local_build COMMAND xs build -file
                                                  ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainInferredLocal.xs)
set_tests_properties(source_native_inferred_local_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_inferred_local_artifacts COMMAND xs_xse_artifact_tests
                                                     ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainInferredLocal.ll
                                                     ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainInferredLocal.o
                                                     ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainInferredLocal.xse 7
                                                     "load i32")
set_tests_properties(source_native_inferred_local_artifacts PROPERTIES DEPENDS source_native_inferred_local_build
                                                                       TIMEOUT 5)
add_test(NAME source_native_mutable_local_build COMMAND xs build -file
                                                  ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainMutableLocal.xs)
set_tests_properties(source_native_mutable_local_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_mutable_local_artifacts COMMAND xs_xse_artifact_tests
                                                     ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainMutableLocal.ll
                                                     ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainMutableLocal.o
                                                     ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainMutableLocal.xse 7)
set_tests_properties(source_native_mutable_local_artifacts PROPERTIES DEPENDS source_native_mutable_local_build
                                                                       TIMEOUT 5)
add_test(NAME source_native_mutable_bool_local_build COMMAND xs build -file
                                                       ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainMutableBoolLocal.xs)
set_tests_properties(source_native_mutable_bool_local_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_mutable_bool_local_artifacts COMMAND xs_xse_artifact_tests
                                                          ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainMutableBoolLocal.ll
                                                          ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainMutableBoolLocal.o
                                                          ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainMutableBoolLocal.xse 7)
set_tests_properties(source_native_mutable_bool_local_artifacts PROPERTIES
                     DEPENDS source_native_mutable_bool_local_build TIMEOUT 5)
add_test(NAME source_native_if_assignment_build COMMAND xs build -file
                                                 ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIfAssignment.xs)
set_tests_properties(source_native_if_assignment_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_if_assignment_artifacts COMMAND xs_xse_artifact_tests
                                                    ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIfAssignment.ll
                                                    ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIfAssignment.o
                                                    ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIfAssignment.xse 7 "store i32 7")
set_tests_properties(source_native_if_assignment_artifacts PROPERTIES DEPENDS source_native_if_assignment_build TIMEOUT 5)
add_test(NAME source_native_compound_assignment_build COMMAND xs build -file
                                                         ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainCompoundAssignment.xs)
set_tests_properties(source_native_compound_assignment_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_compound_assignment_artifacts COMMAND xs_xse_artifact_tests
                                                            ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainCompoundAssignment.ll
                                                            ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainCompoundAssignment.o
                                                            ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainCompoundAssignment.xse 7 "xor i32")
set_tests_properties(source_native_compound_assignment_artifacts PROPERTIES
                     DEPENDS source_native_compound_assignment_build TIMEOUT 5)
add_test(NAME source_native_if_multiple_assignments_build COMMAND xs build -file
                                                            ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIfMultipleAssignments.xs)
set_tests_properties(source_native_if_multiple_assignments_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_if_multiple_assignments_artifacts COMMAND xs_xse_artifact_tests
                                                               ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIfMultipleAssignments.ll
                                                               ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIfMultipleAssignments.o
                                                               ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIfMultipleAssignments.xse 7
                                                               "br i1")
set_tests_properties(source_native_if_multiple_assignments_artifacts PROPERTIES
                     DEPENDS source_native_if_multiple_assignments_build TIMEOUT 5)
add_test(NAME source_native_nested_if_assignment_build COMMAND xs build -file
                                                        ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainNestedIfAssignment.xs)
set_tests_properties(source_native_nested_if_assignment_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_nested_if_assignment_artifacts COMMAND xs_xse_artifact_tests
                                                           ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainNestedIfAssignment.ll
                                                           ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainNestedIfAssignment.o
                                                           ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainNestedIfAssignment.xse 7 "br i1")
set_tests_properties(source_native_nested_if_assignment_artifacts PROPERTIES
                     DEPENDS source_native_nested_if_assignment_build TIMEOUT 5)
add_test(NAME source_native_while_build COMMAND xs build -file ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainWhile.xs)
set_tests_properties(source_native_while_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_while_artifacts COMMAND xs_xse_artifact_tests ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainWhile.ll
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainWhile.o
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainWhile.xse 7 "br i1")
set_tests_properties(source_native_while_artifacts PROPERTIES DEPENDS source_native_while_build TIMEOUT 5)
add_test(NAME source_native_while_control_build COMMAND xs build -file ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainWhileControl.xs)
set_tests_properties(source_native_while_control_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_while_control_artifacts COMMAND xs_xse_artifact_tests
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainWhileControl.ll
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainWhileControl.o
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainWhileControl.xse 7 "br label")
set_tests_properties(source_native_while_control_artifacts PROPERTIES DEPENDS source_native_while_control_build TIMEOUT 5)
add_test(NAME source_native_do_while_build COMMAND xs build -file ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainDoWhile.xs)
set_tests_properties(source_native_do_while_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_do_while_artifacts COMMAND xs_xse_artifact_tests
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainDoWhile.ll
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainDoWhile.o
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainDoWhile.xse 7 "store i32 7")
set_tests_properties(source_native_do_while_artifacts PROPERTIES DEPENDS source_native_do_while_build TIMEOUT 5)
add_test(NAME source_native_block_locals_build COMMAND xs build -file ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainBlockLocals.xs)
set_tests_properties(source_native_block_locals_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_block_locals_artifacts COMMAND xs_xse_artifact_tests
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainBlockLocals.ll
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainBlockLocals.o
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainBlockLocals.xse 10 "alloca i32")
set_tests_properties(source_native_block_locals_artifacts PROPERTIES DEPENDS source_native_block_locals_build TIMEOUT 5)
add_test(NAME source_native_block_local_shadow_build COMMAND xs build -file
                                                     ${XS_SOURCE_NATIVE_FIXTURE_DIR}/BlockLocalShadow.xs)
set_tests_properties(source_native_block_local_shadow_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_block_local_shadow_artifacts COMMAND xs_xse_artifact_tests
                                                               ${XS_SOURCE_NATIVE_FIXTURE_DIR}/BlockLocalShadow.ll
                                                               ${XS_SOURCE_NATIVE_FIXTURE_DIR}/BlockLocalShadow.o
                                                               ${XS_SOURCE_NATIVE_FIXTURE_DIR}/BlockLocalShadow.xse 0)
set_tests_properties(source_native_block_local_shadow_artifacts PROPERTIES
                     DEPENDS source_native_block_local_shadow_build TIMEOUT 5)
add_test(NAME source_native_same_scope_duplicate_local COMMAND xs build -file
                                                         ${XS_SOURCE_NATIVE_FIXTURE_DIR}/SameScopeDuplicateLocal.xs)
set_tests_properties(source_native_same_scope_duplicate_local PROPERTIES TIMEOUT 5 WILL_FAIL TRUE)
add_test(NAME source_native_early_return_build COMMAND xs build -file ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainEarlyReturn.xs)
set_tests_properties(source_native_early_return_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_early_return_artifacts COMMAND xs_xse_artifact_tests
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainEarlyReturn.ll
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainEarlyReturn.o
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainEarlyReturn.xse 7 "ret i32 7")
set_tests_properties(source_native_early_return_artifacts PROPERTIES DEPENDS source_native_early_return_build TIMEOUT 5)
add_test(NAME source_native_else_if_build COMMAND xs build -file ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainElseIf.xs)
set_tests_properties(source_native_else_if_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_else_if_artifacts COMMAND xs_xse_artifact_tests
                                            ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainElseIf.ll
                                            ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainElseIf.o
                                            ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainElseIf.xse 7 "br i1")
set_tests_properties(source_native_else_if_artifacts PROPERTIES DEPENDS source_native_else_if_build TIMEOUT 5)
add_test(NAME source_native_match_build COMMAND xs build -file ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainMatch.xs)
set_tests_properties(source_native_match_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_match_artifacts COMMAND xs_xse_artifact_tests ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainMatch.ll
                                             ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainMatch.o
                                             ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainMatch.xse 7 "icmp eq i32")
set_tests_properties(source_native_match_artifacts PROPERTIES DEPENDS source_native_match_build TIMEOUT 5)
add_test(NAME source_native_match_bool_build COMMAND xs build -file ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainMatchBool.xs)
set_tests_properties(source_native_match_bool_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_match_bool_artifacts COMMAND xs_xse_artifact_tests
                                                  ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainMatchBool.ll
                                                  ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainMatchBool.o
                                                  ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainMatchBool.xse 7 "br i1")
set_tests_properties(source_native_match_bool_artifacts PROPERTIES DEPENDS source_native_match_bool_build TIMEOUT 5)
add_test(NAME source_native_match_expression_build COMMAND xs build -file
                                                           ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainMatchExpression.xs)
set_tests_properties(source_native_match_expression_build PROPERTIES TIMEOUT 5
                     FIXTURES_SETUP source_native_match_expression)
add_test(NAME source_native_match_expression_artifacts COMMAND xs_xse_artifact_tests
  ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainMatchExpression.ll
  ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainMatchExpression.o
  ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainMatchExpression.xse 7 "icmp eq i32")
set_tests_properties(source_native_match_expression_artifacts PROPERTIES
                     DEPENDS source_native_match_expression_build TIMEOUT 5)
add_test(NAME source_native_for_build COMMAND xs build -file ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainFor.xs)
set_tests_properties(source_native_for_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_for_artifacts COMMAND xs_xse_artifact_tests ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainFor.ll
                                           ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainFor.o
                                           ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainFor.xse 8 "icmp slt i32")
set_tests_properties(source_native_for_artifacts PROPERTIES DEPENDS source_native_for_build TIMEOUT 5)
add_test(NAME source_native_postfix_decrement_build COMMAND xs build -file
                                                        ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainPostfixDecrement.xs)
set_tests_properties(source_native_postfix_decrement_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_postfix_decrement_artifacts COMMAND xs_xse_artifact_tests
                                                            ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainPostfixDecrement.ll
                                                            ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainPostfixDecrement.o
                                                            ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainPostfixDecrement.xse 7)
set_tests_properties(source_native_postfix_decrement_artifacts PROPERTIES
                     DEPENDS source_native_postfix_decrement_build TIMEOUT 5)
add_test(NAME source_native_update_values_build COMMAND xs build -file
                                                   ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainUpdateValues.xs)
set_tests_properties(source_native_update_values_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_update_values_artifacts COMMAND xs_xse_artifact_tests
                                                      ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainUpdateValues.ll
                                                      ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainUpdateValues.o
                                                      ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainUpdateValues.xse 24
                                                      "store i32")
set_tests_properties(source_native_update_values_artifacts PROPERTIES DEPENDS source_native_update_values_build TIMEOUT 5)
add_test(NAME source_native_if_build COMMAND xs build -file ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIf.xs)
set_tests_properties(source_native_if_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_if_artifacts COMMAND xs_xse_artifact_tests ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIf.ll
                                          ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIf.o
                                          ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIf.xse 7 "ret i32 7" "!br label")
set_tests_properties(source_native_if_artifacts PROPERTIES DEPENDS source_native_if_build TIMEOUT 5)
add_test(NAME source_native_if_not_build COMMAND xs build -file ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIfNot.xs)
set_tests_properties(source_native_if_not_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_if_not_artifacts COMMAND xs_xse_artifact_tests ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIfNot.ll
                                              ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIfNot.o
                                              ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIfNot.xse 7 "ret i32 7" "!br label")
set_tests_properties(source_native_if_not_artifacts PROPERTIES DEPENDS source_native_if_not_build TIMEOUT 5)
add_test(NAME source_native_if_false_build COMMAND xs build -file ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIfFalse.xs)
set_tests_properties(source_native_if_false_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_if_false_artifacts COMMAND xs_xse_artifact_tests
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIfFalse.ll
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIfFalse.o
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIfFalse.xse 7 "ret i32 7")
set_tests_properties(source_native_if_false_artifacts PROPERTIES DEPENDS source_native_if_false_build TIMEOUT 5)
add_test(NAME source_native_if_not_equal_build COMMAND xs build -file
                                                    ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIfNotEqual.xs)
set_tests_properties(source_native_if_not_equal_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_if_not_equal_artifacts COMMAND xs_xse_artifact_tests
                                                    ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIfNotEqual.ll
                                                    ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIfNotEqual.o
                                                    ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIfNotEqual.xse 7 "ret i32 7"
                                                    "!br label")
set_tests_properties(source_native_if_not_equal_artifacts PROPERTIES DEPENDS source_native_if_not_equal_build TIMEOUT 5)
add_test(NAME source_native_bool_local_build COMMAND xs build -file ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainBoolLocal.xs)
set_tests_properties(source_native_bool_local_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_bool_local_artifacts COMMAND xs_xse_artifact_tests
                                                 ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainBoolLocal.ll
                                                 ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainBoolLocal.o
                                                 ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainBoolLocal.xse 7)
set_tests_properties(source_native_bool_local_artifacts PROPERTIES DEPENDS source_native_bool_local_build TIMEOUT 5)
add_test(NAME source_native_bool_not_local_build COMMAND xs build -file
                                                   ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainBoolNotLocal.xs)
set_tests_properties(source_native_bool_not_local_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_bool_not_local_artifacts COMMAND xs_xse_artifact_tests
                                                     ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainBoolNotLocal.ll
                                                     ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainBoolNotLocal.o
                                                     ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainBoolNotLocal.xse 7)
set_tests_properties(source_native_bool_not_local_artifacts PROPERTIES DEPENDS source_native_bool_not_local_build
                                                                       TIMEOUT 5)
add_test(NAME source_native_inferred_bool_local_build COMMAND xs build -file
                                                       ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainInferredBoolLocal.xs)
set_tests_properties(source_native_inferred_bool_local_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_inferred_bool_local_artifacts COMMAND xs_xse_artifact_tests
                                                          ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainInferredBoolLocal.ll
                                                          ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainInferredBoolLocal.o
                                                          ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainInferredBoolLocal.xse 7)
set_tests_properties(source_native_inferred_bool_local_artifacts PROPERTIES
                     DEPENDS source_native_inferred_bool_local_build TIMEOUT 5)
add_test(NAME source_native_inferred_bool_not_local_build COMMAND xs build -file
                                                            ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainInferredBoolNotLocal.xs)
set_tests_properties(source_native_inferred_bool_not_local_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_inferred_bool_not_local_artifacts COMMAND xs_xse_artifact_tests
                                                              ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainInferredBoolNotLocal.ll
                                                              ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainInferredBoolNotLocal.o
                                                              ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainInferredBoolNotLocal.xse
                                                              7)
set_tests_properties(source_native_inferred_bool_not_local_artifacts PROPERTIES
                     DEPENDS source_native_inferred_bool_not_local_build TIMEOUT 5)
add_test(NAME source_native_call_build COMMAND xs build -file ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainCall.xs)
set_tests_properties(source_native_call_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_call_artifacts COMMAND xs_xse_artifact_tests ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainCall.ll
                                            ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainCall.o
                                            ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainCall.xse 7 "call i32 @Add")
set_tests_properties(source_native_call_artifacts PROPERTIES DEPENDS source_native_call_build TIMEOUT 5)
add_test(NAME source_native_nested_call_build COMMAND xs build -file
                                                 ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainNestedCall.xs)
set_tests_properties(source_native_nested_call_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_nested_call_artifacts COMMAND xs_xse_artifact_tests
                                                   ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainNestedCall.ll
                                                   ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainNestedCall.o
                                                   ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainNestedCall.xse 7
                                                   "call i32 @Add")
set_tests_properties(source_native_nested_call_artifacts PROPERTIES DEPENDS source_native_nested_call_build TIMEOUT 5)
add_test(NAME source_native_local_call_build COMMAND xs build -file ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainLocalCall.xs)
set_tests_properties(source_native_local_call_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_local_call_artifacts COMMAND xs_xse_artifact_tests
                                                  ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainLocalCall.ll
                                                  ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainLocalCall.o
                                                  ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainLocalCall.xse 7
                                                  "call i32 @Add")
set_tests_properties(source_native_local_call_artifacts PROPERTIES DEPENDS source_native_local_call_build TIMEOUT 5)
add_test(NAME source_native_bool_call_build COMMAND xs build -file ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainBoolCall.xs)
set_tests_properties(source_native_bool_call_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_bool_call_artifacts COMMAND xs_xse_artifact_tests
                                                 ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainBoolCall.ll
                                                 ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainBoolCall.o
                                                 ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainBoolCall.xse 7
                                                 "call i1 @IsPositive")
set_tests_properties(source_native_bool_call_artifacts PROPERTIES DEPENDS source_native_bool_call_build TIMEOUT 5)
add_test(NAME source_native_bool_call_local_build COMMAND xs build -file
                                                    ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainBoolCallLocal.xs)
set_tests_properties(source_native_bool_call_local_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_bool_call_local_artifacts COMMAND xs_xse_artifact_tests
                                                       ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainBoolCallLocal.ll
                                                       ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainBoolCallLocal.o
                                                       ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainBoolCallLocal.xse 7
                                                       "call i1 @IsZero")
set_tests_properties(source_native_bool_call_local_artifacts PROPERTIES DEPENDS source_native_bool_call_local_build
                                                                        TIMEOUT 5)
add_test(NAME source_native_bool_parameter_call_build COMMAND xs build -file
                                                        ${XS_SOURCE_NATIVE_FIXTURE_DIR}/BoolParameterCallMain.xs)
set_tests_properties(source_native_bool_parameter_call_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_bool_parameter_call_artifacts COMMAND xs_xse_artifact_tests
                                                            ${XS_SOURCE_NATIVE_FIXTURE_DIR}/BoolParameterCallMain.ll
                                                            ${XS_SOURCE_NATIVE_FIXTURE_DIR}/BoolParameterCallMain.o
                                                            ${XS_SOURCE_NATIVE_FIXTURE_DIR}/BoolParameterCallMain.xse 1
                                                            "call i32 @Choose")
set_tests_properties(source_native_bool_parameter_call_artifacts PROPERTIES
                     DEPENDS source_native_bool_parameter_call_build TIMEOUT 5)
add_test(NAME project_native_build COMMAND xs build -proj ${XS_PROJECT_NATIVE_FIXTURE_DIR}/NativeMain.xsproj)
set_tests_properties(project_native_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME project_native_artifacts COMMAND xs_xse_artifact_tests ${XS_PROJECT_NATIVE_FIXTURE_DIR}/source/NativeMain.ll
                                           ${XS_PROJECT_NATIVE_FIXTURE_DIR}/source/NativeMain.o
                                           ${XS_PROJECT_NATIVE_FIXTURE_DIR}/source/NativeMain.xse 7)
set_tests_properties(project_native_artifacts PROPERTIES DEPENDS project_native_build TIMEOUT 5)
foreach(source_fixture MissingMain NonLiteralMain OutOfRangeMain ParameterizedMain WrongReturnMain UnknownCallMain
                       WrongCallArityMain NonLongReturnCallMain RecursiveCallMain
                       BoolCallAsLongMain ImmutableLocalReassignment MatchMissingElse MatchPatternTypeMismatch)
  add_test(NAME source_native_invalid_${source_fixture} COMMAND xs build -file
                                                           ${XS_SOURCE_NATIVE_FIXTURE_DIR}/${source_fixture}.xs)
  set_tests_properties(source_native_invalid_${source_fixture} PROPERTIES TIMEOUT 5 WILL_FAIL TRUE)
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
xs_add_c_test(inheritance tests/inheritance_tests.c xs_compiler)
xs_add_c_test(xlil tests/xlil_tests.c xs_compiler)
xs_add_c_test(mir tests/mir_tests.c xs_compiler)
xs_add_c_test(mir_flow tests/mir_flow_tests.c xs_compiler)
xs_add_c_test(syntax_ast tests/syntax_ast_tests.c xs_compiler)
xs_add_c_test(syntax_decl tests/syntax_decl_tests.c xs_compiler)
xs_add_c_test(syntax_macro tests/syntax_macro_tests.c xs_compiler)
xs_add_c_test(source_include tests/source_include_tests.c xs_compiler)
xs_add_c_test(syntax_macro_view tests/syntax_macro_view_tests.c xs_compiler)
add_executable(xs_compiler_core_bridge_tests tests/compiler_core_bridge_tests.c)
target_link_libraries(xs_compiler_core_bridge_tests PRIVATE xs_compiler xslang_compiler_core)
add_test(NAME compiler_core_bridge COMMAND xs_compiler_core_bridge_tests)
set_tests_properties(compiler_core_bridge PROPERTIES TIMEOUT 5)
