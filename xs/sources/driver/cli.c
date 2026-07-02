#include "xs/driver.h"

#include "xs/diagnostic.h"
#include "xs/hir/expression_check.h"
#include "xs/hir/module_registry.h"
#include "xs/hir/symbol_table.h"
#include "xs/hir/type_resolution.h"
#include "xs/macro.h"
#include "xs/project.h"
#include "xs/syntax_ast.h"
#include "xs/syntax_parser.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum
{
  XS_BUILD_OUTPUT_NONE,
  XS_BUILD_OUTPUT_HIR,
  XS_BUILD_OUTPUT_MIR,
  XS_BUILD_OUTPUT_XLIL,
} XsBuildOutput;

typedef struct
{
  const char *command;
  const char *manifest_path;
  XsBuildOutput output;
} XsCliOptions;

static const char *output_extension(XsBuildOutput output)
{
  switch (output) {
  case XS_BUILD_OUTPUT_HIR:
    return ".xhir";
  case XS_BUILD_OUTPUT_MIR:
    return ".xmir";
  case XS_BUILD_OUTPUT_XLIL:
    return ".xlil";
  case XS_BUILD_OUTPUT_NONE:
    return "";
  }
  return "";
}

static char *copy_text(const char *text)
{
  size_t length = strlen(text);
  char *copy = malloc(length + 1);
  if (copy != nullptr)
    memcpy(copy, text, length + 1);
  return copy;
}

static char *read_file(const char *path, size_t *length)
{
  FILE *file = fopen(path, "rb");
  if (file == nullptr)
    return nullptr;
  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    return nullptr;
  }
  long size = ftell(file);
  if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);
    return nullptr;
  }
  if ((uintmax_t)size > (uintmax_t)SIZE_MAX - 1U) {
    fclose(file);
    return nullptr;
  }
  size_t file_size = (size_t)size;
  char *text = calloc(file_size + 1U, 1U);
  if (text == nullptr) {
    fclose(file);
    return nullptr;
  }
  size_t read = fread(text, 1, file_size, file);
  fclose(file);
  if (read != file_size) {
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

static char *project_path(const char *manifest_path, const char *source_path)
{
  if (source_path[0] == '/') {
    size_t length = strlen(source_path);
    char *result = malloc(length + 1);
    if (result != nullptr)
      memcpy(result, source_path, length + 1);
    return result;
  }
  const char *slash = strrchr(manifest_path, '/');
  size_t directory_length = slash == nullptr ? 0 : (size_t)(slash - manifest_path + 1);
  size_t source_length = strlen(source_path);
  char *result = malloc(directory_length + source_length + 1);
  if (result == nullptr)
    return nullptr;
  memcpy(result, manifest_path, directory_length);
  memcpy(result + directory_length, source_path, source_length + 1);
  return result;
}

static char *project_root(const char *manifest_path)
{
  const char *slash = strrchr(manifest_path, '/');
  if (slash == nullptr)
    return copy_text(".");
  size_t length = slash == manifest_path ? 1 : (size_t)(slash - manifest_path);
  char *root = malloc(length + 1);
  if (root != nullptr) {
    memcpy(root, manifest_path, length);
    root[length] = '\0';
  }
  return root;
}

static char *build_output_path(const char *manifest_path, XsBuildOutput output)
{
  const char *extension = output_extension(output);
  const char *slash = strrchr(manifest_path, '/');
  const char *base = slash == nullptr ? manifest_path : slash + 1;
  size_t base_length = strlen(base);
  if (base_length >= 7 && strcmp(base + base_length - 7, ".xsproj") == 0)
    base_length -= 7;
  size_t extension_length = strlen(extension);
  char *path = malloc(base_length + extension_length + 1);
  if (path == nullptr)
    return nullptr;
  for (size_t i = 0; i < base_length; ++i)
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
  if (unit->imports_initialized)
    xs_hir_import_scope_free(&unit->imports);
  if (unit->macro_declarations_initialized)
    xs_macro_declaration_expansion_set_free(&unit->macro_declarations);
  if (unit->macro_statements_initialized)
    xs_macro_statement_expansion_set_free(&unit->macro_statements);
  if (unit->tree_initialized)
    xs_syntax_tree_free(&unit->tree);
  if (unit->diagnostics_initialized)
    xs_diagnostics_free(&unit->diagnostics);
  free(unit->text);
  free(unit->path);
  *unit = (CompilationUnit){0};
}

static bool unit_path_exists(const CompilationUnit *units, size_t count, const char *path)
{
  for (size_t i = 0; i < count; ++i) {
    if (strcmp(units[i].path, path) == 0)
      return true;
  }
  return false;
}

static bool append_compilation_unit(CompilationUnit **units, size_t *count, size_t *capacity, char *path)
{
  if (unit_path_exists(*units, *count, path)) {
    free(path);
    return true;
  }
  if (*count == *capacity) {
    size_t new_capacity = *capacity == 0 ? 8 : *capacity * 2;
    CompilationUnit *grown = realloc(*units, new_capacity * sizeof(*grown));
    if (grown == nullptr) {
      free(path);
      return false;
    }
    *units = grown;
    *capacity = new_capacity;
  }
  (*units)[(*count)++] = (CompilationUnit){.path = path};
  return true;
}

static bool parse_compilation_unit(CompilationUnit *unit, uint64_t file_id, XsHirSymbolTable *symbols)
{
  size_t length = 0;
  unit->text = read_file(unit->path, &length);
  if (unit->text == nullptr) {
    fprintf(stderr, "xs: '%s' kaynak dosyası okunamadı\n", unit->path);
    return false;
  }
  unit->source = (XsSource){.path = unit->path, .text = unit->text, .length = length};
  xs_diagnostics_init(&unit->diagnostics);
  unit->diagnostics_initialized = true;
  xs_hir_import_scope_init(&unit->imports);
  unit->imports_initialized = true;
  bool success = xs_syntax_parse(&unit->source, file_id, &unit->diagnostics, &unit->tree);
  unit->tree_initialized = true;
  if (success)
    success = xs_macro_validate(&unit->tree, &unit->diagnostics);
  if (success) {
    XsMacroExpansionReport macro_report;
    success = xs_macro_prepare_expansion(&unit->tree, &unit->diagnostics, &macro_report);
  }
  if (success) {
    success = xs_macro_expand_statements(&unit->tree, &unit->diagnostics, &unit->macro_statements);
    unit->macro_statements_initialized = success;
  }
  if (success)
    success = xs_macro_expand_declarations(&unit->tree, &unit->diagnostics, &unit->macro_declarations);
  unit->macro_declarations_initialized = success;
  if (success)
    success = xs_hir_collect_symbols_expanded(&unit->tree, &unit->macro_declarations, symbols, &unit->diagnostics);
  unit->hir_ready = success;
  return success;
}

static bool emit_requested_output(XsBuildOutput output, const XsHirSymbolTable *symbols, const char *manifest_path)
{
  if (output == XS_BUILD_OUTPUT_NONE)
    return true;
  (void)symbols;
  char *path = build_output_path(manifest_path, output);
  if (path == nullptr) {
    fprintf(stderr, "xs: çıktı dosyası hazırlanırken bellek tükendi\n");
    return false;
  }
  fprintf(stderr, "xs: '%s' üretilmedi; structural AST tamamlanmadan resmi %s kodu üretimi yapılmaz\n", path,
          output_extension(output));
  free(path);
  return false;
}

static bool check_project_sources(const char *manifest_path, const XsProject *project, XsBuildOutput output)
{
  if (project->xs_version.is_nil || xs_project_selected_entry(project) == nullptr)
    return true;

  size_t direct_count = project->additional_file_count;
  if (!project->entry.is_nil && project->entry.text != nullptr)
    ++direct_count;
  const char **direct = calloc(direct_count, sizeof(*direct));
  char *root = project_root(manifest_path);
  if (direct == nullptr || root == nullptr) {
    free(direct);
    free(root);
    fprintf(stderr, "xs: proje grafiği hazırlanırken bellek tükendi\n");
    return false;
  }
  size_t direct_index = 0;
  if (!project->entry.is_nil && project->entry.text != nullptr)
    direct[direct_index++] = project->entry.text;
  for (size_t i = 0; i < project->additional_file_count; ++i) {
    if (!project->additional_files[i].is_nil && project->additional_files[i].text != nullptr)
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
  if (success)
    success = xs_module_graph_resolve(root, direct, direct_count, &registry, &graph, &issues);
  xs_module_issues_print(&issues);

  CompilationUnit *units = nullptr;
  size_t unit_count = 0;
  size_t unit_capacity = 0;
  for (size_t i = 0; i < direct_count; ++i) {
    char *path = direct[i][0] == '/' ? copy_text(direct[i]) : project_path(manifest_path, direct[i]);
    if (path == nullptr || !append_compilation_unit(&units, &unit_count, &unit_capacity, path))
      success = false;
  }
  for (size_t i = 0; i < graph.count; ++i) {
    char *path = copy_text(graph.dependencies[i].imported_path);
    if (path == nullptr || !append_compilation_unit(&units, &unit_count, &unit_capacity, path))
      success = false;
  }

  XsHirSymbolTable symbols;
  xs_hir_symbol_table_init(&symbols);
  uint64_t file_id = 1;
  for (size_t i = 0; i < unit_count; ++i)
    success = parse_compilation_unit(&units[i], file_id++, &symbols) && success;
  if (success) {
    for (size_t i = 0; i < unit_count; ++i) {
      if (units[i].hir_ready) {
        success = xs_hir_resolve_imports(&units[i].tree, &symbols, &units[i].imports, &units[i].diagnostics) && success;
        success = xs_hir_validate_name_uses_with_macros(&units[i].tree, &units[i].macro_declarations,
                                                        &units[i].macro_statements, &symbols, &units[i].imports,
                                                        &units[i].diagnostics) &&
                  success;
        success = xs_hir_resolve_types_with_macros(&units[i].tree, &units[i].macro_declarations,
                                                   &units[i].macro_statements, &symbols, &units[i].imports,
                                                   &units[i].diagnostics) &&
                  success;
        success = xs_hir_check_expression_types_with_macros(&units[i].tree, &units[i].macro_declarations,
                                                            &units[i].macro_statements, &units[i].diagnostics) &&
                  success;
      }
    }
  }
  for (size_t i = 0; i < unit_count; ++i)
    xs_diagnostics_print(&units[i].diagnostics, &units[i].source, stderr);
  if (success)
    success = emit_requested_output(output, &symbols, manifest_path);

  xs_hir_symbol_table_free(&symbols);
  for (size_t i = 0; i < unit_count; ++i)
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
  if (!has_suffix(options->manifest_path, ".xsproj")) {
    fprintf(stderr, "xs: -proj bir .xsproj dosya yolu ile kullanılmalıdır\n");
    return 2;
  }
  size_t length = 0;
  char *text = read_file(options->manifest_path, &length);
  if (text == nullptr) {
    fprintf(stderr, "xs: '%s' proje dosyası okunamadı\n", options->manifest_path);
    return 2;
  }

  XsSource source = {.path = options->manifest_path, .text = text, .length = length};
  XsDiagnostics diagnostics;
  XsProject project;
  xs_diagnostics_init(&diagnostics);
  xs_project_init(&project);
  bool success = xs_project_parse(&source, &diagnostics, &project);
  xs_diagnostics_print(&diagnostics, &source, stderr);
  if (success)
    success = check_project_sources(options->manifest_path, &project, options->output);

  if (success && strcmp(options->command, "check") != 0 && options->output == XS_BUILD_OUTPUT_NONE) {
    fprintf(stderr, "xs: %s henüz kullanılamıyor; MIR, XLIL lowering, nesne kodu ve bağlama aşamaları tamamlanmadı\n",
            options->command);
    success = false;
  }

  xs_project_free(&project);
  xs_diagnostics_free(&diagnostics);
  free(text);
  return success ? 0 : 1;
}

static bool parse_output_kind(const char *text, XsBuildOutput *output)
{
  if (strcmp(text, "hir") == 0) {
    *output = XS_BUILD_OUTPUT_HIR;
    return true;
  }
  if (strcmp(text, "mir") == 0) {
    *output = XS_BUILD_OUTPUT_MIR;
    return true;
  }
  if (strcmp(text, "xlil") == 0) {
    *output = XS_BUILD_OUTPUT_XLIL;
    return true;
  }
  return false;
}

static bool parse_cli(int argc, char **argv, XsCliOptions *options)
{
  if (argc < 4)
    return false;
  if (strcmp(argv[1], "check") != 0 && strcmp(argv[1], "build") != 0 && strcmp(argv[1], "run") != 0)
    return false;
  *options = (XsCliOptions){.command = argv[1]};
  for (int i = 2; i < argc; ++i) {
    if (strcmp(argv[i], "-proj") == 0) {
      if (++i >= argc || options->manifest_path != nullptr)
        return false;
      options->manifest_path = argv[i];
    } else if (strcmp(argv[i], "--output") == 0) {
      if (++i >= argc || options->output != XS_BUILD_OUTPUT_NONE)
        return false;
      if (!parse_output_kind(argv[i], &options->output))
        return false;
    } else {
      return false;
    }
  }
  if (options->manifest_path == nullptr)
    return false;
  return strcmp(options->command, "build") == 0 || options->output == XS_BUILD_OUTPUT_NONE;
}

int xs_driver_main(int argc, char **argv)
{
  XsCliOptions options = {0};
  if (!parse_cli(argc, argv, &options)) {
    fprintf(stderr, "kullanım: xs <check|run> -proj <proje.xsproj>\n");
    fprintf(stderr, "kullanım: xs build [--output hir|mir|xlil] -proj <proje.xsproj>\n");
    return 2;
  }
  return run_project_command(&options);
}
