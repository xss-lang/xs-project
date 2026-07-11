/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parser_internal.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

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

void project_error(ProjectParser *parser, XsSpan span, const char *message)
{
  xs_diagnostics_add(parser->diagnostics, XS_DIAGNOSTIC_ERROR, span, message);
}

static ProjectToken project_lex(ProjectParser *parser)
{
  while (!project_at_end(parser))
  {
    char character = project_peek(parser, 0);
    if (character == ' ' || character == '\t' || character == '\v' || character == '\f')
    {
      ++parser->cursor;
      continue;
    }
    if (character == '/' && project_peek(parser, 1) == '/')
    {
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
  if (character == '\n' || character == '\r')
  {
    if (character == '\r' && project_peek(parser, 0) == '\n')
      ++parser->cursor;
    return project_token(PROJECT_NEWLINE, start, parser->cursor);
  }
  if (character == '_' || isalpha((unsigned char)character) != 0)
  {
    while (project_peek(parser, 0) == '_' || isalnum((unsigned char)project_peek(parser, 0)) != 0)
      ++parser->cursor;
    return project_token(PROJECT_IDENTIFIER, start, parser->cursor);
  }
  if (character == '"')
  {
    while (!project_at_end(parser) && project_peek(parser, 0) != '"' && project_peek(parser, 0) != '\n' &&
           project_peek(parser, 0) != '\r')
      ++parser->cursor;
    if (project_peek(parser, 0) == '"')
    {
      ++parser->cursor;
      return project_token(PROJECT_STRING, start, parser->cursor);
    }
    project_error(parser, (XsSpan){start, parser->cursor}, "unterminated project string");
    return project_token(PROJECT_ERROR, start, parser->cursor);
  }

  switch (character)
  {
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

void project_advance(ProjectParser *parser)
{
  parser->current = project_lex(parser);
}

bool project_accept(ProjectParser *parser, ProjectTokenKind kind)
{
  if (parser->current.kind != kind)
    return false;
  project_advance(parser);
  return true;
}

bool project_expect(ProjectParser *parser, ProjectTokenKind kind, const char *message)
{
  if (project_accept(parser, kind))
    return true;
  project_error(parser, parser->current.span, message);
  return false;
}

void skip_newlines(ProjectParser *parser)
{
  while (project_accept(parser, PROJECT_NEWLINE))
  {
  }
}

bool token_is(const ProjectParser *parser, ProjectToken token, const char *text)
{
  size_t length = token.span.end - token.span.start;
  return strlen(text) == length && memcmp(parser->source->text + token.span.start, text, length) == 0;
}

char *copy_string(ProjectParser *parser, XsSpan span)
{
  size_t start = span.start + 1;
  size_t length = span.end - span.start - 2;
  char *text = malloc(length + 1);
  if (text == nullptr)
  {
    project_error(parser, span, "compiler ran out of memory while reading the project manifest");
    return nullptr;
  }
  memcpy(text, parser->source->text + start, length);
  text[length] = '\0';
  return text;
}
