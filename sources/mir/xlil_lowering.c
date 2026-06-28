#include "xs/mir/xlil_lowering.h"

#include <stdio.h>

static XsMirStatus set_error(XsMirError *error, XsMirStatus status, const char *message)
{
  if (error != NULL) {
    error->status = status;
    snprintf(error->message, sizeof(error->message), "%s", message == NULL ? "MIR to XLIL lowering error" : message);
  }
  return status;
}

XsMirStatus xs_lil_module_add_mir_function_declarations(XsLilModule *module, const XsMirModule *mir, XsMirError *error)
{
  if (module == NULL || mir == NULL)
    return set_error(error, XS_MIR_INVALID_ARGUMENT, "valid XLIL and MIR modules are required");
  for (size_t i = 0; i < xs_mir_module_function_count(mir); ++i) {
    const XsMirFunction *function = xs_mir_module_function_at(mir, i);
    XsLilError lil_error = {0};
    XsLilStatus status = xs_lil_module_add_function(
        module, xs_mir_function_name(function), xs_mir_function_return_type(function),
        xs_mir_function_parameters(function), xs_mir_function_parameter_count(function), &lil_error);
    if (status == XS_LIL_ALLOCATION_FAILED)
      return set_error(error, XS_MIR_ALLOCATION_FAILED, lil_error.message);
    if (status != XS_LIL_OK)
      return set_error(error, XS_MIR_INVALID_ARGUMENT, lil_error.message);
  }
  return XS_MIR_OK;
}
