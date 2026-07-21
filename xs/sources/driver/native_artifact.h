/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef XS_DRIVER_NATIVE_ARTIFACT_H
#define XS_DRIVER_NATIVE_ARTIFACT_H

#include <stddef.h>

char *xs_driver_native_artifact_path(const char *input_path, const char *extension);
int xs_driver_run_native_artifact(const char *input_path);

#endif
