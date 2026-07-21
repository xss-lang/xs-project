/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: MPL-2.0
 */

#include "xs/diagnostic.h"
#include "xs/project.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef XS_PROJECT_VERSION
#define XS_PROJECT_VERSION "0.2.1"
#endif

static char *read_file(const char *path, size_t *length)
{
  FILE *file = fopen(path, "rb");
  if(file == nullptr || fseek(file, 0, SEEK_END) != 0)
    goto failure;
  long size = ftell(file);
  if(size < 0 || fseek(file, 0, SEEK_SET) != 0)
    goto failure;
  char *text = malloc((size_t)size + 1U);
  if(text == nullptr || fread(text, 1, (size_t)size, file) != (size_t)size)
  {
    free(text);
    goto failure;
  }
  text[size] = '\0';
  *length = (size_t)size;
  fclose(file);
  return text;

failure:
  if(file != nullptr)
    fclose(file);
  return nullptr;
}

int main(int argc, char **argv)
{
  if(argc == 2 && strcmp(argv[1], "--version") == 0)
  {
    printf("xs-proj %s\n", XS_PROJECT_VERSION);
    return 0;
  }
  if(argc != 2)
  {
    fprintf(stderr, "usage: xs-proj <project.xsproj>\n");
    return 2;
  }
  size_t length = 0;
  char *text = read_file(argv[1], &length);
  if(text == nullptr)
  {
    fprintf(stderr, "xs-proj: project file '%s' could not be read\n", argv[1]);
    return 2;
  }
  XsSource source = {.path = argv[1], .text = text, .length = length};
  XsDiagnostics diagnostics;
  XsProject project;
  xs_diagnostics_init(&diagnostics);
  xs_project_init(&project);
  bool success = xs_project_parse(&source, &diagnostics, &project);
  xs_diagnostics_print(&diagnostics, &source, stderr);
  xs_project_free(&project);
  xs_diagnostics_free(&diagnostics);
  free(text);
  return success ? 0 : 1;
}
