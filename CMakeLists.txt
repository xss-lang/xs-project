# SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.31)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/llvm-toolchain.cmake" CACHE FILEPATH
      "xs-project LLVM toolchain")
endif()
include("${CMAKE_TOOLCHAIN_FILE}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(XSMonorepo)
xs_monorepo_validate_projects()
xs_monorepo_validate_runtimes()
xs_monorepo_project_enabled(xs XS_BUILD_PROJECT_XS)
xs_monorepo_project_enabled(xsproj XS_BUILD_PROJECT_XSPROJ)
if(NOT XS_BUILD_PROJECT_XS AND NOT XS_BUILD_PROJECT_XSPROJ)
  message(FATAL_ERROR "This checkout currently has buildable monorepo projects: 'xs' and 'xsproj'.")
endif()

project(xs_project VERSION 0.1.7 LANGUAGES C)
xs_validate_llvm_toolchain()

file(RELATIVE_PATH XS_SOURCE_FROM_BINARY "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
cmake_path(NORMAL_PATH XS_SOURCE_FROM_BINARY)
if(XS_SOURCE_FROM_BINARY STREQUAL "")
  set(XS_SOURCE_FROM_BINARY ".")
endif()
set(XS_COMPILER_CHECK_HEADER "${XS_SOURCE_FROM_BINARY}/include/xs/compiler_check.h")
set(XS_BACKEND_TEST_OBJECT "xs_backend_test.o")

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
option(XS_ENABLE_SANITIZERS "Enable Clang address and undefined behavior sanitizers" OFF)
if(XS_ENABLE_SANITIZERS)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

include(XSProjects)

include(CTest)
if(BUILD_TESTING)
  include(XSTests)
endif()
