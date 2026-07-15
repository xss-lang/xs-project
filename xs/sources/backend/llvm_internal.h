/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XS_BACKEND_LLVM_INTERNAL_H
#define XS_BACKEND_LLVM_INTERNAL_H

#include "xs/backend/llvm_backend.h"

struct XsLlvmBackend
{
  LLVMContextRef context;
  LLVMTargetMachineRef target_machine;
  LLVMTargetDataRef target_data;
  char *target_triple;
  char *data_layout;
  XsLlvmOptimizationLevel optimization;
  bool verify_modules;
};

struct XsLlvmCodegenUnit
{
  XsLlvmBackend *backend;
  LLVMModuleRef module;
  LLVMTypeRef *lil_types;
  size_t lil_type_count;
};

XsBackendStatus xs_llvm_codegen_lil_type(XsLlvmCodegenUnit *unit, XsLilType type, LLVMTypeRef *llvm_type,
                                         XsBackendError *error);
XsBackendStatus xs_llvm_lower_aggregate_instruction(XsLlvmCodegenUnit *unit, LLVMBuilderRef builder,
                                                    const XsLilFunction *function, const XsLilBlock *block,
                                                    size_t index, LLVMValueRef *values, size_t value_count,
                                                    XsBackendError *error);

#endif
