/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef XS_SYNTAX_PARSER_H
#define XS_SYNTAX_PARSER_H

#include "xs/diagnostic.h"
#include "xs/syntax_ast.h"

#include <stdint.h>

bool xs_syntax_parse(const XsSource *source, uint64_t file_id, XsDiagnostics *diagnostics, XsSyntaxTree *tree);

#endif
