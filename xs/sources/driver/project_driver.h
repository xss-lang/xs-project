/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XS_DRIVER_PROJECT_DRIVER_H
#define XS_DRIVER_PROJECT_DRIVER_H

#include "options.h"

#include <stddef.h>

bool xs_driver_resolve_kotlin_project(char ***paths, size_t *path_count, XsCompilerSettings *settings);
void xs_driver_free_project_paths(char **paths, size_t path_count);

#endif
