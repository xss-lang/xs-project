/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: MPL-2.0
 */

#include "project_driver.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

static const char *const REGISTRY_VERSION = "xs-project-sources-v4";

static bool run_resolver(const char *mode, const char *project_root, const char *output_path, const char *module_path)
{
  const char *program = getenv("XS_PROJECT_DRIVER");
  if(program == nullptr || program[0] == '\0')
    program = "xs-project";
  char *arguments[] = {(char *)program,     (char *)mode,        (char *)project_root,
                       (char *)output_path, (char *)module_path, nullptr};
  pid_t process = 0;
  int status = posix_spawnp(&process, program, nullptr, nullptr, arguments, environ);
  if(status != 0)
  {
    fprintf(stderr, "xs: could not start Kotlin project resolver '%s': error %d\n", program, status);
    return false;
  }
  int result = 0;
  if(waitpid(process, &result, 0) < 0 || !WIFEXITED(result))
  {
    fprintf(stderr, "xs: Kotlin project resolver terminated abnormally\n");
    return false;
  }
  return WEXITSTATUS(result) == 0;
}

static char *read_registry(const char *path, size_t *length)
{
  FILE *file = fopen(path, "rb");
  if(file == nullptr || fseek(file, 0, SEEK_END) != 0)
    goto failure;
  long size = ftell(file);
  if(size <= 0 || fseek(file, 0, SEEK_SET) != 0)
    goto failure;
  char *data = malloc((size_t)size);
  if(data == nullptr || fread(data, 1, (size_t)size, file) != (size_t)size)
  {
    free(data);
    goto failure;
  }
  fclose(file);
  *length = (size_t)size;
  return data;

failure:
  if(file != nullptr)
    fclose(file);
  return nullptr;
}

void xs_driver_free_kotlin_project(XsResolvedKotlinProject *project)
{
  if(project == nullptr)
    return;
  for(size_t i = 0; i < project->path_count; ++i)
  {
    free(project->paths[i]);
    free(project->module_names[i]);
  }
  free(project->paths);
  free(project->module_names);
  for(size_t i = 0; i < project->test_path_count; ++i)
    free(project->test_paths[i]);
  free(project->test_paths);
  *project = (XsResolvedKotlinProject){0};
}

static bool parse_bool_record(const char *text, bool *value)
{
  if(strcmp(text, "true") == 0)
  {
    *value = true;
    return true;
  }
  if(strcmp(text, "false") == 0)
  {
    *value = false;
    return true;
  }
  return false;
}

static bool parse_size_record(const char *text, size_t *value)
{
  char *end = nullptr;
  unsigned long long parsed = strtoull(text, &end, 10);
  if(text[0] == '\0' || end == nullptr || *end != '\0' || parsed > SIZE_MAX)
    return false;
  *value = (size_t)parsed;
  return true;
}

static char *copy_record(const char *text)
{
  size_t length = strlen(text);
  char *result = malloc(length + 1U);
  if(result != nullptr)
    memcpy(result, text, length + 1U);
  return result;
}

static char *next_record(char *record)
{
  return record + strlen(record) + 1U;
}

static bool parse_header(char *data, size_t record_count, size_t *source_count, size_t *module_count,
                         size_t *test_count, XsCompilerSettings *settings)
{
  *settings = xs_cli_default_compiler_settings();
  if(strcmp(data, REGISTRY_VERSION) != 0 || record_count < 8U)
    return false;
  char *warning = next_record(data);
  char *werror = next_record(warning);
  char *verbose = next_record(werror);
  char *xgc = next_record(verbose);
  char *sources = next_record(xgc);
  char *modules = next_record(sources);
  char *tests = next_record(modules);
  bool parsed_warning = false;
  for(XsWarningLevel level = XS_WARNING_NONE; level <= XS_WARNING_ALL; ++level)
  {
    if(strcmp(warning, xs_cli_warning_level_name(level)) == 0)
    {
      settings->warning_level = level;
      parsed_warning = true;
      break;
    }
  }
  if(!parsed_warning || !parse_bool_record(werror, &settings->warnings_as_errors) ||
     !parse_bool_record(verbose, &settings->verbose) || !parse_bool_record(xgc, &settings->xgc_enabled) ||
     !parse_size_record(sources, source_count) || !parse_size_record(modules, module_count) ||
     !parse_size_record(tests, test_count))
    return false;
  if(*source_count > SIZE_MAX - 8U || *test_count > SIZE_MAX - 8U - *source_count ||
     *module_count > (SIZE_MAX - 8U - *source_count - *test_count) / 2U)
    return false;
  return record_count == 8U + *source_count + (*module_count * 2U) + *test_count;
}

static bool resolve_kotlin_registry(const char *mode, const char *project_root, const char *module_path,
                                    bool require_sources, XsResolvedKotlinProject *project)
{
  if(project == nullptr)
    return false;
  *project = (XsResolvedKotlinProject){0};
  char registry_path[] = "/tmp/xs-project-sources-XXXXXX";
  int registry = mkstemp(registry_path);
  if(registry < 0)
  {
    fprintf(stderr, "xs: could not create the project source registry\n");
    return false;
  }
  close(registry);
  bool success = run_resolver(mode, project_root, registry_path, module_path);
  size_t length = 0;
  char *data = success ? read_registry(registry_path, &length) : nullptr;
  unlink(registry_path);
  if(data == nullptr || data[length - 1U] != '\0')
  {
    free(data);
    if(success)
      fprintf(stderr, "xs: Kotlin project resolver produced an invalid source registry\n");
    return false;
  }
  size_t count = 0;
  for(size_t i = 0; i < length; ++i)
    count += data[i] == '\0';
  size_t source_count = 0;
  size_t module_count = 0;
  size_t test_count = 0;
  if(count == 0U || !parse_header(data, count, &source_count, &module_count, &test_count, &project->settings) ||
     (require_sources && source_count == 0U))
  {
    free(data);
    fprintf(stderr, "xs: Kotlin project resolver returned an invalid compiler/source registry\n");
    return false;
  }
  size_t path_count = source_count + module_count;
  project->paths = calloc(path_count, sizeof(*project->paths));
  project->module_names = calloc(path_count, sizeof(*project->module_names));
  project->test_paths = test_count == 0U ? nullptr : calloc(test_count, sizeof(*project->test_paths));
  project->test_path_count = test_count;
  if((path_count != 0U && (project->paths == nullptr || project->module_names == nullptr)) ||
     (test_count != 0U && project->test_paths == nullptr))
  {
    free(data);
    xs_driver_free_kotlin_project(project);
    return false;
  }
  project->path_count = path_count;
  char *record = data;
  for(size_t i = 0; i < 8U; ++i)
    record = next_record(record);
  for(size_t i = 0; i < source_count; ++i)
  {
    project->paths[i] = copy_record(record);
    if(project->paths[i] == nullptr)
    {
      free(data);
      xs_driver_free_kotlin_project(project);
      return false;
    }
    record = next_record(record);
  }
  for(size_t i = 0; i < module_count; ++i)
  {
    size_t index = source_count + i;
    project->module_names[index] = copy_record(record);
    record = next_record(record);
    project->paths[index] = copy_record(record);
    record = next_record(record);
    if(project->module_names[index] == nullptr || project->paths[index] == nullptr)
    {
      free(data);
      xs_driver_free_kotlin_project(project);
      return false;
    }
  }
  for(size_t i = 0; i < test_count; ++i)
  {
    project->test_paths[i] = copy_record(record);
    record = next_record(record);
    if(project->test_paths[i] == nullptr)
    {
      free(data);
      xs_driver_free_kotlin_project(project);
      return false;
    }
  }
  free(data);
  return true;
}

bool xs_driver_resolve_kotlin_project(const char *module_path, XsResolvedKotlinProject *project)
{
  return resolve_kotlin_registry("sources0", ".", module_path, true, project);
}

bool xs_driver_resolve_kotlin_tests(const char *module_path, XsResolvedKotlinProject *project)
{
  return resolve_kotlin_registry("sources0", ".", module_path, false, project);
}

bool xs_driver_resolve_kotlin_modules(const char *project_root, const char *module_path,
                                      XsResolvedKotlinProject *project)
{
  if(project_root == nullptr || module_path == nullptr)
    return false;
  return resolve_kotlin_registry("modules0", project_root, module_path, false, project);
}
