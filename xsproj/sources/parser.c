/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xs/project.h"

#include "parser_internal.h"

#include <stdlib.h>
#include <string.h>

static bool parse_value(ProjectParser *parser, XsProjectValue *value)
{
  if (parser->current.kind == PROJECT_STRING)
  {
    value->span = parser->current.span;
    value->text = copy_string(parser, parser->current.span);
    project_advance(parser);
    return value->text != nullptr;
  }
  if (parser->current.kind == PROJECT_IDENTIFIER && token_is(parser, parser->current, "nil"))
  {
    value->span = parser->current.span;
    value->is_nil = true;
    project_advance(parser);
    return true;
  }
  project_error(parser, parser->current.span, "expected a string or nil value");
  if (parser->current.kind != PROJECT_EOF)
    project_advance(parser);
  return false;
}

void finish_field(ProjectParser *parser)
{
  if (project_accept(parser, PROJECT_SEMICOLON))
  {
    if (parser->current.kind == PROJECT_SEMICOLON)
    {
      project_error(parser, parser->current.span, "repeated semicolons are not allowed");
      while (project_accept(parser, PROJECT_SEMICOLON))
      {
      }
    }
    skip_newlines(parser);
    return;
  }
  if (project_accept(parser, PROJECT_NEWLINE))
  {
    skip_newlines(parser);
    return;
  }
  if (parser->current.kind != PROJECT_RIGHT_BRACE && parser->current.kind != PROJECT_RIGHT_BRACKET &&
      parser->current.kind != PROJECT_EOF)
  {
    project_error(parser, parser->current.span, "fields on the same line must be separated with ';'");
    project_advance(parser);
  }
}

static bool append_author(ProjectParser *parser, XsProjectAuthor author)
{
  size_t count = parser->project->author_count;
  XsProjectAuthor *items = realloc(parser->project->authors, (count + 1) * sizeof(*items));
  if (items == nullptr)
  {
    project_error(parser, author.name.span, "compiler ran out of memory while reading authors");
    return false;
  }
  parser->project->authors = items;
  items[count] = author;
  parser->project->author_count = count + 1;
  return true;
}

static bool append_file(ProjectParser *parser, XsProjectValue value)
{
  size_t count = parser->project->additional_file_count;
  XsProjectValue *items = realloc(parser->project->additional_files, (count + 1) * sizeof(*items));
  if (items == nullptr)
  {
    project_error(parser, value.span, "compiler ran out of memory while reading source files");
    return false;
  }
  parser->project->additional_files = items;
  items[count] = value;
  parser->project->additional_file_count = count + 1;
  return true;
}

static bool append_target(ProjectParser *parser, XsProjectTarget target)
{
  size_t count = parser->project->target_count;
  XsProjectTarget *items = realloc(parser->project->targets, (count + 1) * sizeof(*items));
  if (items == nullptr)
  {
    project_error(parser, target.os_name.span, "compiler ran out of memory while reading output targets");
    return false;
  }
  parser->project->targets = items;
  items[count] = target;
  parser->project->target_count = count + 1;
  return true;
}

static bool append_module(ProjectParser *parser, XsProjectModule module)
{
  size_t count = parser->project->external_module_count;
  XsProjectModule *items = realloc(parser->project->external_modules, (count + 1) * sizeof(*items));
  if (items == nullptr)
  {
    project_error(parser, module.name.span, "compiler ran out of memory while reading external modules");
    return false;
  }
  parser->project->external_modules = items;
  items[count] = module;
  parser->project->external_module_count = count + 1;
  return true;
}

void skip_unknown(ProjectParser *parser)
{
  if (parser->current.kind == PROJECT_RIGHT_BRACE || parser->current.kind == PROJECT_RIGHT_BRACKET)
  {
    project_advance(parser);
    return;
  }
  size_t braces = 0;
  size_t brackets = 0;
  size_t start = parser->current.span.start;
  while (parser->current.kind != PROJECT_EOF)
  {
    if (parser->current.kind == PROJECT_NEWLINE || parser->current.kind == PROJECT_SEMICOLON)
    {
      if (braces == 0 && brackets == 0)
        break;
    }
    if (parser->current.kind == PROJECT_LEFT_BRACE)
      ++braces;
    else if (parser->current.kind == PROJECT_RIGHT_BRACE)
    {
      if (braces == 0)
        break;
      --braces;
    }
    else if (parser->current.kind == PROJECT_LEFT_BRACKET)
      ++brackets;
    else if (parser->current.kind == PROJECT_RIGHT_BRACKET)
    {
      if (brackets == 0)
        break;
      --brackets;
    }
    project_advance(parser);
    if (braces == 0 && brackets == 0 && parser->current.kind == PROJECT_RIGHT_BRACE)
      break;
  }
  if (parser->current.kind != PROJECT_EOF && parser->current.span.start == start)
    project_advance(parser);
  finish_field(parser);
}

void duplicate_field(ProjectParser *parser, ProjectToken name, unsigned *seen, unsigned bit)
{
  if ((*seen & bit) != 0)
    project_error(parser, name.span, "field is defined more than once");
  *seen |= bit;
}

void parse_scalar_field(ProjectParser *parser, XsProjectValue *value)
{
  if (project_expect(parser, PROJECT_COLON, "expected ':' after field name"))
    parse_value(parser, value);
  finish_field(parser);
}

static void validate_xs_backend(ProjectParser *parser)
{
  const XsProjectValue *backend = &parser->project->xs_backend;
  if (backend->is_nil || backend->text == nullptr ||
      (strcmp(backend->text, "LLVM") != 0 && strcmp(backend->text, "XS") != 0))
    project_error(parser, backend->span, "compilerOptions.xsBackend must be LLVM or XS");
}

void parse_authors(ProjectParser *parser)
{
  if (!project_expect(parser, PROJECT_LEFT_BRACE, "expected '{' after appAuthors"))
    return;
  skip_newlines(parser);
  while (parser->current.kind != PROJECT_RIGHT_BRACE && parser->current.kind != PROJECT_EOF)
  {
    if (!project_expect(parser, PROJECT_LEFT_BRACKET, "expected an author record"))
    {
      skip_unknown(parser);
      continue;
    }
    XsProjectAuthor author = {0};
    parse_value(parser, &author.name);
    project_expect(parser, PROJECT_COMMA, "expected ',' between author name and email");
    parse_value(parser, &author.email);
    if (project_accept(parser, PROJECT_COMMA))
      project_error(parser, parser->current.span, "an author record must contain exactly two values");
    project_expect(parser, PROJECT_RIGHT_BRACKET, "an author record must contain exactly two values");
    append_author(parser, author);
    finish_field(parser);
  }
  project_expect(parser, PROJECT_RIGHT_BRACE, "expected '}' after appAuthors");
  if (parser->project->author_count == 0)
    project_error(parser, parser->current.span, "appAuthors must contain at least one author record");
}

static void parse_file_list(ProjectParser *parser)
{
  project_advance(parser);
  skip_newlines(parser);
  while (parser->current.kind != PROJECT_RIGHT_BRACKET && parser->current.kind != PROJECT_EOF)
  {
    XsProjectValue value = {0};
    if (parse_value(parser, &value))
      append_file(parser, value);
    skip_newlines(parser);
    if (project_accept(parser, PROJECT_COMMA))
    {
      skip_newlines(parser);
      continue;
    }
    if (parser->current.kind != PROJECT_RIGHT_BRACKET)
    {
      project_error(parser, parser->current.span, "expected ',' between source file values");
      if (parser->current.kind != PROJECT_EOF)
        project_advance(parser);
    }
  }
  project_expect(parser, PROJECT_RIGHT_BRACKET, "expected ']' after source file list");
}

static void parse_add_files(ProjectParser *parser)
{
  if (!project_expect(parser, PROJECT_LEFT_BRACE, "expected '{' after addFiles"))
    return;
  unsigned seen = 0;
  bool seen_list = false;
  skip_newlines(parser);
  while (parser->current.kind != PROJECT_RIGHT_BRACE && parser->current.kind != PROJECT_EOF)
  {
    if (parser->current.kind == PROJECT_LEFT_BRACKET)
    {
      if (seen_list)
        project_error(parser, parser->current.span, "the additional source file list may appear only once");
      seen_list = true;
      parse_file_list(parser);
      finish_field(parser);
      continue;
    }
    if (parser->current.kind != PROJECT_IDENTIFIER)
    {
      project_error(parser, parser->current.span, "expected entry or a source file list in addFiles");
      skip_unknown(parser);
      continue;
    }
    ProjectToken name = parser->current;
    project_advance(parser);
    if (token_is(parser, name, "entry"))
    {
      duplicate_field(parser, name, &seen, 1U);
      parse_scalar_field(parser, &parser->project->entry);
    }
    else
    {
      project_error(parser, name.span, "unknown field in addFiles");
      skip_unknown(parser);
    }
  }
  project_expect(parser, PROJECT_RIGHT_BRACE, "expected '}' after addFiles");
  if ((seen & 1U) == 0)
    project_error(parser, parser->current.span, "required field addFiles.entry is missing");
}

static XsProjectTarget parse_target_record(ProjectParser *parser)
{
  XsProjectTarget target = {0};
  unsigned seen = 0;
  project_expect(parser, PROJECT_LEFT_BRACKET, "expected output target record");
  skip_newlines(parser);
  while (parser->current.kind != PROJECT_RIGHT_BRACKET && parser->current.kind != PROJECT_EOF)
  {
    if (parser->current.kind != PROJECT_IDENTIFIER)
    {
      project_error(parser, parser->current.span, "expected output target field");
      skip_unknown(parser);
      continue;
    }
    ProjectToken name = parser->current;
    project_advance(parser);
    if (token_is(parser, name, "osName"))
    {
      duplicate_field(parser, name, &seen, 1U);
      parse_scalar_field(parser, &target.os_name);
    }
    else if (token_is(parser, name, "osArch"))
    {
      duplicate_field(parser, name, &seen, 2U);
      parse_scalar_field(parser, &target.os_arch);
    }
    else
    {
      project_error(parser, name.span, "unknown output target field");
      skip_unknown(parser);
    }
  }
  project_expect(parser, PROJECT_RIGHT_BRACKET, "expected ']' after output target");
  if ((seen & 1U) == 0)
    project_error(parser, parser->current.span, "required output field osName is missing");
  if ((seen & 2U) == 0)
    project_error(parser, parser->current.span, "required output field osArch is missing");
  return target;
}

static void parse_output(ProjectParser *parser)
{
  if (!project_expect(parser, PROJECT_LEFT_BRACE, "expected '{' after output"))
    return;
  skip_newlines(parser);
  while (parser->current.kind != PROJECT_RIGHT_BRACE && parser->current.kind != PROJECT_EOF)
  {
    if (parser->current.kind != PROJECT_LEFT_BRACKET)
    {
      project_error(parser, parser->current.span, "expected an output target record");
      skip_unknown(parser);
      continue;
    }
    append_target(parser, parse_target_record(parser));
    finish_field(parser);
  }
  project_expect(parser, PROJECT_RIGHT_BRACE, "expected '}' after output");
  if (parser->project->target_count == 0)
    project_error(parser, parser->current.span, "output must contain at least one target");
}

void parse_compiler_options(ProjectParser *parser)
{
  if (!project_expect(parser, PROJECT_LEFT_BRACE, "expected '{' after compilerOptions"))
    return;
  unsigned seen = 0;
  skip_newlines(parser);
  while (parser->current.kind != PROJECT_RIGHT_BRACE && parser->current.kind != PROJECT_EOF)
  {
    if (parser->current.kind != PROJECT_IDENTIFIER)
    {
      project_error(parser, parser->current.span, "expected compiler option field");
      skip_unknown(parser);
      continue;
    }
    ProjectToken name = parser->current;
    project_advance(parser);
    if (token_is(parser, name, "xsVersion"))
    {
      duplicate_field(parser, name, &seen, 1U);
      parse_scalar_field(parser, &parser->project->xs_version);
    }
    else if (token_is(parser, name, "addFiles"))
    {
      duplicate_field(parser, name, &seen, 2U);
      parse_add_files(parser);
      finish_field(parser);
    }
    else if (token_is(parser, name, "output"))
    {
      duplicate_field(parser, name, &seen, 4U);
      parse_output(parser);
      finish_field(parser);
    }
    else if (token_is(parser, name, "xsBackend"))
    {
      duplicate_field(parser, name, &seen, 8U);
      parse_scalar_field(parser, &parser->project->xs_backend);
    }
    else
    {
      project_error(parser, name.span, "unknown compilerOptions field");
      skip_unknown(parser);
    }
  }
  project_expect(parser, PROJECT_RIGHT_BRACE, "expected '}' after compilerOptions");
  if ((seen & 1U) == 0)
    project_error(parser, parser->current.span, "required compilerOptions field xsVersion is missing");
  if ((seen & 2U) == 0)
    project_error(parser, parser->current.span, "required compilerOptions field addFiles is missing");
  if ((seen & 4U) == 0)
    project_error(parser, parser->current.span, "required compilerOptions field output is missing");
  if ((seen & 8U) != 0)
    validate_xs_backend(parser);
}

static XsProjectModule parse_external_module(ProjectParser *parser)
{
  XsProjectModule module = {0};
  unsigned seen = 0;
  if (!project_expect(parser, PROJECT_LEFT_BRACE, "expected '{' after addModule"))
    return module;
  skip_newlines(parser);
  while (parser->current.kind != PROJECT_RIGHT_BRACE && parser->current.kind != PROJECT_EOF)
  {
    if (parser->current.kind != PROJECT_IDENTIFIER)
    {
      project_error(parser, parser->current.span, "expected addModule field");
      skip_unknown(parser);
      continue;
    }
    ProjectToken name = parser->current;
    project_advance(parser);
    if (token_is(parser, name, "moduleName"))
    {
      duplicate_field(parser, name, &seen, 1U);
      parse_scalar_field(parser, &module.name);
    }
    else if (token_is(parser, name, "moduleVersion"))
    {
      duplicate_field(parser, name, &seen, 2U);
      parse_scalar_field(parser, &module.version);
    }
    else
    {
      project_error(parser, name.span, "unknown addModule field");
      skip_unknown(parser);
    }
  }
  project_expect(parser, PROJECT_RIGHT_BRACE, "expected '}' after addModule");
  if ((seen & 1U) == 0)
    project_error(parser, parser->current.span, "required addModule field moduleName is missing");
  if ((seen & 2U) == 0)
    project_error(parser, parser->current.span, "required addModule field moduleVersion is missing");
  return module;
}

void parse_external_modules(ProjectParser *parser)
{
  if (!project_expect(parser, PROJECT_LEFT_BRACE, "expected '{' after externalModules"))
    return;
  skip_newlines(parser);
  while (parser->current.kind != PROJECT_RIGHT_BRACE && parser->current.kind != PROJECT_EOF)
  {
    if (parser->current.kind != PROJECT_IDENTIFIER || !token_is(parser, parser->current, "addModule"))
    {
      project_error(parser, parser->current.span, "externalModules may contain only addModule blocks");
      skip_unknown(parser);
      continue;
    }
    project_advance(parser);
    append_module(parser, parse_external_module(parser));
    finish_field(parser);
  }
  project_expect(parser, PROJECT_RIGHT_BRACE, "expected '}' after externalModules");
}
