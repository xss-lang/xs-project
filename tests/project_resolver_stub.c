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
  const char *records[] = {"xs-project-sources-v1", "all", "true", "true", source};
  bool success = true;
  for(size_t i = 0; i < sizeof(records) / sizeof(records[0]); ++i)
  {
    size_t length = strlen(records[i]) + 1U;
    success = fwrite(records[i], 1, length, registry) == length && success;
  }
  success = fclose(registry) == 0 && success;
  return success ? 0 : 1;
}
