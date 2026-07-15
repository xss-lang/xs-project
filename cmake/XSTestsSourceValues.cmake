# SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
# SPDX-License-Identifier: Apache-2.0

set(XS_SOURCE_NATIVE_FIXTURE_DIR "${CMAKE_CURRENT_BINARY_DIR}/tests/fixtures/source")
file(MAKE_DIRECTORY "${XS_SOURCE_NATIVE_FIXTURE_DIR}")
foreach(source_fixture MainReturn0 MainReturn7 MainArithmetic MainDivision MainRemainder MainOperatorCall MainIntOperators MainFloatConstants MainFloatOperators MainStringLiteral MainStringFlow MainStringCompare MainCharFlow MainIntegerWidths MainIntegerOperators MainDataFields MainNegative MainPositive
                       MainBitwise MainXor MainLocal MainLocalArithmetic MainLocalIf MainInferredLocal MainIf MainIfValue
                       MainIfNot
                       MainIfFalse MainIfNotEqual MainBoolLocal MainBoolNotLocal MainInferredBoolLocal
                       MainInferredBoolNotLocal MainCall MainNestedCall MainLocalCall MainBoolCall MainBoolCallLocal
                       MainRecursiveCall MainUnitCalls MainShortCircuit
                       MainMutableLocal MainMutableBoolLocal MainIfAssignment MainCompoundAssignment
                       MainIfMultipleAssignments MainNestedIfAssignment MainWhile MainWhileControl MainDoWhile MainLoop
                       MainBlockLocals
                       MainEarlyReturn MainElseIf MainMatch MainMatchBool MainMatchExpression MainFor
                       MainPostfixDecrement MainUpdateValues
                       ImmutableLocalReassignment BlockLocalShadow SameScopeDuplicateLocal
                       MissingMain NonLiteralMain OutOfRangeMain OutOfRangeByteMain OutOfRangeUIntegerMain
                       ParameterizedMain WrongReturnMain UnknownCallMain
                       WrongCallArityMain BoolParameterCallMain NonLongReturnCallMain
                       BoolCallAsLongMain UnitCallAsLongMain InvalidLogicalOperands MatchMissingElse
                       MatchPatternTypeMismatch)
  configure_file(tests/fixtures/source/${source_fixture}.xs "${XS_SOURCE_NATIVE_FIXTURE_DIR}/${source_fixture}.xs"
                 COPYONLY)
endforeach()
set(XS_PROJECT_NATIVE_FIXTURE_DIR "${CMAKE_CURRENT_BINARY_DIR}/tests/fixtures/projects")
file(COPY tests/fixtures/projects/ DESTINATION "${XS_PROJECT_NATIVE_FIXTURE_DIR}")

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
add_test(NAME source_native_operator_call_build COMMAND xs build -file
                                                       ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainOperatorCall.xs)
set_tests_properties(source_native_operator_call_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_operator_call_artifacts COMMAND xs_xse_artifact_tests
                                                    ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainOperatorCall.ll
                                                    ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainOperatorCall.o
                                                    ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainOperatorCall.xse 7)
set_tests_properties(source_native_operator_call_artifacts PROPERTIES DEPENDS source_native_operator_call_build TIMEOUT 5)
add_test(NAME source_native_int_operators_build COMMAND xs build -file
                                                   ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIntOperators.xs)
set_tests_properties(source_native_int_operators_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_int_operators_artifacts COMMAND xs_xse_artifact_tests
                                                       ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIntOperators.ll
                                                       ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIntOperators.o
                                                       ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIntOperators.xse 7
                                                       "define i64 @wide_ops" "sdiv i64" "srem i64" "ashr i64"
                                                       "icmp sge i64")
set_tests_properties(source_native_int_operators_artifacts PROPERTIES DEPENDS source_native_int_operators_build TIMEOUT 5)
add_test(NAME source_native_float_constants_build COMMAND xs build -file
                                                   ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainFloatConstants.xs)
set_tests_properties(source_native_float_constants_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_float_constants_artifacts COMMAND xs_xse_artifact_tests
                                                         ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainFloatConstants.ll
                                                         ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainFloatConstants.o
                                                         ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainFloatConstants.xse 0
                                                         "define float @single_value" "ret float 1.500000e+00"
                                                         "define double @double_value" "ret double 1.500000e+00")
set_tests_properties(source_native_float_constants_artifacts PROPERTIES DEPENDS source_native_float_constants_build
                                                                           TIMEOUT 5)
add_test(NAME source_native_float_operators_build COMMAND xs build -file
                                                   ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainFloatOperators.xs)
set_tests_properties(source_native_float_operators_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_float_operators_artifacts COMMAND xs_xse_artifact_tests
                                                         ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainFloatOperators.ll
                                                         ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainFloatOperators.o
  ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainFloatOperators.xse 0
  "fadd float" "frem float" "fdiv double" "fcmp olt double" "fcmp one double")
set_tests_properties(source_native_float_operators_artifacts PROPERTIES DEPENDS source_native_float_operators_build
                                                                           TIMEOUT 5)
add_test(NAME source_native_string_literal_build COMMAND xs build -file
                                                   ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainStringLiteral.xs)
set_tests_properties(source_native_string_literal_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_string_literal_artifacts COMMAND xs_xse_artifact_tests
                                                         ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainStringLiteral.ll
                                                         ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainStringLiteral.o
                                                         ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainStringLiteral.xse 0
                                                         "define { ptr, i64 } @greeting" "constant [6 x i8]")
set_tests_properties(source_native_string_literal_artifacts PROPERTIES DEPENDS source_native_string_literal_build
                                                                          TIMEOUT 5)
add_test(NAME source_native_string_flow_build COMMAND xs build -file
                                                ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainStringFlow.xs)
set_tests_properties(source_native_string_flow_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_string_flow_artifacts COMMAND xs_xse_artifact_tests
                                                      ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainStringFlow.ll
                                                      ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainStringFlow.o
                                                      ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainStringFlow.xse 0
                                                      "define { ptr, i64 } @identity" "call { ptr, i64 } @identity")
set_tests_properties(source_native_string_flow_artifacts PROPERTIES DEPENDS source_native_string_flow_build TIMEOUT 5)
add_test(NAME source_native_string_compare_build COMMAND xs build -file
                                               ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainStringCompare.xs)
set_tests_properties(source_native_string_compare_build PROPERTIES TIMEOUT 5
                     PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_string_compare_artifacts COMMAND xs_xse_artifact_tests
                                                    ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainStringCompare.ll
                                                    ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainStringCompare.o
                                                    ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainStringCompare.xse 0
                                                    "call i32 @memcmp")
set_tests_properties(source_native_string_compare_artifacts PROPERTIES DEPENDS source_native_string_compare_build TIMEOUT 5)
add_test(NAME source_native_char_flow_build COMMAND xs build -file
                                              ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainCharFlow.xs)
set_tests_properties(source_native_char_flow_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_char_flow_artifacts COMMAND xs_xse_artifact_tests
                                                    ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainCharFlow.ll
                                                    ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainCharFlow.o
                                                    ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainCharFlow.xse 0
                                                    "define i16 @identity" "store i16 937" "call i16 @identity")
set_tests_properties(source_native_char_flow_artifacts PROPERTIES DEPENDS source_native_char_flow_build TIMEOUT 5)
add_test(NAME source_native_integer_widths_build COMMAND xs build -file
                                                  ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIntegerWidths.xs)
set_tests_properties(source_native_integer_widths_build PROPERTIES TIMEOUT 5
                    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_integer_widths_artifacts COMMAND xs_xse_artifact_tests
                                                       ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIntegerWidths.ll
                                                       ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIntegerWidths.o
                                                       ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIntegerWidths.xse 0
                                                       "define i8 @byte_max" "define i16 @short_min"
                                                       "define i128 @integer_min" "ret i128 -1")
set_tests_properties(source_native_integer_widths_artifacts PROPERTIES DEPENDS source_native_integer_widths_build
                                                                       TIMEOUT 5)
add_test(NAME source_native_integer_operators_build COMMAND xs build -file
                                                   ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIntegerOperators.xs)
set_tests_properties(source_native_integer_operators_build PROPERTIES TIMEOUT 5
                     PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_integer_operators_artifacts COMMAND xs_xse_artifact_tests
                                                        ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIntegerOperators.ll
                                                        ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIntegerOperators.o
                                                        ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainIntegerOperators.xse 0
                                                        "add i8" "sdiv i8" "udiv i64" "icmp ult i128"
                                                        "lshr i128" "icmp slt i128" "ashr i128")
set_tests_properties(source_native_integer_operators_artifacts PROPERTIES
                     DEPENDS source_native_integer_operators_build TIMEOUT 5)

add_test(NAME source_native_data_fields_build COMMAND xs build -file
                                                  ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainDataFields.xs)
set_tests_properties(source_native_data_fields_build PROPERTIES TIMEOUT 5
                     PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME source_native_data_fields_artifacts COMMAND xs_xse_artifact_tests
                                                       ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainDataFields.ll
                                                       ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainDataFields.o
                                                       ${XS_SOURCE_NATIVE_FIXTURE_DIR}/MainDataFields.xse 9
                                                       "alloca i32" "store i32 2" "store i32 3" "add i32")
set_tests_properties(source_native_data_fields_artifacts PROPERTIES DEPENDS source_native_data_fields_build TIMEOUT 5)
