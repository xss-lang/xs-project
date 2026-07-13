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
                       MissingMain NonLiteralMain OutOfRangeMain ParameterizedMain WrongReturnMain UnknownCallMain
                       WrongCallArityMain NonLongParameterCallMain NonLongReturnCallMain RecursiveCallMain
                       BoolCallAsLongMain)
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
                                             ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainLocal.xse 7 "ret i32 7")
set_tests_properties(source_native_local_artifacts PROPERTIES DEPENDS source_native_local_build TIMEOUT 5)
add_test(NAME source_native_local_arithmetic_build COMMAND xs build -file
                                                     ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainLocalArithmetic.xs)
set_tests_properties(source_native_local_arithmetic_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_local_arithmetic_artifacts COMMAND xs_xse_artifact_tests
                                                        ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainLocalArithmetic.ll
                                                        ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainLocalArithmetic.o
                                                        ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainLocalArithmetic.xse 7
                                                        "ret i32 7")
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
                                                     "ret i32 7")
set_tests_properties(source_native_inferred_local_artifacts PROPERTIES DEPENDS source_native_inferred_local_build
                                                                       TIMEOUT 5)
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
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIfFalse.xse 7 "ret i32 7"
                                                "!br label")
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
add_test(NAME project_native_build COMMAND xs build -proj ${XS_PROJECT_NATIVE_FIXTURE_DIR}/NativeMain.xsproj)
set_tests_properties(project_native_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME project_native_artifacts COMMAND xs_xse_artifact_tests ${XS_PROJECT_NATIVE_FIXTURE_DIR}/source/NativeMain.ll
                                           ${XS_PROJECT_NATIVE_FIXTURE_DIR}/source/NativeMain.o
                                           ${XS_PROJECT_NATIVE_FIXTURE_DIR}/source/NativeMain.xse 7)
set_tests_properties(project_native_artifacts PROPERTIES DEPENDS project_native_build TIMEOUT 5)
foreach(source_fixture MissingMain NonLiteralMain OutOfRangeMain ParameterizedMain WrongReturnMain UnknownCallMain
                       WrongCallArityMain NonLongParameterCallMain NonLongReturnCallMain RecursiveCallMain
                       BoolCallAsLongMain)
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
xs_add_c_test(xlil tests/xlil_tests.c xs_compiler)
xs_add_c_test(mir tests/mir_tests.c xs_compiler)
xs_add_c_test(syntax_ast tests/syntax_ast_tests.c xs_compiler)
xs_add_c_test(syntax_decl tests/syntax_decl_tests.c xs_compiler)
xs_add_c_test(syntax_macro tests/syntax_macro_tests.c xs_compiler)
xs_add_c_test(source_include tests/source_include_tests.c xs_compiler)
xs_add_c_test(syntax_macro_view tests/syntax_macro_view_tests.c xs_compiler)
