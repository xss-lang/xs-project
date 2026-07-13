/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
  const char *source = getenv("XS_TEST_PROJECT_SOURCE");
  if(argc != 4 || strcmp(argv[1], "sources0") != 0 || source == nullptr)
    return 2;
  FILE *registry = fopen(argv[3], "wb");
  if(registry == nullptr)
    return 2;
  size_t length = strlen(source) + 1U;
  bool success = fwrite(source, 1, length, registry) == length;
  success = fclose(registry) == 0 && success;
  return success ? 0 : 1;
}
