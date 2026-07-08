/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xs/project.h"

#include "parser_internal.h"

#include <string.h>

bool xs_project_parse(const XsSource *source, XsDiagnostics *diagnostics, XsProject *project)
{
  ProjectParser parser = {.source = source, .diagnostics = diagnostics, .project = project};
  project_advance(&parser);
  unsigned seen = 0;
  skip_newlines(&parser);
  while (parser.current.kind != PROJECT_EOF)
  {
    if (parser.current.kind != PROJECT_IDENTIFIER)
    {
      project_error(&parser, parser.current.span, "expected a top-level project field");
      skip_unknown(&parser);
      continue;
    }
    ProjectToken name = parser.current;
    project_advance(&parser);
    if (token_is(&parser, name, "appName"))
    {
      duplicate_field(&parser, name, &seen, 1U);
      parse_scalar_field(&parser, &project->app_name);
    }
    else if (token_is(&parser, name, "appVersion"))
    {
      duplicate_field(&parser, name, &seen, 2U);
      parse_scalar_field(&parser, &project->app_version);
    }
    else if (token_is(&parser, name, "appRelease"))
    {
      duplicate_field(&parser, name, &seen, 4U);
      parse_scalar_field(&parser, &project->app_release);
    }
    else if (token_is(&parser, name, "appLicense"))
    {
      duplicate_field(&parser, name, &seen, 8U);
      parse_scalar_field(&parser, &project->app_license);
    }
    else if (token_is(&parser, name, "appAuthors"))
    {
      duplicate_field(&parser, name, &seen, 16U);
      parse_authors(&parser);
      finish_field(&parser);
    }
    else if (token_is(&parser, name, "compilerOptions"))
    {
      duplicate_field(&parser, name, &seen, 32U);
      parse_compiler_options(&parser);
      finish_field(&parser);
    }
    else if (token_is(&parser, name, "externalModules"))
    {
      duplicate_field(&parser, name, &seen, 64U);
      parse_external_modules(&parser);
      finish_field(&parser);
    }
    else
    {
      project_error(&parser, name.span, "unknown top-level project field");
      skip_unknown(&parser);
    }
  }

  if ((seen & 1U) == 0)
    project_error(&parser, parser.current.span, "required field appName is missing");
  if ((seen & 2U) == 0)
    project_error(&parser, parser.current.span, "required field appVersion is missing");
  if ((seen & 4U) == 0)
    project_error(&parser, parser.current.span, "required field appRelease is missing");
  if ((seen & 8U) == 0)
    project_error(&parser, parser.current.span, "required field appLicense is missing");
  if ((seen & 16U) == 0)
    project_error(&parser, parser.current.span, "required field appAuthors is missing");
  if ((seen & 32U) == 0)
    project_error(&parser, parser.current.span, "required field compilerOptions is missing");

  if (!project->app_release.is_nil && project->app_release.text != nullptr &&
      strcmp(project->app_release.text, "ALPHA") != 0 && strcmp(project->app_release.text, "BETA") != 0 &&
      strcmp(project->app_release.text, "STABLE") != 0)
    project_error(&parser, project->app_release.span, "appRelease must be ALPHA, BETA, STABLE, or None");

  return !xs_diagnostics_has_error(diagnostics);
}
