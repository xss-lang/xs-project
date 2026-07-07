# SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
# SPDX-License-Identifier: Apache-2.0

# XS monorepo project registry.
#
# This mirrors the LLVM-style "enable projects from one checkout" model.

set(XS_MONOREPO_STABLE_PROJECTS xs xsproj)
set(XS_MONOREPO_FUTURE_PROJECTS xsfmt xstidy xs-analyzer xs-backend)
set(XS_MONOREPO_STABLE_RUNTIMES)
set(XS_MONOREPO_FUTURE_RUNTIMES xsrt)

set(XS_ENABLE_PROJECTS
    "xs"
    CACHE STRING "Semicolon-separated XS monorepo projects to enable. Use 'all' for all stable projects.")
set(XS_ENABLE_RUNTIMES
    ""
    CACHE STRING "Semicolon-separated XS monorepo runtimes to enable. Use 'all' for all stable runtimes.")

function(xs_monorepo_normalize_list out_variable input_items all_items)
  if("${input_items}" STREQUAL "all")
    set(${out_variable} ${all_items} PARENT_SCOPE)
    return()
  endif()

  string(REPLACE "," ";" normalized "${input_items}")
  set(result)
  foreach(item_name IN LISTS normalized)
    string(STRIP "${item_name}" item_name)
    if(item_name STREQUAL "")
      continue()
    endif()
    list(APPEND result "${item_name}")
  endforeach()
  set(${out_variable} ${result} PARENT_SCOPE)
endfunction()

function(xs_monorepo_validate_projects)
  xs_monorepo_normalize_list(enabled_projects "${XS_ENABLE_PROJECTS}" "${XS_MONOREPO_STABLE_PROJECTS}")
  if(NOT enabled_projects)
    message(FATAL_ERROR "XS_ENABLE_PROJECTS must name at least one monorepo project.")
  endif()

  foreach(project_name IN LISTS enabled_projects)
    if(project_name IN_LIST XS_MONOREPO_STABLE_PROJECTS)
      continue()
    endif()
    if(project_name IN_LIST XS_MONOREPO_FUTURE_PROJECTS)
      message(FATAL_ERROR
              "Monorepo project '${project_name}' is registered as a future sibling project, but it is not buildable yet.")
    endif()
    message(FATAL_ERROR "Unknown XS monorepo project '${project_name}'.")
  endforeach()

  set(XS_ENABLED_PROJECTS ${enabled_projects} CACHE INTERNAL "Normalized XS monorepo projects")
endfunction()

function(xs_monorepo_validate_runtimes)
  xs_monorepo_normalize_list(enabled_runtimes "${XS_ENABLE_RUNTIMES}" "${XS_MONOREPO_STABLE_RUNTIMES}")

  foreach(runtime_name IN LISTS enabled_runtimes)
    if(runtime_name IN_LIST XS_MONOREPO_STABLE_RUNTIMES)
      continue()
    endif()
    if(runtime_name IN_LIST XS_MONOREPO_FUTURE_RUNTIMES)
      message(FATAL_ERROR
              "Monorepo runtime '${runtime_name}' is registered as a future runtime, but it is not buildable yet.")
    endif()
    message(FATAL_ERROR "Unknown XS monorepo runtime '${runtime_name}'.")
  endforeach()

  set(XS_ENABLED_RUNTIMES ${enabled_runtimes} CACHE INTERNAL "Normalized XS monorepo runtimes")
endfunction()

function(xs_monorepo_project_enabled project_name out_variable)
  if(project_name IN_LIST XS_ENABLED_PROJECTS)
    set(${out_variable} TRUE PARENT_SCOPE)
  else()
    set(${out_variable} FALSE PARENT_SCOPE)
  endif()
endfunction()

function(xs_monorepo_runtime_enabled runtime_name out_variable)
  if(runtime_name IN_LIST XS_ENABLED_RUNTIMES)
    set(${out_variable} TRUE PARENT_SCOPE)
  else()
    set(${out_variable} FALSE PARENT_SCOPE)
  endif()
endfunction()
