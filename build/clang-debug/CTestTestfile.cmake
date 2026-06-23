# CMake generated Testfile for 
# Source directory: /home/hasan/Projeler/XS
# Build directory: /home/hasan/Projeler/XS/build/clang-debug
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[lexer]=] "/home/hasan/Projeler/XS/build/clang-debug/xs_lexer_tests")
set_tests_properties([=[lexer]=] PROPERTIES  TIMEOUT "5" _BACKTRACE_TRIPLES "/home/hasan/Projeler/XS/CMakeLists.txt;86;add_test;/home/hasan/Projeler/XS/CMakeLists.txt;0;")
add_test([=[parser]=] "/home/hasan/Projeler/XS/build/clang-debug/xs_parser_tests")
set_tests_properties([=[parser]=] PROPERTIES  TIMEOUT "5" _BACKTRACE_TRIPLES "/home/hasan/Projeler/XS/CMakeLists.txt;91;add_test;/home/hasan/Projeler/XS/CMakeLists.txt;0;")
add_test([=[project]=] "/home/hasan/Projeler/XS/build/clang-debug/xs_project_tests")
set_tests_properties([=[project]=] PROPERTIES  TIMEOUT "5" _BACKTRACE_TRIPLES "/home/hasan/Projeler/XS/CMakeLists.txt;96;add_test;/home/hasan/Projeler/XS/CMakeLists.txt;0;")
add_test([=[example_project]=] "/home/hasan/Projeler/XS/build/clang-debug/xs" "check" "-proj" "/home/hasan/Projeler/XS/XS/example/MyApp.xsproj")
set_tests_properties([=[example_project]=] PROPERTIES  TIMEOUT "5" _BACKTRACE_TRIPLES "/home/hasan/Projeler/XS/CMakeLists.txt;99;add_test;/home/hasan/Projeler/XS/CMakeLists.txt;0;")
add_test([=[backend]=] "/home/hasan/Projeler/XS/build/clang-debug/xs_backend_tests" "/home/hasan/Projeler/XS/build/clang-debug/xs_backend_test.o" "/usr/bin/ld.lld")
set_tests_properties([=[backend]=] PROPERTIES  TIMEOUT "10" _BACKTRACE_TRIPLES "/home/hasan/Projeler/XS/CMakeLists.txt;104;add_test;/home/hasan/Projeler/XS/CMakeLists.txt;0;")
add_test([=[modules]=] "/home/hasan/Projeler/XS/build/clang-debug/xs_module_tests" "/home/hasan/Projeler/XS/tests/fixtures/module_project" "/home/hasan/Projeler/XS/tests/fixtures/duplicate_modules")
set_tests_properties([=[modules]=] PROPERTIES  TIMEOUT "5" _BACKTRACE_TRIPLES "/home/hasan/Projeler/XS/CMakeLists.txt;109;add_test;/home/hasan/Projeler/XS/CMakeLists.txt;0;")
add_test([=[syntax_ast]=] "/home/hasan/Projeler/XS/build/clang-debug/xs_syntax_ast_tests")
set_tests_properties([=[syntax_ast]=] PROPERTIES  TIMEOUT "5" _BACKTRACE_TRIPLES "/home/hasan/Projeler/XS/CMakeLists.txt;115;add_test;/home/hasan/Projeler/XS/CMakeLists.txt;0;")
