/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <spawn.h>
#include <sys/wait.h>

extern char **environ;

static int failures;

#define CHECK(condition)                                                                                               \
  do                                                                                                                   \
  {                                                                                                                    \
    if(!(condition))                                                                                                   \
    {                                                                                                                  \
      fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition);                                    \
      ++failures;                                                                                                      \
    }                                                                                                                  \
  } while(0)

static bool has_magic(const char *path, const uint8_t *magic, size_t length)
{
  FILE *file = fopen(path, "rb");
  if(file == nullptr)
    return false;
  uint8_t buffer[8] = {0};
  size_t read = fread(buffer, 1, length, file);
  fclose(file);
  if(read != length)
    return false;
  for(size_t i = 0; i < length; ++i)
    if(buffer[i] != magic[i])
      return false;
  return true;
}

static bool exists(const char *path)
{
  FILE *file = fopen(path, "rb");
  if(file == nullptr)
    return false;
  fclose(file);
  return true;
}

static bool parse_exit_code(const char *text, int *code)
{
  char *end = nullptr;
  long value = strtol(text, &end, 10);
  if(end == text || *end != '\0' || value < 0 || value > 255)
    return false;
  *code = (int)value;
  return true;
}

static bool run_and_check_exit(const char *path, int expected)
{
  char *const arguments[] = {(char *)path, nullptr};
  pid_t process = 0;
  int spawn_status = posix_spawnp(&process, path, nullptr, nullptr, arguments, environ);
  if(spawn_status != 0)
  {
    fprintf(stderr, "could not run %s: %s\n", path, strerror(spawn_status));
    return false;
  }
  int status = 0;
  if(waitpid(process, &status, 0) < 0)
  {
    fprintf(stderr, "could not wait for %s: %s\n", path, strerror(errno));
    return false;
  }
  if(!WIFEXITED(status))
    return false;
  int actual = WEXITSTATUS(status);
  if(actual != expected)
  {
    fprintf(stderr, "%s exited with %d, expected %d\n", path, actual, expected);
    return false;
  }
  return true;
}

int main(int argc, char **argv)
{
  if(argc != 5)
  {
    fprintf(stderr, "xse artifact test requires .ll, .o, .xse, and expected exit-code arguments\n");
    return 2;
  }
  int expected_exit = 0;
  if(!parse_exit_code(argv[4], &expected_exit))
  {
    fprintf(stderr, "invalid expected exit code: %s\n", argv[4]);
    return 2;
  }

  static const uint8_t elf_magic[] = {0x7f, 'E', 'L', 'F'};
  CHECK(exists(argv[1]));
  CHECK(has_magic(argv[2], elf_magic, sizeof(elf_magic)));
  CHECK(has_magic(argv[3], elf_magic, sizeof(elf_magic)));
  CHECK(run_and_check_exit(argv[3], expected_exit));
  return failures == 0 ? 0 : 1;
}
