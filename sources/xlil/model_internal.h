#ifndef XS_XLIL_MODEL_INTERNAL_H
#define XS_XLIL_MODEL_INTERNAL_H

#include "xs/lil.h"

typedef enum
{
  XS_LIL_INSTRUCTION_CONST_I64,
} XsLilInstructionKind;

typedef enum
{
  XS_LIL_TERMINATOR_NONE,
  XS_LIL_TERMINATOR_RETURN,
} XsLilTerminatorKind;

typedef struct
{
  XsLilTerminatorKind kind;
  bool has_value;
  XsLilValueId value;
} XsLilTerminator;

typedef struct
{
  XsLilType type;
} XsLilValue;

typedef struct
{
  XsLilInstructionKind kind;
  XsLilValueId result;
  int64_t immediate_i64;
} XsLilInstruction;

struct XsLilBlock
{
  char *label;
  XsLilFunction *owner;
  uint32_t id;
  XsLilInstruction *instructions;
  size_t instruction_count;
  size_t instruction_capacity;
  XsLilTerminator terminator;
};

struct XsLilFunction
{
  char *name;
  XsLilType return_type;
  XsLilType *parameters;
  size_t parameter_count;
  bool is_definition;
  XsLilValue *values;
  size_t value_count;
  size_t value_capacity;
  XsLilBlock **blocks;
  size_t block_count;
  size_t block_capacity;
};

struct XsLilModule
{
  char *name;
  XsLilFunction *functions;
  size_t function_count;
  size_t function_capacity;
};

void xs_lil_clear_error(XsLilError *error);
XsLilStatus xs_lil_set_error(XsLilError *error, XsLilStatus status, const char *message);
char *xs_lil_copy_text(const char *text);
XsLilStatus xs_lil_write_checked(FILE *stream, XsLilError *error, const char *text);

#endif
