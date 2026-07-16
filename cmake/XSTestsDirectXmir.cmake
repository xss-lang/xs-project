# SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
# SPDX-License-Identifier: Apache-2.0

set(XS_DIRECT_XMIR_FIXTURE_DIR "${CMAKE_CURRENT_BINARY_DIR}/tests/fixtures/direct_xmir")
file(MAKE_DIRECTORY "${XS_DIRECT_XMIR_FIXTURE_DIR}")
foreach(fixture Supported InvalidLocal)
  configure_file(tests/fixtures/intermediate/${fixture}.xmir
                 "${XS_DIRECT_XMIR_FIXTURE_DIR}/${fixture}.xmir" COPYONLY)
endforeach()
configure_file(tests/fixtures/source/MainCall.xs "${XS_DIRECT_XMIR_FIXTURE_DIR}/MainCall.xs" COPYONLY)
configure_file(tests/fixtures/source/MainTupleCalls.xs "${XS_DIRECT_XMIR_FIXTURE_DIR}/MainTupleCalls.xs" COPYONLY)
configure_file(tests/fixtures/source/MainFixedArray.xs "${XS_DIRECT_XMIR_FIXTURE_DIR}/MainFixedArray.xs" COPYONLY)

add_test(NAME direct_xmir_native_build COMMAND xs build --mir -file
  ${XS_DIRECT_XMIR_FIXTURE_DIR}/Supported.xmir)
set_tests_properties(direct_xmir_native_build PROPERTIES TIMEOUT 5
  PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME direct_xmir_native_artifacts COMMAND xs_xse_artifact_tests
  ${XS_DIRECT_XMIR_FIXTURE_DIR}/Supported.ll
  ${XS_DIRECT_XMIR_FIXTURE_DIR}/Supported.o
  ${XS_DIRECT_XMIR_FIXTURE_DIR}/Supported.xse 7 "ret i32 7")
set_tests_properties(direct_xmir_native_artifacts PROPERTIES TIMEOUT 5 DEPENDS direct_xmir_native_build)

add_test(NAME direct_xmir_rejects_invalid_local COMMAND xs build --mir -file
  ${XS_DIRECT_XMIR_FIXTURE_DIR}/InvalidLocal.xmir)
set_tests_properties(direct_xmir_rejects_invalid_local PROPERTIES TIMEOUT 5 WILL_FAIL TRUE)

add_test(NAME direct_xmir_source_output COMMAND xs build --mir -file
  ${XS_DIRECT_XMIR_FIXTURE_DIR}/MainCall.xs)
set_tests_properties(direct_xmir_source_output PROPERTIES TIMEOUT 5 PASS_REGULAR_EXPRESSION "wrote XMIR")
add_test(NAME direct_xmir_source_roundtrip COMMAND xs build --mir -file
  ${XS_DIRECT_XMIR_FIXTURE_DIR}/MainCall.xmir)
set_tests_properties(direct_xmir_source_roundtrip PROPERTIES TIMEOUT 5 DEPENDS direct_xmir_source_output
  PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
add_test(NAME direct_xmir_source_artifacts COMMAND xs_xse_artifact_tests
  ${XS_DIRECT_XMIR_FIXTURE_DIR}/MainCall.ll
  ${XS_DIRECT_XMIR_FIXTURE_DIR}/MainCall.o
  ${XS_DIRECT_XMIR_FIXTURE_DIR}/MainCall.xse 7 "call i32 @Add")
set_tests_properties(direct_xmir_source_artifacts PROPERTIES TIMEOUT 5 DEPENDS direct_xmir_source_roundtrip)

foreach(fixture MainTupleCalls MainFixedArray)
  add_test(NAME direct_xmir_${fixture}_output COMMAND xs build --mir -file
    ${XS_DIRECT_XMIR_FIXTURE_DIR}/${fixture}.xs)
  set_tests_properties(direct_xmir_${fixture}_output PROPERTIES TIMEOUT 5 PASS_REGULAR_EXPRESSION "wrote XMIR")
  add_test(NAME direct_xmir_${fixture}_roundtrip COMMAND xs build --mir -file
    ${XS_DIRECT_XMIR_FIXTURE_DIR}/${fixture}.xmir)
  set_tests_properties(direct_xmir_${fixture}_roundtrip PROPERTIES TIMEOUT 5 DEPENDS direct_xmir_${fixture}_output
    PASS_REGULAR_EXPRESSION "wrote optimized LLVM IR.*executable")
  add_test(NAME direct_xmir_${fixture}_artifacts COMMAND xs_xse_artifact_tests
    ${XS_DIRECT_XMIR_FIXTURE_DIR}/${fixture}.ll
    ${XS_DIRECT_XMIR_FIXTURE_DIR}/${fixture}.o
    ${XS_DIRECT_XMIR_FIXTURE_DIR}/${fixture}.xse 7)
  set_tests_properties(direct_xmir_${fixture}_artifacts PROPERTIES TIMEOUT 5 DEPENDS direct_xmir_${fixture}_roundtrip)
endforeach()
