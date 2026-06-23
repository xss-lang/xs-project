#include "xs/diagnostic.h"
#include "xs/hir/module_registry.h"
#include "xs/macro.h"
#include "xs/project.h"
#include "xs/syntax_ast.h"
#include "xs/syntax_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *copy_text(const char *text)
{
  size_t length = strlen(text);
  char *copy = malloc(length + 1);
  if (copy != NULL)
    memcpy(copy, text, length + 1);
  return copy;
}

static char *read_file(const char *path, size_t *length)
{
  FILE *file = fopen(path, "rb");
  if (file == NULL)
    return NULL;
  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    return NULL;
  }
  long size = ftell(file);
  if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);
    return NULL;
  }
  char *text = malloc((size_t)size + 1);
  if (text == NULL) {
    fclose(file);
    return NULL;
  }
  size_t read = fread(text, 1, (size_t)size, file);
  fclose(file);
  if (read != (size_t)size) {
    free(text);
    return NULL;
  }
  text[read] = '\0';
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
    if (result != NULL)
      memcpy(result, source_path, length + 1);
    return result;
  }
  const char *slash = strrchr(manifest_path, '/');
  size_t directory_length = slash == NULL ? 0 : (size_t)(slash - manifest_path + 1);
  size_t source_length = strlen(source_path);
  char *result = malloc(directory_length + source_length + 1);
  if (result == NULL)
    return NULL;
  memcpy(result, manifest_path, directory_length);
  memcpy(result + directory_length, source_path, source_length + 1);
  return result;
}

static char *project_root(const char *manifest_path)
{
  const char *slash = strrchr(manifest_path, '/');
  if (slash == NULL)
    return copy_text(".");
  size_t length = slash == manifest_path ? 1 : (size_t)(slash - manifest_path);
  char *root = malloc(length + 1);
  if (root != NULL) {
    memcpy(root, manifest_path, length);
    root[length] = '\0';
  }
  return root;
}

static bool check_source(const char *path, uint64_t file_id)
{
  size_t length = 0;
  char *text = read_file(path, &length);
  if (text == NULL) {
    fprintf(stderr, "xs: '%s' kaynak dosyası okunamadı\n", path);
    return false;
  }

  XsSource source = {.path = path, .text = text, .length = length};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  bool success = xs_syntax_parse(&source, file_id, &diagnostics, &tree);
  if (success)
    success = xs_macro_validate(&tree, &diagnostics);
  xs_diagnostics_print(&diagnostics, &source, stderr);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
  free(text);
  return success;
}

static bool check_project_sources(const char *manifest_path, const XsProject *project)
{
  if (project->xs_version.is_nil || xs_project_selected_entry(project) == NULL)
    return true;

  size_t direct_count = project->additional_file_count;
  if (!project->entry.is_nil && project->entry.text != NULL)
    ++direct_count;
  const char **direct = calloc(direct_count, sizeof(*direct));
  char *root = project_root(manifest_path);
  if (direct == NULL || root == NULL) {
    free(direct);
    free(root);
    fprintf(stderr, "xs: proje grafiği hazırlanırken bellek tükendi\n");
    return false;
  }
  size_t direct_index = 0;
  if (!project->entry.is_nil && project->entry.text != NULL)
    direct[direct_index++] = project->entry.text;
  for (size_t i = 0; i < project->additional_file_count; ++i) {
    if (!project->additional_files[i].is_nil && project->additional_files[i].text != NULL)
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

  uint64_t file_id = 1;
  for (size_t i = 0; i < direct_count; ++i) {
    char *path = direct[i][0] == '/' ? copy_text(direct[i]) : project_path(manifest_path, direct[i]);
    if (path == NULL) {
      success = false;
      continue;
    }
    success = check_source(path, file_id++) && success;
    free(path);
  }
  for (size_t i = 0; i < graph.count; ++i) {
    bool already_checked = false;
    for (size_t j = 0; j < i; ++j)
      already_checked =
          already_checked || strcmp(graph.dependencies[i].imported_path, graph.dependencies[j].imported_path) == 0;
    if (!already_checked)
      success = check_source(graph.dependencies[i].imported_path, file_id++) && success;
  }

  xs_module_issues_free(&issues);
  xs_module_graph_free(&graph);
  xs_module_registry_free(&registry);
  free(direct);
  free(root);
  return success;
}

static int run_project_command(const char *command, const char *manifest_path)
{
  if (!has_suffix(manifest_path, ".xsproj")) {
    fprintf(stderr, "xs: -proj bir .xsproj dosya yolu ile kullanılmalıdır\n");
    return 2;
  }
  size_t length = 0;
  char *text = read_file(manifest_path, &length);
  if (text == NULL) {
    fprintf(stderr, "xs: '%s' proje dosyası okunamadı\n", manifest_path);
    return 2;
  }

  XsSource source = {.path = manifest_path, .text = text, .length = length};
  XsDiagnostics diagnostics;
  XsProject project;
  xs_diagnostics_init(&diagnostics);
  xs_project_init(&project);
  bool success = xs_project_parse(&source, &diagnostics, &project);
  xs_diagnostics_print(&diagnostics, &source, stderr);
  if (success)
    success = check_project_sources(manifest_path, &project);

  if (success && strcmp(command, "check") != 0) {
    fprintf(stderr, "xs: %s henüz kullanılamıyor; HIR, MIR, LLVM IR, nesne kodu ve bağlama aşamaları tamamlanmadı\n",
            command);
    success = false;
  }

  xs_project_free(&project);
  xs_diagnostics_free(&diagnostics);
  free(text);
  return success ? 0 : 1;
}

int main(int argc, char **argv)
{
  if (argc != 4 || (strcmp(argv[1], "check") != 0 && strcmp(argv[1], "build") != 0 && strcmp(argv[1], "run") != 0) ||
      strcmp(argv[2], "-proj") != 0) {
    fprintf(stderr, "kullanım: xs <check|build|run> -proj <proje.xsproj>\n");
    return 2;
  }
  return run_project_command(argv[1], argv[3]);
}
