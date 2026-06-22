#include "xs/ast.h"
#include "xs/diagnostic.h"
#include "xs/parser.h"
#include "xs/project.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static bool check_source(const char *path)
{
  size_t length = 0;
  char *text = read_file(path, &length);
  if (text == NULL) {
    fprintf(stderr, "xs: '%s' kaynak dosyası okunamadı\n", path);
    return false;
  }

  XsSource source = {.path = path, .text = text, .length = length};
  XsDiagnostics diagnostics;
  XsAst ast;
  XsParser parser;
  xs_diagnostics_init(&diagnostics);
  xs_ast_init(&ast);
  xs_parser_init(&parser, &source, &diagnostics);
  bool success = xs_parser_parse(&parser, &ast);
  xs_diagnostics_print(&diagnostics, &source, stderr);
  xs_ast_free(&ast);
  xs_diagnostics_free(&diagnostics);
  free(text);
  return success;
}

static bool check_value_source(const char *manifest_path, const XsProjectValue *value)
{
  if (value == NULL || value->is_nil || value->text == NULL)
    return true;
  char *path = project_path(manifest_path, value->text);
  if (path == NULL) {
    fprintf(stderr, "xs: kaynak yolu oluşturulurken bellek tükendi\n");
    return false;
  }
  bool success = check_source(path);
  free(path);
  return success;
}

static bool check_project_sources(const char *manifest_path, const XsProject *project)
{
  if (project->xs_version.is_nil)
    return true;

  bool success = true;
  if (!project->entry.is_nil && project->entry.text != NULL)
    success = check_value_source(manifest_path, &project->entry);
  for (size_t i = 0; i < project->additional_file_count; ++i)
    success = check_value_source(manifest_path, &project->additional_files[i]) && success;
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
