# SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
# SPDX-License-Identifier: MPL-2.0

add_library(xsproj
  "${PROJECT_SOURCE_DIR}/xs/sources/diagnostic.c"
  sources/lexer.c
  sources/model.c
  sources/parser.c
  sources/parser_main.c
)
target_include_directories(xsproj PUBLIC "${PROJECT_SOURCE_DIR}/include" include)
target_compile_options(xsproj PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
target_compile_options(xsproj PUBLIC -include "${XS_COMPILER_CHECK_HEADER}")

add_executable(xs_proj sources/main.c)
set_target_properties(xs_proj PROPERTIES OUTPUT_NAME xs-proj)
set_target_properties(xs_proj PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
target_link_libraries(xs_proj PRIVATE xsproj)
target_compile_definitions(xs_proj PRIVATE XS_PROJECT_VERSION="${PROJECT_VERSION}")
