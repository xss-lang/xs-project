/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdio.h>

static int failures;

#define CHECK(condition)                                                                                               \
  do                                                                                                                   \
  {                                                                                                                    \
    if (!(condition))                                                                                                  \
    {                                                                                                                  \
      fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition);                                    \
      ++failures;                                                                                                      \
    }                                                                                                                  \
  } while (0)

static bool has_magic(const char *path, const uint8_t *magic, size_t length)
{
  FILE *file = fopen(path, "rb");
  if (file == nullptr)
    return false;
  uint8_t buffer[8] = {0};
  size_t read = fread(buffer, 1, length, file);
  fclose(file);
  if (read != length)
    return false;
  for (size_t i = 0; i < length; ++i)
    if (buffer[i] != magic[i])
      return false;
  return true;
}

static bool exists(const char *path)
{
  FILE *file = fopen(path, "rb");
  if (file == nullptr)
    return false;
  fclose(file);
  return true;
}

int main(int argc, char **argv)
{
  if (argc != 4)
  {
    fprintf(stderr, "xse artifact test requires .ll, .o, and .xse paths\n");
    return 2;
  }

  static const uint8_t elf_magic[] = {0x7f, 'E', 'L', 'F'};
  CHECK(exists(argv[1]));
  CHECK(has_magic(argv[2], elf_magic, sizeof(elf_magic)));
  CHECK(has_magic(argv[3], elf_magic, sizeof(elf_magic)));
  return failures == 0 ? 0 : 1;
}
