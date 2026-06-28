#ifndef XS_MIR_HIR_LOWERING_H
#define XS_MIR_HIR_LOWERING_H

#include "xs/hir/symbol_table.h"
#include "xs/mir.h"

XsMirStatus xs_mir_module_add_hir_function_declarations(XsMirModule *module, const XsHirSymbolTable *symbols,
                                                        XsMirError *error);

#endif
