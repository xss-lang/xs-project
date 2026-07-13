/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xs/driver.h"

#include "compiler_core_native.h"
#include "direct_xlil.h"
#include "options.h"
#include "project_driver.h"
#include "source_native.h"

#include "xs/compiler_core.h"
#include "xs/diagnostic.h"
#include "xs/hir/cffi.h"
#include "xs/hir/expression_check.h"
#include "xs/hir/inheritance.h"
#include "xs/hir/module_registry.h"
#include "xs/hir/symbol_table.h"
#include "xs/hir/type_resolution.h"
#include "xs/macro.h"
#include "xs/project.h"
#include "xs/source_include.h"
#include "xs/syntax_ast.h"
#include "xs/syntax_parser.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef XS_PROJECT_VERSION
#define XS_PROJECT_VERSION "0.1.6"
#endif

static char *copy_text(const char *text)
{
  size_t length = strlen(text);
  char *copy = malloc(length + 1);
  if(copy != nullptr)
    memcpy(copy, text, length + 1);
  return copy;
}

static void print_verbose_settings(const XsCliOptions *options, const XsCompilerSettings *settings, const char *input)
{
  if(!settings->verbose)
    return;
  fprintf(stderr, "xs: verbose: command=%s input=%s warning=%s werrror=%s\n", options->command, input,
          xs_cli_warning_level_name(settings->warning_level), settings->warnings_as_errors ? "true" : "false");
}

static char *read_file(const char *path, size_t *length)
{
  FILE *file = fopen(path, "rb");
  if(file == nullptr)
    return nullptr;
  if(fseek(file, 0, SEEK_END) != 0)
  {
    fclose(file);
    return nullptr;
  }
  long size = ftell(file);
  if(size < 0 || fseek(file, 0, SEEK_SET) != 0)
  {
    fclose(file);
    return nullptr;
  }
  if((uintmax_t)size > (uintmax_t)SIZE_MAX - 1U)
  {
    fclose(file);
    return nullptr;
  }
  size_t file_size = (size_t)size;
  char *text = calloc(file_size + 1U, 1U);
  if(text == nullptr)
  {
    fclose(file);
    return nullptr;
  }
  size_t read = fread(text, 1, file_size, file);
  fclose(file);
  if(read != file_size)
  {
    free(text);
    return nullptr;
  }
  *length = read;
  return text;
}

static bool has_suffix(const char *text, const char *suffix)
{
  size_t text_length = strlen(text);
  size_t suffix_length = strlen(suffix);
  return text_length >= suffix_length && strcmp(text + text_length - suffix_length, suffix) == 0;
}

static const char *ir_version_prefix(XsBuildOutput output)
{
  switch(output)
  {
  case XS_BUILD_OUTPUT_HIR:
    return ".xhir version ";
  case XS_BUILD_OUTPUT_MIR:
    return ".xmir version ";
  case XS_BUILD_OUTPUT_XLIL:
    return ".xlil version ";
  case XS_BUILD_OUTPUT_NONE:
    return "";
  }
  return "";
}

static const char *ir_kind_name(XsBuildOutput output)
{
  switch(output)
  {
  case XS_BUILD_OUTPUT_HIR:
    return "XHIR";
  case XS_BUILD_OUTPUT_MIR:
    return "XMIR";
  case XS_BUILD_OUTPUT_XLIL:
    return "XLIL";
  case XS_BUILD_OUTPUT_NONE:
    return "output";
  }
  return "output";
}

static bool is_direct_ir_input(const XsCliOptions *options)
{
  return options->output != XS_BUILD_OUTPUT_NONE &&
         has_suffix(options->file_path, xs_cli_output_extension(options->output));
}

static bool supported_ir_version(uint32_t version)
{
  return version == 0;
}

static bool parse_ir_version_line(const char *line, const char *prefix, uint32_t *version)
{
  size_t prefix_length = strlen(prefix);
  if(strncmp(line, prefix, prefix_length) != 0)
    return false;
  const char *digits = line + prefix_length;
  if(*digits == '\0')
    return false;
  uint32_t value = 0;
  for(const char *cursor = digits; *cursor != '\0'; ++cursor)
  {
    if(*cursor < '0' || *cursor > '9')
      return false;
    uint32_t digit = (uint32_t)(*cursor - '0');
    if(value > (UINT32_MAX - digit) / 10U)
      return false;
    value = value * 10U + digit;
  }
  *version = value;
  return true;
}

static bool validate_direct_ir_version(XsBuildOutput output, const char *path, const char *text, size_t length)
{
  const char *prefix = ir_version_prefix(output);
  size_t line_length = 0;
  while(line_length < length && text[line_length] != '\n' && text[line_length] != '\r')
    ++line_length;
  char *line = malloc(line_length + 1U);
  if(line == nullptr)
  {
    fprintf(stderr, "xs: out of memory while checking %s version\n", ir_kind_name(output));
    return false;
  }
  memcpy(line, text, line_length);
  line[line_length] = '\0';
  uint32_t version = 0;
  bool parsed = parse_ir_version_line(line, prefix, &version);
  free(line);
  if(!parsed)
  {
    fprintf(stderr, "xs: %s file '%s' has an invalid version header\n", ir_kind_name(output), path);
    return false;
  }
  if(!supported_ir_version(version))
  {
    fprintf(stderr, "xs: %s version %" PRIu32 " is not supported; supported version is 0\n", ir_kind_name(output),
            version);
    return false;
  }
  return true;
}

static char *project_path(const char *manifest_path, const char *source_path)
{
  if(source_path[0] == '/')
  {
    size_t length = strlen(source_path);
    char *result = malloc(length + 1);
    if(result != nullptr)
      memcpy(result, source_path, length + 1);
    return result;
  }
  const char *slash = strrchr(manifest_path, '/');
  size_t directory_length = slash == nullptr ? 0 : (size_t)(slash - manifest_path + 1);
  size_t source_length = strlen(source_path);
  char *result = malloc(directory_length + source_length + 1);
  if(result == nullptr)
    return nullptr;
  memcpy(result, manifest_path, directory_length);
  memcpy(result + directory_length, source_path, source_length + 1);
  return result;
}

static char *project_root(const char *manifest_path)
{
  const char *slash = strrchr(manifest_path, '/');
  if(slash == nullptr)
    return copy_text(".");
  size_t length = slash == manifest_path ? 1 : (size_t)(slash - manifest_path);
  char *root = malloc(length + 1);
  if(root != nullptr)
  {
    memcpy(root, manifest_path, length);
    root[length] = '\0';
  }
  return root;
}

static char *build_output_path(const char *manifest_path, XsBuildOutput output)
{
  const char *extension = xs_cli_output_extension(output);
  const char *slash = strrchr(manifest_path, '/');
  const char *base = slash == nullptr ? manifest_path : slash + 1;
  size_t base_length = strlen(base);
  if(base_length >= 7 && strcmp(base + base_length - 7, ".xsproj") == 0)
    base_length -= 7;
  size_t extension_length = strlen(extension);
  char *path = malloc(base_length + extension_length + 1);
  if(path == nullptr)
    return nullptr;
  for(size_t i = 0; i < base_length; ++i)
    path[i] = base[i];
  memcpy(path + base_length, extension, extension_length + 1);
  return path;
}

typedef struct
{
  char *path;
  char *text;
  XsSource source;
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  XsMacroStatementExpansionSet macro_statements;
  XsMacroDeclarationExpansionSet macro_declarations;
  XsCompilerCoreSession *compiler_core;
  XsHirImportScope imports;
  bool diagnostics_initialized;
  bool tree_initialized;
  bool macro_statements_initialized;
  bool macro_declarations_initialized;
  bool imports_initialized;
  bool hir_ready;
} CompilationUnit;

static void compilation_unit_free(CompilationUnit *unit)
{
  xslang_compiler_core_session_free(unit->compiler_core);
  if(unit->imports_initialized)
    xs_hir_import_scope_free(&unit->imports);
  if(unit->macro_declarations_initialized)
    xs_macro_declaration_expansion_set_free(&unit->macro_declarations);
  if(unit->macro_statements_initialized)
    xs_macro_statement_expansion_set_free(&unit->macro_statements);
  if(unit->tree_initialized)
    xs_syntax_tree_free(&unit->tree);
  if(unit->diagnostics_initialized)
    xs_diagnostics_free(&unit->diagnostics);
  free(unit->text);
  free(unit->path);
  *unit = (CompilationUnit){0};
}

static bool unit_path_exists(const CompilationUnit *units, size_t count, const char *path)
{
  for(size_t i = 0; i < count; ++i)
  {
    if(strcmp(units[i].path, path) == 0)
      return true;
  }
  return false;
}

static bool append_compilation_unit(CompilationUnit **units, size_t *count, size_t *capacity, char *path)
{
  if(unit_path_exists(*units, *count, path))
  {
    free(path);
    return true;
  }
  if(*count == *capacity)
  {
    size_t new_capacity = *capacity == 0 ? 8 : *capacity * 2;
    CompilationUnit *grown = realloc(*units, new_capacity * sizeof(*grown));
    if(grown == nullptr)
    {
      free(path);
      return false;
    }
    *units = grown;
    *capacity = new_capacity;
  }
  (*units)[(*count)++] = (CompilationUnit){.path = path};
  return true;
}

static bool import_compiler_core_syntax(CompilationUnit *unit)
{
  XsSpan root_span = {.start = unit->tree.root->span.start_offset, .end = unit->tree.root->span.end_offset};
  XsSyntaxTree expanded = {0};
  if(!xs_macro_materialize_expanded_tree(&unit->tree, &unit->macro_declarations, &unit->macro_statements,
                                         &unit->diagnostics, &expanded))
    return false;
  XsCompilerCoreSyntaxStorage *storage = nullptr;
  XsCompilerCoreStatus packet_status = xs_compiler_core_syntax_packet_create(&expanded, &storage);
  if(packet_status != XS_COMPILER_CORE_OK)
  {
    xs_syntax_tree_free(&expanded);
    return xs_diagnostics_add(&unit->diagnostics, XS_DIAGNOSTIC_ERROR, root_span,
                              "compiler-core syntax packet could not be created") &&
           false;
  }
  const XsCompilerCoreSyntaxPacket *packet = xs_compiler_core_syntax_packet(storage);
  XsCompilerCoreFfiStatus import_status = xslang_compiler_core_session_create(packet, &unit->compiler_core);
  xs_compiler_core_syntax_packet_free(storage);
  xs_syntax_tree_free(&expanded);
  if(import_status == XS_COMPILER_CORE_FFI_OK)
    return true;
  return xs_diagnostics_add(&unit->diagnostics, XS_DIAGNOSTIC_ERROR, root_span,
                            "Rust compiler core rejected the expanded structural AST packet") &&
         false;
}

static bool parse_compilation_unit(CompilationUnit *unit, uint64_t file_id, XsHirSymbolTable *symbols,
                                   const XsCompilerSettings *settings)
{
  size_t length = 0;
  unit->text = read_file(unit->path, &length);
  if(unit->text == nullptr)
  {
    fprintf(stderr, "xs: source file '%s' could not be read\n", unit->path);
    return false;
  }
  xs_diagnostics_init(&unit->diagnostics);
  xs_diagnostics_set_warning_policy(&unit->diagnostics, settings->warning_level, settings->warnings_as_errors);
  unit->diagnostics_initialized = true;
  unit->source = (XsSource){.path = unit->path, .text = unit->text, .length = length};
  xs_hir_import_scope_init(&unit->imports);
  unit->imports_initialized = true;
  bool success = xs_syntax_parse(&unit->source, file_id, &unit->diagnostics, &unit->tree);
  unit->tree_initialized = true;
  XsIncludedSource included = {0};
  if(success)
    success = xs_source_expand_include_macros(&unit->tree, &unit->diagnostics, &included);
  if(success)
  {
    xs_syntax_tree_free(&unit->tree);
    free(unit->text);
    unit->text = included.text;
    included.text = nullptr;
    unit->source = (XsSource){.path = unit->path, .text = unit->text, .length = included.length};
    success = xs_syntax_parse(&unit->source, file_id, &unit->diagnostics, &unit->tree);
  }
  xs_included_source_free(&included);
  if(success)
    success = xs_macro_validate(&unit->tree, &unit->diagnostics);
  if(success)
  {
    XsMacroExpansionReport macro_report;
    success = xs_macro_prepare_expansion(&unit->tree, &unit->diagnostics, &macro_report);
  }
  if(success)
  {
    success = xs_macro_expand_statements(&unit->tree, &unit->diagnostics, &unit->macro_statements);
    unit->macro_statements_initialized = success;
  }
  if(success)
    success = xs_macro_expand_declarations(&unit->tree, &unit->diagnostics, &unit->macro_declarations);
  unit->macro_declarations_initialized = success;
  if(success)
    success = import_compiler_core_syntax(unit);
  if(success)
    success = xs_hir_collect_symbols_expanded(&unit->tree, &unit->macro_declarations, symbols, &unit->diagnostics);
  unit->hir_ready = success;
  return success;
}

static bool emit_requested_output(XsBuildOutput output, const XsHirSymbolTable *symbols, const char *manifest_path)
{
  if(output == XS_BUILD_OUTPUT_NONE)
    return true;
  (void)symbols;
  char *path = build_output_path(manifest_path, output);
  if(path == nullptr)
  {
    fprintf(stderr, "xs: out of memory while preparing the output file\n");
    return false;
  }
  fprintf(stderr, "xs: '%s' was not produced; official %s code emission waits for structural AST completion\n", path,
          xs_cli_output_extension(output));
  free(path);
  return false;
}

static bool check_compilation_unit_semantics(CompilationUnit *unit, XsHirSymbolTable *symbols)
{
  if(!unit->hir_ready)
    return false;
  bool success = xs_hir_resolve_imports(&unit->tree, symbols, &unit->imports, &unit->diagnostics);
  success = xs_hir_validate_cffi(&unit->tree, &unit->diagnostics) && success;
  success = xs_hir_validate_name_uses_with_macros(&unit->tree, &unit->macro_declarations, &unit->macro_statements,
                                                  symbols, &unit->imports, &unit->diagnostics) &&
            success;
  success = xs_hir_resolve_types_with_macros(&unit->tree, &unit->macro_declarations, &unit->macro_statements, symbols,
                                             &unit->imports, &unit->diagnostics) &&
            success;
  success = xs_hir_validate_inheritance(&unit->tree, symbols, &unit->imports, &unit->diagnostics) && success;
  return xs_hir_check_expression_types_with_macros(&unit->tree, &unit->macro_declarations, &unit->macro_statements,
                                                   &unit->diagnostics) &&
         success;
}

static bool check_single_source_file(const char *path, bool build_native, const XsCompilerSettings *settings)
{
  CompilationUnit unit = {.path = copy_text(path)};
  if(unit.path == nullptr)
  {
    fprintf(stderr, "xs: out of memory while preparing source file '%s'\n", path);
    return false;
  }
  XsHirSymbolTable symbols;
  xs_hir_symbol_table_init(&symbols);
  bool success = parse_compilation_unit(&unit, 1, &symbols, settings);
  if(success)
    success = check_compilation_unit_semantics(&unit, &symbols);
  xs_diagnostics_print(&unit.diagnostics, &unit.source, stderr);
  if(success && build_native)
  {
    if(xs_driver_compiler_core_native_available(unit.compiler_core))
    {
      XsSpan span = {.start = unit.tree.root->span.start_offset, .end = unit.tree.root->span.end_offset};
      success = xs_driver_build_compiler_core_native(path, unit.compiler_core, &unit.diagnostics, span);
    }
    else
      success = xs_driver_build_source_native(path, &unit.tree, &unit.diagnostics);
  }
  if(!success && build_native)
    xs_diagnostics_print(&unit.diagnostics, &unit.source, stderr);
  xs_hir_symbol_table_free(&symbols);
  compilation_unit_free(&unit);
  return success;
}

static XsCompilerCoreSession *merge_compiler_core_sessions(CompilationUnit *units, size_t unit_count)
{
  const XsCompilerCoreSession **sessions = calloc(unit_count, sizeof(*sessions));
  if(sessions == nullptr)
    return nullptr;
  for(size_t i = 0; i < unit_count; ++i)
    sessions[i] = units[i].compiler_core;
  XsCompilerCoreSession *merged = nullptr;
  XsCompilerCoreFfiStatus status = xslang_compiler_core_session_merge(sessions, unit_count, &merged);
  free(sessions);
  return status == XS_COMPILER_CORE_FFI_OK ? merged : nullptr;
}

static bool check_project_sources(const char *manifest_path, const XsProject *project, XsBuildOutput output,
                                  bool build_native, const XsCompilerSettings *settings)
{
  if(project->xs_version.is_nil || xs_project_selected_entry(project) == nullptr)
  {
    if(build_native)
      fprintf(stderr, "xs: project native build requires compilerOptions.xsVersion and a selected entry source\n");
    return !build_native;
  }

  size_t direct_count = project->additional_file_count;
  if(!project->entry.is_nil && project->entry.text != nullptr)
    ++direct_count;
  const char **direct = calloc(direct_count, sizeof(*direct));
  char *root = project_root(manifest_path);
  if(direct == nullptr || root == nullptr)
  {
    free(direct);
    free(root);
    fprintf(stderr, "xs: out of memory while preparing the project graph\n");
    return false;
  }
  size_t direct_index = 0;
  if(!project->entry.is_nil && project->entry.text != nullptr)
    direct[direct_index++] = project->entry.text;
  for(size_t i = 0; i < project->additional_file_count; ++i)
  {
    if(!project->additional_files[i].is_nil && project->additional_files[i].text != nullptr)
      direct[direct_index++] = project->additional_files[i].text;
  }
  direct_count = direct_index;

  XsModuleRegistry registry;
  XsModuleGraph graph;
  XsModuleIssues issues;
  xs_module_registry_init(&registry);
  xs_module_graph_init(&graph);
  xs_module_issues_init(&issues);
  bool success = xs_module_registry_discover(root, &registry, &issues);
  if(success)
    success = xs_module_graph_resolve(root, direct, direct_count, &registry, &graph, &issues);
  xs_module_issues_print(&issues);

  CompilationUnit *units = nullptr;
  size_t unit_count = 0;
  size_t unit_capacity = 0;
  for(size_t i = 0; i < direct_count; ++i)
  {
    char *path = direct[i][0] == '/' ? copy_text(direct[i]) : project_path(manifest_path, direct[i]);
    if(path == nullptr || !append_compilation_unit(&units, &unit_count, &unit_capacity, path))
      success = false;
  }
  for(size_t i = 0; i < graph.count; ++i)
  {
    char *path = copy_text(graph.dependencies[i].imported_path);
    if(path == nullptr || !append_compilation_unit(&units, &unit_count, &unit_capacity, path))
      success = false;
  }

  XsHirSymbolTable symbols;
  xs_hir_symbol_table_init(&symbols);
  uint64_t file_id = 1;
  for(size_t i = 0; i < unit_count; ++i)
  {
    if(settings->verbose)
      fprintf(stderr, "xs: verbose: source[%zu]=%s\n", i, units[i].path);
    success = parse_compilation_unit(&units[i], file_id++, &symbols, settings) && success;
  }
  if(success)
  {
    for(size_t i = 0; i < unit_count; ++i)
    {
      if(units[i].hir_ready)
      {
        success = check_compilation_unit_semantics(&units[i], &symbols) && success;
      }
    }
  }
  for(size_t i = 0; i < unit_count; ++i)
    xs_diagnostics_print(&units[i].diagnostics, &units[i].source, stderr);
  if(success)
    success = emit_requested_output(output, &symbols, manifest_path);
  if(success && build_native)
  {
    success = unit_count != 0;
    XsCompilerCoreSession *merged =
        success && unit_count > 1 ? merge_compiler_core_sessions(units, unit_count) : nullptr;
    const XsCompilerCoreSession *native_session = unit_count == 1 ? units[0].compiler_core : merged;
    if(success && unit_count > 1 && merged == nullptr)
    {
      XsSpan span = {.start = units[0].tree.root->span.start_offset, .end = units[0].tree.root->span.end_offset};
      success = xs_diagnostics_add(&units[0].diagnostics, XS_DIAGNOSTIC_ERROR, span,
                                   "Rust compiler core could not merge the project source sessions") &&
                false;
    }
    else if(success && xs_driver_compiler_core_native_available(native_session))
    {
      XsSpan span = {.start = units[0].tree.root->span.start_offset, .end = units[0].tree.root->span.end_offset};
      success = xs_driver_build_compiler_core_native(units[0].path, native_session, &units[0].diagnostics, span);
    }
    else if(success)
      success = xs_driver_build_source_native(units[0].path, &units[0].tree, &units[0].diagnostics);
    xslang_compiler_core_session_free(merged);
  }
  if(!success && build_native && unit_count != 0)
    xs_diagnostics_print(&units[0].diagnostics, &units[0].source, stderr);

  xs_hir_symbol_table_free(&symbols);
  for(size_t i = 0; i < unit_count; ++i)
    compilation_unit_free(&units[i]);
  free(units);
  xs_module_issues_free(&issues);
  xs_module_graph_free(&graph);
  xs_module_registry_free(&registry);
  free(direct);
  free(root);
  return success;
}

static int run_project_command(const XsCliOptions *options)
{
  XsCompilerSettings settings = xs_cli_default_compiler_settings();
  xs_cli_apply_compiler_overrides(options, &settings);
  print_verbose_settings(options, &settings, options->manifest_path);
  if(!has_suffix(options->manifest_path, ".xsproj"))
  {
    fprintf(stderr, "xs: -proj must be used with a .xsproj file path\n");
    return 2;
  }
  size_t length = 0;
  char *text = read_file(options->manifest_path, &length);
  if(text == nullptr)
  {
    fprintf(stderr, "xs: project file '%s' could not be read\n", options->manifest_path);
    return 2;
  }

  XsSource source = {.path = options->manifest_path, .text = text, .length = length};
  XsDiagnostics diagnostics;
  XsProject project;
  xs_diagnostics_init(&diagnostics);
  xs_diagnostics_set_warning_policy(&diagnostics, settings.warning_level, settings.warnings_as_errors);
  xs_project_init(&project);
  bool success = xs_project_parse(&source, &diagnostics, &project);
  xs_diagnostics_print(&diagnostics, &source, stderr);
  if(success)
    success = check_project_sources(options->manifest_path, &project, options->output,
                                    strcmp(options->command, "build") == 0 && options->output == XS_BUILD_OUTPUT_NONE,
                                    &settings);

  xs_project_free(&project);
  xs_diagnostics_free(&diagnostics);
  free(text);
  return success ? 0 : 1;
}

static int run_kotlin_project_command(const XsCliOptions *options)
{
  char **paths = nullptr;
  size_t path_count = 0;
  XsCompilerSettings settings;
  if(!xs_driver_resolve_kotlin_project(&paths, &path_count, &settings))
    return 1;
  xs_cli_apply_compiler_overrides(options, &settings);
  print_verbose_settings(options, &settings, "Kotlin project");
  XsProjectValue *additional = nullptr;
  if(path_count > 1U)
  {
    additional = calloc(path_count - 1U, sizeof(*additional));
    if(additional == nullptr)
    {
      xs_driver_free_project_paths(paths, path_count);
      return 1;
    }
    for(size_t i = 1; i < path_count; ++i)
      additional[i - 1U] = (XsProjectValue){.text = paths[i]};
  }
  XsProject project = {
      .xs_version = {.text = "kotlin-project"},
      .entry = {.text = paths[0]},
      .additional_files = additional,
      .additional_file_count = path_count - 1U,
  };
  bool success = check_project_sources(
      ".", &project, options->output, strcmp(options->command, "check") != 0 && options->output == XS_BUILD_OUTPUT_NONE,
      &settings);
  free(additional);
  xs_driver_free_project_paths(paths, path_count);
  return success ? 0 : 1;
}

static int run_file_command(const XsCliOptions *options)
{
  XsCompilerSettings settings = xs_cli_default_compiler_settings();
  xs_cli_apply_compiler_overrides(options, &settings);
  print_verbose_settings(options, &settings, options->file_path);
  if(is_direct_ir_input(options))
  {
    size_t length = 0;
    char *text = read_file(options->file_path, &length);
    if(text == nullptr)
    {
      fprintf(stderr, "xs: input file '%s' could not be read\n", options->file_path);
      return 2;
    }
    bool valid_version = validate_direct_ir_version(options->output, options->file_path, text, length);
    if(!valid_version)
    {
      free(text);
      return 1;
    }
    if(options->output == XS_BUILD_OUTPUT_XLIL)
    {
      bool success = xs_driver_build_direct_xlil(options->file_path, text, length);
      free(text);
      return success ? 0 : 1;
    }
    free(text);
    fprintf(stderr, "xs: %s direct compilation for '%s' is not wired yet\n", ir_kind_name(options->output),
            options->file_path);
    return 1;
  }
  if(options->output == XS_BUILD_OUTPUT_NONE)
    return check_single_source_file(options->file_path, strcmp(options->command, "build") == 0, &settings) ? 0 : 1;
  fprintf(stderr, "xs: %s emission for -file '%s' is not wired yet\n", xs_cli_output_extension(options->output),
          options->file_path);
  return 1;
}

int xs_driver_main(int argc, char **argv)
{
  if(argc == 2 && strcmp(argv[1], "--version") == 0)
  {
    printf("xs %s\n", XS_PROJECT_VERSION);
    return 0;
  }
  XsCliOptions options = {0};
  if(!xs_cli_parse(argc, argv, &options))
  {
    xs_cli_print_usage(stderr);
    return 2;
  }
  if(options.file_path != nullptr)
    return run_file_command(&options);
  if(options.manifest_path == nullptr)
    return run_kotlin_project_command(&options);
  return run_project_command(&options);
}
