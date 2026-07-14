/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "project_driver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

static const char *const REGISTRY_VERSION = "xs-project-sources-v1";

static bool run_resolver(const char *output_path)
{
  const char *program = getenv("XS_PROJECT_DRIVER");
  if(program == nullptr || program[0] == '\0')
    program = "xs-project";
  char *arguments[] = {(char *)program, "sources0", ".", (char *)output_path, nullptr};
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

void xs_driver_free_project_paths(char **paths, size_t path_count)
{
  for(size_t i = 0; i < path_count; ++i)
    free(paths[i]);
  free(paths);
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

static bool parse_settings(char *data, size_t record_count, size_t *source_offset, XsCompilerSettings *settings)
{
  *settings = xs_cli_default_compiler_settings();
  *source_offset = 0;
  if(strcmp(data, REGISTRY_VERSION) != 0)
    return true;
  if(record_count < 5U)
    return false;
  char *warning = data + strlen(data) + 1U;
  char *werror = warning + strlen(warning) + 1U;
  char *verbose = werror + strlen(werror) + 1U;
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
     !parse_bool_record(verbose, &settings->verbose))
    return false;
  *source_offset = 4U;
  return true;
}

bool xs_driver_resolve_kotlin_project(char ***paths, size_t *path_count, XsCompilerSettings *settings)
{
  char registry_path[] = "/tmp/xs-project-sources-XXXXXX";
  int registry = mkstemp(registry_path);
  if(registry < 0)
  {
    fprintf(stderr, "xs: could not create the project source registry\n");
    return false;
  }
  close(registry);
  bool success = run_resolver(registry_path);
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
  size_t source_offset = 0;
  if(count == 0U || !parse_settings(data, count, &source_offset, settings) || source_offset >= count)
  {
    free(data);
    fprintf(stderr, "xs: Kotlin project resolver returned an invalid compiler/source registry\n");
    return false;
  }
  size_t source_count = count - source_offset;
  char **result = calloc(source_count, sizeof(*result));
  if(result == nullptr)
  {
    free(data);
    return false;
  }
  size_t offset = 0;
  for(size_t i = 0; i < source_offset; ++i)
    offset += strlen(data + offset) + 1U;
  for(size_t i = 0; i < source_count; ++i)
  {
    size_t item_length = strlen(data + offset);
    result[i] = malloc(item_length + 1U);
    if(result[i] == nullptr)
    {
      xs_driver_free_project_paths(result, i);
      free(data);
      return false;
    }
    memcpy(result[i], data + offset, item_length + 1U);
    offset += item_length + 1U;
  }
  free(data);
  *paths = result;
  *path_count = source_count;
  return true;
}
