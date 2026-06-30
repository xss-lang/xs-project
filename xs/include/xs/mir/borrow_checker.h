#ifndef XS_MIR_BORROW_CHECKER_H
#define XS_MIR_BORROW_CHECKER_H

#include "xs/mir.h"

XsMirStatus xs_mir_borrow_check_module(const XsMirModule *module, XsMirError *error);

#endif
