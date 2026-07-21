/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef XS_PROJECT_PARSER_INTERNAL_H
#define XS_PROJECT_PARSER_INTERNAL_H

#include "xs/project.h"

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

void project_error(ProjectParser *parser, XsSpan span, const char *message);
void project_advance(ProjectParser *parser);
bool project_accept(ProjectParser *parser, ProjectTokenKind kind);
bool project_expect(ProjectParser *parser, ProjectTokenKind kind, const char *message);
void project_skip_newlines(ProjectParser *parser);
bool project_token_is(const ProjectParser *parser, ProjectToken token, const char *text);
char *project_copy_string(ProjectParser *parser, XsSpan span);

void finish_field(ProjectParser *parser);
void skip_unknown(ProjectParser *parser);
void duplicate_field(ProjectParser *parser, ProjectToken name, unsigned *seen, unsigned bit);
void parse_scalar_field(ProjectParser *parser, XsProjectValue *value);
void parse_authors(ProjectParser *parser);
void parse_compiler_options(ProjectParser *parser);

#define skip_newlines project_skip_newlines
#define token_is project_token_is
#define copy_string project_copy_string

#endif
