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

if(NOT XS_BUILD_PROJECT_XS)
  return()
endif()

add_test(NAME cli_version COMMAND xs --version)
string(REPLACE "." "\\." XS_PROJECT_VERSION_REGEX "${PROJECT_VERSION}")
set_tests_properties(cli_version PROPERTIES TIMEOUT 5 PASS_REGULAR_EXPRESSION "xs ${XS_PROJECT_VERSION_REGEX}")

xs_add_c_test(lexer tests/lexer_tests.c xs_compiler)
xs_add_c_test(parser tests/parser_tests.c xs_compiler)
xs_add_c_test(diagnostic tests/diagnostic_tests.c xs_compiler)

set(XS_GRADLE_EXECUTABLE "${CMAKE_SOURCE_DIR}/xs_kts/gradlew")
set(XS_PROJECT_TEST_DRIVER
    "${CMAKE_SOURCE_DIR}/xs_kts/build/install/xs-project/bin/xs-project")
add_test(NAME kotlin_project_resolver_build COMMAND "${XS_GRADLE_EXECUTABLE}" --daemon --build-cache
  -p "${CMAKE_SOURCE_DIR}/xs_kts" installDist)
set_tests_properties(kotlin_project_resolver_build PROPERTIES TIMEOUT 180
  FIXTURES_SETUP kotlin_project_resolver ENVIRONMENT "GRADLE_OPTS=-Xmx512m")

add_test(NAME example_source COMMAND xs check -file
  ${XS_SOURCE_FROM_BINARY}/tests/fixtures/example_project/source/Main.xs)
set_tests_properties(example_source PROPERTIES TIMEOUT 5)
add_test(NAME macro_source COMMAND xs check -file
  ${XS_SOURCE_FROM_BINARY}/tests/fixtures/macro_project/source/Main.xs)
set_tests_properties(macro_source PROPERTIES TIMEOUT 5)
add_test(NAME compiler_check_file COMMAND xs check -file
  ${XS_SOURCE_FROM_BINARY}/tests/fixtures/example_project/source/Main.xs)
set_tests_properties(compiler_check_file PROPERTIES TIMEOUT 5)
add_test(NAME compiler_check_file_verbose COMMAND xs check -file
  ${XS_SOURCE_FROM_BINARY}/tests/fixtures/example_project/source/Main.xs
  --warning all --werror true --verbose true --xgc-enabled true)
set_tests_properties(compiler_check_file_verbose PROPERTIES TIMEOUT 5
  PASS_REGULAR_EXPRESSION "verbose: command=check.*warning=all.*werror=true.*xgc=true")
add_test(NAME compiler_rejects_invalid_xgc COMMAND xs check -file
  ${XS_SOURCE_FROM_BINARY}/tests/fixtures/example_project/source/Main.xs --xgc-enabled maybe)
set_tests_properties(compiler_rejects_invalid_xgc PROPERTIES TIMEOUT 5 WILL_FAIL TRUE)
add_test(NAME compiler_rejects_invalid_warning COMMAND xs check -file
  ${XS_SOURCE_FROM_BINARY}/tests/fixtures/example_project/source/Main.xs --warning invalid)
set_tests_properties(compiler_rejects_invalid_warning PROPERTIES TIMEOUT 5 WILL_FAIL TRUE)
add_test(NAME compiler_rejects_misspelled_werror COMMAND xs check -file
  ${XS_SOURCE_FROM_BINARY}/tests/fixtures/example_project/source/Main.xs --werrror true)
set_tests_properties(compiler_rejects_misspelled_werror PROPERTIES TIMEOUT 5 WILL_FAIL TRUE)

foreach(output hir mir xlil)
  add_test(NAME build_file_output_${output} COMMAND xs build --output ${output} -file ${XS_SOURCE_FROM_BINARY}/tests/fixtures/example_project/source/Main.xs)
  set_tests_properties(build_file_output_${output} PROPERTIES TIMEOUT 5 WILL_FAIL TRUE)
  add_test(NAME build_file_short_${output} COMMAND xs build --${output} -file ${XS_SOURCE_FROM_BINARY}/tests/fixtures/example_project/source/Main.xs)
  set_tests_properties(build_file_short_${output} PROPERTIES TIMEOUT 5 WILL_FAIL TRUE)
endforeach()

foreach(format xhir xmir xlil)
  add_test(NAME direct_${format}_unsupported_version COMMAND xs build --${format} -file ${XS_SOURCE_FROM_BINARY}/tests/fixtures/intermediate/Unsupported.${format})
  set_tests_properties(direct_${format}_unsupported_version PROPERTIES TIMEOUT 5 WILL_FAIL TRUE)
endforeach()

include(XSTestsDirectXlil)
include(XSTestsSourceValues)
include(XSTestsSourceControl)
include(XSTestsSourceCalls)
include(XSTestsKotlin)
include(XSTestsLibraries)
