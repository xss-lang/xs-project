#include "xs/project.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef enum
{
  PROJECT_EOF,
  PROJECT_ERROR,
  PROJECT_NEWLINE,
  PROJECT_IDENTIFIER,
  PROJECT_STRING,
  PROJECT_LEFT_BRACE,
  PROJECT_RIGHT_BRACE,
  PROJECT_LEFT_BRACKET,
  PROJECT_RIGHT_BRACKET,
  PROJECT_COLON,
  PROJECT_SEMICOLON,
  PROJECT_COMMA,
} ProjectTokenKind;

typedef struct
{
  ProjectTokenKind kind;
  XsSpan span;
} ProjectToken;

typedef struct
{
  const XsSource *source;
  XsDiagnostics *diagnostics;
  XsProject *project;
  size_t cursor;
  ProjectToken current;
} ProjectParser;

static bool project_at_end(const ProjectParser *parser)
{
  return parser->cursor >= parser->source->length;
}

static char project_peek(const ProjectParser *parser, size_t ahead)
{
  size_t offset = parser->cursor + ahead;
  return offset < parser->source->length ? parser->source->text[offset] : '\0';
}

static ProjectToken project_token(ProjectTokenKind kind, size_t start, size_t end)
{
  return (ProjectToken){.kind = kind, .span = {.start = start, .end = end}};
}

static void project_error(ProjectParser *parser, XsSpan span, const char *message)
{
  xs_diagnostics_add(parser->diagnostics, XS_DIAGNOSTIC_ERROR, span, message);
}

static ProjectToken project_lex(ProjectParser *parser)
{
  while (!project_at_end(parser)) {
    char character = project_peek(parser, 0);
    if (character == ' ' || character == '\t' || character == '\v' || character == '\f') {
      ++parser->cursor;
      continue;
    }
    if (character == '/' && project_peek(parser, 1) == '/') {
      size_t start = parser->cursor;
      if (project_peek(parser, 2) == '{')
        project_error(parser, (XsSpan){start, start + 3}, "multiline comments are not allowed in .xsproj files");
      while (!project_at_end(parser) && project_peek(parser, 0) != '\n' && project_peek(parser, 0) != '\r')
        ++parser->cursor;
      continue;
    }
    break;
  }

  if (project_at_end(parser))
    return project_token(PROJECT_EOF, parser->cursor, parser->cursor);

  size_t start = parser->cursor++;
  char character = parser->source->text[start];
  if (character == '\n' || character == '\r') {
    if (character == '\r' && project_peek(parser, 0) == '\n')
      ++parser->cursor;
    return project_token(PROJECT_NEWLINE, start, parser->cursor);
  }
  if (character == '_' || isalpha((unsigned char)character) != 0) {
    while (project_peek(parser, 0) == '_' || isalnum((unsigned char)project_peek(parser, 0)) != 0)
      ++parser->cursor;
    return project_token(PROJECT_IDENTIFIER, start, parser->cursor);
  }
  if (character == '"') {
    while (!project_at_end(parser) && project_peek(parser, 0) != '"' && project_peek(parser, 0) != '\n' &&
           project_peek(parser, 0) != '\r')
      ++parser->cursor;
    if (project_peek(parser, 0) == '"') {
      ++parser->cursor;
      return project_token(PROJECT_STRING, start, parser->cursor);
    }
    project_error(parser, (XsSpan){start, parser->cursor}, "unterminated project string");
    return project_token(PROJECT_ERROR, start, parser->cursor);
  }

  switch (character) {
  case '{':
    return project_token(PROJECT_LEFT_BRACE, start, parser->cursor);
  case '}':
    return project_token(PROJECT_RIGHT_BRACE, start, parser->cursor);
  case '[':
    return project_token(PROJECT_LEFT_BRACKET, start, parser->cursor);
  case ']':
    return project_token(PROJECT_RIGHT_BRACKET, start, parser->cursor);
  case ':':
    return project_token(PROJECT_COLON, start, parser->cursor);
  case ';':
    return project_token(PROJECT_SEMICOLON, start, parser->cursor);
  case ',':
    return project_token(PROJECT_COMMA, start, parser->cursor);
  default:
    project_error(parser, (XsSpan){start, parser->cursor}, "unrecognized character in project manifest");
    return project_token(PROJECT_ERROR, start, parser->cursor);
  }
}

static void project_advance(ProjectParser *parser)
{
  parser->current = project_lex(parser);
}

static bool project_accept(ProjectParser *parser, ProjectTokenKind kind)
{
  if (parser->current.kind != kind)
    return false;
  project_advance(parser);
  return true;
}

static bool project_expect(ProjectParser *parser, ProjectTokenKind kind, const char *message)
{
  if (project_accept(parser, kind))
    return true;
  project_error(parser, parser->current.span, message);
  return false;
}

static void skip_newlines(ProjectParser *parser)
{
  while (project_accept(parser, PROJECT_NEWLINE)) {
  }
}

static bool token_is(const ProjectParser *parser, ProjectToken token, const char *text)
{
  size_t length = token.span.end - token.span.start;
  return strlen(text) == length && memcmp(parser->source->text + token.span.start, text, length) == 0;
}

static char *copy_string(ProjectParser *parser, XsSpan span)
{
  size_t start = span.start + 1;
  size_t length = span.end - span.start - 2;
  char *text = malloc(length + 1);
  if (text == NULL) {
    project_error(parser, span, "compiler ran out of memory while reading the project manifest");
    return NULL;
  }
  memcpy(text, parser->source->text + start, length);
  text[length] = '\0';
  return text;
}

static bool parse_value(ProjectParser *parser, XsProjectValue *value)
{
  if (parser->current.kind == PROJECT_STRING) {
    value->span = parser->current.span;
    value->text = copy_string(parser, parser->current.span);
    project_advance(parser);
    return value->text != NULL;
  }
  if (parser->current.kind == PROJECT_IDENTIFIER && token_is(parser, parser->current, "nil")) {
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

static void finish_field(ProjectParser *parser)
{
  if (project_accept(parser, PROJECT_SEMICOLON)) {
    if (parser->current.kind == PROJECT_SEMICOLON) {
      project_error(parser, parser->current.span, "repeated semicolons are not allowed");
      while (project_accept(parser, PROJECT_SEMICOLON)) {
      }
    }
    skip_newlines(parser);
    return;
  }
  if (project_accept(parser, PROJECT_NEWLINE)) {
    skip_newlines(parser);
    return;
  }
  if (parser->current.kind != PROJECT_RIGHT_BRACE && parser->current.kind != PROJECT_RIGHT_BRACKET &&
      parser->current.kind != PROJECT_EOF)
    project_error(parser, parser->current.span, "fields on the same line must be separated with ';'");
}

static bool append_author(ProjectParser *parser, XsProjectAuthor author)
{
  size_t count = parser->project->author_count;
  XsProjectAuthor *items = realloc(parser->project->authors, (count + 1) * sizeof(*items));
  if (items == NULL) {
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
  if (items == NULL) {
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
  if (items == NULL) {
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
  if (items == NULL) {
    project_error(parser, module.name.span, "compiler ran out of memory while reading external modules");
    return false;
  }
  parser->project->external_modules = items;
  items[count] = module;
  parser->project->external_module_count = count + 1;
  return true;
}

static void skip_unknown(ProjectParser *parser)
{
  size_t braces = 0;
  size_t brackets = 0;
  while (parser->current.kind != PROJECT_EOF) {
    if (parser->current.kind == PROJECT_NEWLINE || parser->current.kind == PROJECT_SEMICOLON) {
      if (braces == 0 && brackets == 0)
        break;
    }
    if (parser->current.kind == PROJECT_LEFT_BRACE)
      ++braces;
    else if (parser->current.kind == PROJECT_RIGHT_BRACE) {
      if (braces == 0)
        break;
      --braces;
    } else if (parser->current.kind == PROJECT_LEFT_BRACKET)
      ++brackets;
    else if (parser->current.kind == PROJECT_RIGHT_BRACKET) {
      if (brackets == 0)
        break;
      --brackets;
    }
    project_advance(parser);
    if (braces == 0 && brackets == 0 && parser->current.kind == PROJECT_RIGHT_BRACE)
      break;
  }
  finish_field(parser);
}

static void duplicate_field(ProjectParser *parser, ProjectToken name, unsigned *seen, unsigned bit)
{
  if ((*seen & bit) != 0)
    project_error(parser, name.span, "field is defined more than once");
  *seen |= bit;
}

static void parse_scalar_field(ProjectParser *parser, XsProjectValue *value)
{
  if (project_expect(parser, PROJECT_COLON, "expected ':' after field name"))
    parse_value(parser, value);
  finish_field(parser);
}

static void parse_authors(ProjectParser *parser)
{
  if (!project_expect(parser, PROJECT_LEFT_BRACE, "expected '{' after appAuthors"))
    return;
  skip_newlines(parser);
  while (parser->current.kind != PROJECT_RIGHT_BRACE && parser->current.kind != PROJECT_EOF) {
    if (!project_expect(parser, PROJECT_LEFT_BRACKET, "expected an author record")) {
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
  while (parser->current.kind != PROJECT_RIGHT_BRACKET && parser->current.kind != PROJECT_EOF) {
    XsProjectValue value = {0};
    if (parse_value(parser, &value))
      append_file(parser, value);
    skip_newlines(parser);
    if (project_accept(parser, PROJECT_COMMA)) {
      skip_newlines(parser);
      continue;
    }
    if (parser->current.kind != PROJECT_RIGHT_BRACKET) {
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
  while (parser->current.kind != PROJECT_RIGHT_BRACE && parser->current.kind != PROJECT_EOF) {
    if (parser->current.kind == PROJECT_LEFT_BRACKET) {
      if (seen_list)
        project_error(parser, parser->current.span, "the additional source file list may appear only once");
      seen_list = true;
      parse_file_list(parser);
      finish_field(parser);
      continue;
    }
    if (parser->current.kind != PROJECT_IDENTIFIER) {
      project_error(parser, parser->current.span, "expected entry or a source file list in addFiles");
      skip_unknown(parser);
      continue;
    }
    ProjectToken name = parser->current;
    project_advance(parser);
    if (token_is(parser, name, "entry")) {
      duplicate_field(parser, name, &seen, 1U);
      parse_scalar_field(parser, &parser->project->entry);
    } else {
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
  while (parser->current.kind != PROJECT_RIGHT_BRACKET && parser->current.kind != PROJECT_EOF) {
    if (parser->current.kind != PROJECT_IDENTIFIER) {
      project_error(parser, parser->current.span, "expected output target field");
      skip_unknown(parser);
      continue;
    }
    ProjectToken name = parser->current;
    project_advance(parser);
    if (token_is(parser, name, "osName")) {
      duplicate_field(parser, name, &seen, 1U);
      parse_scalar_field(parser, &target.os_name);
    } else if (token_is(parser, name, "osArch")) {
      duplicate_field(parser, name, &seen, 2U);
      parse_scalar_field(parser, &target.os_arch);
    } else {
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
  while (parser->current.kind != PROJECT_RIGHT_BRACE && parser->current.kind != PROJECT_EOF) {
    if (parser->current.kind != PROJECT_LEFT_BRACKET) {
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

static void parse_compiler_options(ProjectParser *parser)
{
  if (!project_expect(parser, PROJECT_LEFT_BRACE, "expected '{' after compilerOptions"))
    return;
  unsigned seen = 0;
  skip_newlines(parser);
  while (parser->current.kind != PROJECT_RIGHT_BRACE && parser->current.kind != PROJECT_EOF) {
    if (parser->current.kind != PROJECT_IDENTIFIER) {
      project_error(parser, parser->current.span, "expected compiler option field");
      skip_unknown(parser);
      continue;
    }
    ProjectToken name = parser->current;
    project_advance(parser);
    if (token_is(parser, name, "xsVersion")) {
      duplicate_field(parser, name, &seen, 1U);
      parse_scalar_field(parser, &parser->project->xs_version);
    } else if (token_is(parser, name, "addFiles")) {
      duplicate_field(parser, name, &seen, 2U);
      parse_add_files(parser);
      finish_field(parser);
    } else if (token_is(parser, name, "output")) {
      duplicate_field(parser, name, &seen, 4U);
      parse_output(parser);
      finish_field(parser);
    } else {
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
}

static XsProjectModule parse_external_module(ProjectParser *parser)
{
  XsProjectModule module = {0};
  unsigned seen = 0;
  project_expect(parser, PROJECT_LEFT_BRACE, "expected '{' after addModule");
  skip_newlines(parser);
  while (parser->current.kind != PROJECT_RIGHT_BRACE && parser->current.kind != PROJECT_EOF) {
    if (parser->current.kind != PROJECT_IDENTIFIER) {
      project_error(parser, parser->current.span, "expected addModule field");
      skip_unknown(parser);
      continue;
    }
    ProjectToken name = parser->current;
    project_advance(parser);
    if (token_is(parser, name, "moduleName")) {
      duplicate_field(parser, name, &seen, 1U);
      parse_scalar_field(parser, &module.name);
    } else if (token_is(parser, name, "moduleVersion")) {
      duplicate_field(parser, name, &seen, 2U);
      parse_scalar_field(parser, &module.version);
    } else {
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

static void parse_external_modules(ProjectParser *parser)
{
  if (!project_expect(parser, PROJECT_LEFT_BRACE, "expected '{' after externalModules"))
    return;
  skip_newlines(parser);
  while (parser->current.kind != PROJECT_RIGHT_BRACE && parser->current.kind != PROJECT_EOF) {
    if (parser->current.kind != PROJECT_IDENTIFIER || !token_is(parser, parser->current, "addModule")) {
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

void xs_project_init(XsProject *project)
{
  *project = (XsProject){0};
}

static void free_value(XsProjectValue *value)
{
  free(value->text);
  *value = (XsProjectValue){0};
}

void xs_project_free(XsProject *project)
{
  free_value(&project->app_name);
  free_value(&project->app_version);
  free_value(&project->app_release);
  free_value(&project->app_license);
  for (size_t i = 0; i < project->author_count; ++i) {
    free_value(&project->authors[i].name);
    free_value(&project->authors[i].email);
  }
  free(project->authors);
  free_value(&project->xs_version);
  free_value(&project->entry);
  for (size_t i = 0; i < project->additional_file_count; ++i)
    free_value(&project->additional_files[i]);
  free(project->additional_files);
  for (size_t i = 0; i < project->target_count; ++i) {
    free_value(&project->targets[i].os_name);
    free_value(&project->targets[i].os_arch);
  }
  free(project->targets);
  for (size_t i = 0; i < project->external_module_count; ++i) {
    free_value(&project->external_modules[i].name);
    free_value(&project->external_modules[i].version);
  }
  free(project->external_modules);
  *project = (XsProject){0};
}

bool xs_project_parse(const XsSource *source, XsDiagnostics *diagnostics, XsProject *project)
{
  ProjectParser parser = {.source = source, .diagnostics = diagnostics, .project = project};
  project_advance(&parser);
  unsigned seen = 0;
  skip_newlines(&parser);
  while (parser.current.kind != PROJECT_EOF) {
    if (parser.current.kind != PROJECT_IDENTIFIER) {
      project_error(&parser, parser.current.span, "expected a top-level project field");
      skip_unknown(&parser);
      continue;
    }
    ProjectToken name = parser.current;
    project_advance(&parser);
    if (token_is(&parser, name, "appName")) {
      duplicate_field(&parser, name, &seen, 1U);
      parse_scalar_field(&parser, &project->app_name);
    } else if (token_is(&parser, name, "appVersion")) {
      duplicate_field(&parser, name, &seen, 2U);
      parse_scalar_field(&parser, &project->app_version);
    } else if (token_is(&parser, name, "appRelease")) {
      duplicate_field(&parser, name, &seen, 4U);
      parse_scalar_field(&parser, &project->app_release);
    } else if (token_is(&parser, name, "appLicense")) {
      duplicate_field(&parser, name, &seen, 8U);
      parse_scalar_field(&parser, &project->app_license);
    } else if (token_is(&parser, name, "appAuthors")) {
      duplicate_field(&parser, name, &seen, 16U);
      parse_authors(&parser);
      finish_field(&parser);
    } else if (token_is(&parser, name, "compilerOptions")) {
      duplicate_field(&parser, name, &seen, 32U);
      parse_compiler_options(&parser);
      finish_field(&parser);
    } else if (token_is(&parser, name, "externalModules")) {
      duplicate_field(&parser, name, &seen, 64U);
      parse_external_modules(&parser);
      finish_field(&parser);
    } else {
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

  if (!project->app_release.is_nil && project->app_release.text != NULL &&
      strcmp(project->app_release.text, "ALPHA") != 0 && strcmp(project->app_release.text, "BETA") != 0 &&
      strcmp(project->app_release.text, "STABLE") != 0)
    project_error(&parser, project->app_release.span, "appRelease must be ALPHA, BETA, STABLE, or nil");

  return !xs_diagnostics_has_error(diagnostics);
}

const XsProjectValue *xs_project_selected_entry(const XsProject *project)
{
  if (!project->entry.is_nil && project->entry.text != NULL)
    return &project->entry;
  if (project->additional_file_count != 0)
    return &project->additional_files[0];
  return NULL;
}
