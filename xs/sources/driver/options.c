/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "options.h"

#include <string.h>

const char *xs_cli_output_extension(XsBuildOutput output)
{
  switch(output)
  {
  case XS_BUILD_OUTPUT_HIR:
    return ".xhir";
  case XS_BUILD_OUTPUT_MIR:
    return ".xmir";
  case XS_BUILD_OUTPUT_XLIL:
    return ".xlil";
  case XS_BUILD_OUTPUT_NONE:
    return "";
  }
  return "";
}

static bool parse_output_kind(const char *text, XsBuildOutput *output)
{
  if(strcmp(text, "hir") == 0)
  {
    *output = XS_BUILD_OUTPUT_HIR;
    return true;
  }
  if(strcmp(text, "mir") == 0)
  {
    *output = XS_BUILD_OUTPUT_MIR;
    return true;
  }
  if(strcmp(text, "xlil") == 0)
  {
    *output = XS_BUILD_OUTPUT_XLIL;
    return true;
  }
  return false;
}

static bool set_output(XsCliOptions *options, XsBuildOutput output)
{
  if(options->output != XS_BUILD_OUTPUT_NONE)
    return false;
  options->output = output;
  return true;
}

static bool parse_output_flag(const char *flag, XsCliOptions *options)
{
  if(strcmp(flag, "--hir") == 0)
    return set_output(options, XS_BUILD_OUTPUT_HIR);
  if(strcmp(flag, "--mir") == 0)
    return set_output(options, XS_BUILD_OUTPUT_MIR);
  if(strcmp(flag, "--xlil") == 0)
    return set_output(options, XS_BUILD_OUTPUT_XLIL);
  return false;
}

bool xs_cli_parse(int argc, char **argv, XsCliOptions *options)
{
  if(argc < 4)
    return false;
  if(strcmp(argv[1], "check") != 0 && strcmp(argv[1], "build") != 0 && strcmp(argv[1], "run") != 0)
    return false;
  *options = (XsCliOptions){.command = argv[1]};
  for(int i = 2; i < argc; ++i)
  {
    if(strcmp(argv[i], "-proj") == 0)
    {
      if(++i >= argc || options->manifest_path != nullptr || options->file_path != nullptr)
        return false;
      options->manifest_path = argv[i];
    }
    else if(strcmp(argv[i], "-file") == 0)
    {
      if(++i >= argc || options->file_path != nullptr || options->manifest_path != nullptr)
        return false;
      options->file_path = argv[i];
    }
    else if(strcmp(argv[i], "--output") == 0)
    {
      if(++i >= argc || options->output != XS_BUILD_OUTPUT_NONE)
        return false;
      if(!parse_output_kind(argv[i], &options->output))
        return false;
    }
    else if(!parse_output_flag(argv[i], options))
    {
      return false;
    }
  }
  if(options->manifest_path == nullptr && options->file_path == nullptr)
    return false;
  if(options->file_path != nullptr)
    return strcmp(options->command, "build") == 0;
  return strcmp(options->command, "build") == 0 || options->output == XS_BUILD_OUTPUT_NONE;
}

void xs_cli_print_usage(FILE *stream)
{
  fprintf(stream, "usage: xs --version\n");
  fprintf(stream, "usage: xs <check|run> -proj <project.xsproj>\n");
  fprintf(stream, "usage: xs build -file <Main.xs>\n");
  fprintf(stream, "usage: xs build [--output hir|mir|xlil] -proj <project.xsproj>\n");
  fprintf(stream, "usage: xs build [--output hir|mir|xlil] -file <input>\n");
  fprintf(stream, "usage: xs build [--hir|--mir|--xlil] -file <input>\n");
}
