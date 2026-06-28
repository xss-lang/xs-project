#ifndef XS_MIR_H
#define XS_MIR_H

#include "xs/lil.h"

#include <stddef.h>
#include <stdio.h>

typedef XsLilType XsMirType;

typedef enum
{
  XS_MIR_OK,
  XS_MIR_INVALID_ARGUMENT,
  XS_MIR_ALLOCATION_FAILED,
  XS_MIR_IO_ERROR,
  XS_MIR_UNSUPPORTED,
} XsMirStatus;

typedef struct
{
  XsMirStatus status;
  char message[256];
} XsMirError;

typedef struct XsMirFunction XsMirFunction;
typedef struct XsMirModule XsMirModule;

XsMirStatus xs_mir_module_create(const char *name, XsMirModule **module, XsMirError *error);
void xs_mir_module_destroy(XsMirModule *module);
const char *xs_mir_module_name(const XsMirModule *module);

XsMirStatus xs_mir_module_add_function_declaration(XsMirModule *module, const char *qualified_name,
                                                   XsMirType return_type, const XsMirType *parameters,
                                                   size_t parameter_count, XsMirError *error);
size_t xs_mir_module_function_count(const XsMirModule *module);
const XsMirFunction *xs_mir_module_function_at(const XsMirModule *module, size_t index);
const char *xs_mir_function_name(const XsMirFunction *function);
XsMirType xs_mir_function_return_type(const XsMirFunction *function);
const XsMirType *xs_mir_function_parameters(const XsMirFunction *function);
size_t xs_mir_function_parameter_count(const XsMirFunction *function);

XsMirStatus xs_mir_module_write_text(const XsMirModule *module, FILE *stream, XsMirError *error);

#endif
