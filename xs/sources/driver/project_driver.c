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

bool xs_driver_resolve_kotlin_project(char ***paths, size_t *path_count)
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
  if(count == 0U)
  {
    free(data);
    fprintf(stderr, "xs: Kotlin project resolver returned no source files\n");
    return false;
  }
  char **result = calloc(count, sizeof(*result));
  if(result == nullptr)
  {
    free(data);
    return false;
  }
  size_t offset = 0;
  for(size_t i = 0; i < count; ++i)
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
  *path_count = count;
  return true;
}
