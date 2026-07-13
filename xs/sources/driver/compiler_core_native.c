/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "compiler_core_native.h"

#include "direct_xlil.h"

#include "xs/lil.h"

#include <stddef.h>

bool xs_driver_compiler_core_native_available(const XsCompilerCoreSession *session)
{
  uint64_t length = 0;
  return xslang_compiler_core_session_xlil_text(session, &length) != nullptr && length != 0;
}

bool xs_driver_build_compiler_core_native(const char *input_path, const XsCompilerCoreSession *session,
                                          XsDiagnostics *diagnostics, XsSpan span)
{
  uint64_t raw_length = 0;
  const uint8_t *text = xslang_compiler_core_session_xlil_text(session, &raw_length);
  if(text == nullptr || raw_length == 0 || raw_length > SIZE_MAX)
    return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, span,
                              "Rust compiler core did not produce a complete XLIL module") &&
           false;
  XsLilModule *module = nullptr;
  XsLilError error = {0};
  if(xs_lil_module_parse_text(input_path, (const char *)text, (size_t)raw_length, &module, &error) != XS_LIL_OK)
    return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, span,
                              error.message[0] == '\0' ? "Rust compiler-core XLIL could not be parsed"
                                                       : error.message) &&
           false;
  bool success = xs_lil_module_verify(module, &error) == XS_LIL_OK;
  if(!success)
    xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, span,
                       error.message[0] == '\0' ? "Rust compiler-core XLIL verification failed" : error.message);
  if(success)
    success = xs_driver_build_lil_module_native(input_path, module);
  if(!success && !xs_diagnostics_has_error(diagnostics))
    xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, span, "Rust compiler-core XLIL native backend failed");
  xs_lil_module_destroy(module);
  return success;
}
