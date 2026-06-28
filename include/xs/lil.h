#ifndef XS_LIL_H
#define XS_LIL_H

#include <stddef.h>
#include <stdio.h>

typedef enum
{
  XS_LIL_OK,
  XS_LIL_INVALID_ARGUMENT,
  XS_LIL_ALLOCATION_FAILED,
  XS_LIL_IO_ERROR,
} XsLilStatus;

typedef struct
{
  XsLilStatus status;
  char message[256];
} XsLilError;

typedef enum
{
  XS_LIL_TYPE_VOID,
  XS_LIL_TYPE_BOOL,
  XS_LIL_TYPE_U8,
  XS_LIL_TYPE_I8,
  XS_LIL_TYPE_U16,
  XS_LIL_TYPE_I16,
  XS_LIL_TYPE_U32,
  XS_LIL_TYPE_I32,
  XS_LIL_TYPE_U64,
  XS_LIL_TYPE_I64,
  XS_LIL_TYPE_U128,
  XS_LIL_TYPE_I128,
  XS_LIL_TYPE_F16,
  XS_LIL_TYPE_F32,
  XS_LIL_TYPE_F64,
  XS_LIL_TYPE_F128,
} XsLilTypeKind;

typedef struct
{
  XsLilTypeKind kind;
} XsLilType;

typedef struct XsLilModule XsLilModule;

XsLilStatus xs_lil_module_create(const char *name, XsLilModule **module, XsLilError *error);
void xs_lil_module_destroy(XsLilModule *module);
const char *xs_lil_module_name(const XsLilModule *module);

XsLilStatus xs_lil_module_add_function(XsLilModule *module, const char *name, XsLilType return_type,
                                       const XsLilType *parameters, size_t parameter_count, XsLilError *error);
size_t xs_lil_module_function_count(const XsLilModule *module);

const char *xs_lil_type_name(XsLilType type);
XsLilStatus xs_lil_module_write_text(const XsLilModule *module, FILE *stream, XsLilError *error);

#endif
