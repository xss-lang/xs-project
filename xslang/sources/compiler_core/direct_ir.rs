/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use std::{slice, str};

use super::FfiStatus;
use crate::{hir, mir, xlil};

pub struct DirectIrSession
{
  xlil_text: Option<Vec<u8>>,
  diagnostics: Vec<String>,
}

impl DirectIrSession
{
  fn from_xhir(text: &[u8]) -> Self
  {
    let text = match str::from_utf8(text)
    {
      Ok(text) => text,
      Err(error) => return Self::failed(format!("XHIR input is not valid UTF-8: {error}")),
    };
    let program = match hir::text::parse_xhir_program(text)
    {
      Ok(program) => program,
      Err(errors) =>
      {
        return Self { xlil_text: None,
                      diagnostics: errors.into_iter()
                                         .map(|error| format!("XHIR line {}: {}", error.line, error.message))
                                         .collect() };
      }
    };
    let Some(aggregate_registry) = hir::aggregate_registry::build_functions(&program.functions)
    else
    {
      return Self::failed("XHIR composite type registry could not be constructed".to_string());
    };
    let collection_registry = hir::collection_registry::build_functions(&program.functions, &aggregate_registry);
    let aggregate_types = aggregate_registry.layouts
                                            .iter()
                                            .map(|layout| xlil::AggregateType { id: layout.value_type.registry_id,
                                                                                name: layout.name.clone(),
                                                                                fields: layout.fields.clone() })
                                            .collect();
    let array_types = collection_registry.arrays
                                         .iter()
                                         .map(|layout| xlil::ArrayType { id: layout.value_type.registry_id,
                                                                         element_type: layout.element_type,
                                                                         length: layout.length })
                                         .collect();
    let mut diagnostics = Vec::new();
    let mut functions = Vec::with_capacity(program.functions.len());
    for (function, parameter_count) in program.functions.into_iter().zip(program.parameter_counts)
    {
      let name = function.name.clone();
      diagnostics.extend(hir::type_check::TypeChecker::new().check_function(&function)
                                                            .into_iter()
                                                            .map(|error| {
                                                              format!("XHIR function '{name}' type check: {}",
                                                                      error.message)
                                                            }));
      match hir::mir_lowering::HirToMirLowerer::new().with_aggregate_types(&aggregate_registry)
                                                     .with_collection_types(&collection_registry)
                                                     .lower_function_with_parameters(&function, parameter_count)
      {
        Ok(function) => functions.push(function),
        Err(errors) =>
        {
          diagnostics.extend(errors.into_iter()
                                   .map(|error| {
                                     format!("XHIR function '{name}' MIR lowering failed: {}", error.message)
                                   }))
        }
      }
    }
    if diagnostics.is_empty()
    {
      Self::from_mir_program(program.name, aggregate_types, array_types, functions)
    }
    else
    {
      Self { xlil_text: None,
             diagnostics }
    }
  }

  fn from_xmir(text: &[u8]) -> Self
  {
    let text = match str::from_utf8(text)
    {
      Ok(text) => text,
      Err(error) => return Self::failed(format!("XMIR input is not valid UTF-8: {error}")),
    };
    let program = match mir::text::parse_xmir_program(text)
    {
      Ok(program) => program,
      Err(errors) =>
      {
        return Self { xlil_text: None,
                      diagnostics: errors.into_iter()
                                         .map(|error| format!("XMIR line {}: {}", error.line, error.message))
                                         .collect() };
      }
    };
    Self::from_mir_program(program.name,
                           program.aggregate_types,
                           program.array_types,
                           program.functions)
  }

  fn from_mir_program(name: String,
                      aggregate_types: Vec<xlil::AggregateType>,
                      array_types: Vec<xlil::ArrayType>,
                      input_functions: Vec<mir::Function>)
                      -> Self
  {
    let mut diagnostics = Vec::new();
    let mut functions = Vec::with_capacity(input_functions.len());
    for function in input_functions
    {
      let name = function.name.clone();
      diagnostics.extend(mir::verify::verify_function(&function).into_iter()
                                                                .map(|error| {
                                                                  format!("MIR function '{name}': {}", error.message)
                                                                }));
      diagnostics.extend(mir::BorrowChecker::new().check_function(&function)
                                                  .into_iter()
                                                  .map(|error| {
                                                    format!("MIR function '{name}' borrow check: {}", error.message)
                                                  }));
      functions.push(function);
    }
    if !diagnostics.is_empty()
    {
      return Self { xlil_text: None,
                    diagnostics };
    }
    let mut module = xlil::Module { name,
                                    aggregate_types,
                                    array_types,
                                    functions: Vec::new() };
    for function in functions
    {
      let name = function.name.clone();
      let optimized = match mir::optimizer::optimize_verified_function(function)
      {
        Ok(optimized) => optimized.function,
        Err(error) =>
        {
          diagnostics.push(format!("MIR function '{name}' optimization failed: {error:?}"));
          continue;
        }
      };
      match xlil::lowering::MirToXlilLowerer::new().lower_function(&optimized)
      {
        Ok(function) => module.add_function(function),
        Err(errors) =>
        {
          diagnostics.extend(errors.into_iter()
                                   .map(|error| format!("MIR function '{name}' lowering failed: {}", error.message)))
        }
      }
    }
    diagnostics.extend(xlil::verify::verify_module(&module).into_iter().map(|error| {
                                                                         format!("lowered XLIL verification failed: {}",
                                                                                 error.message)
                                                                       }));
    if diagnostics.is_empty()
    {
      Self { xlil_text: Some(xlil::writer::module_to_string(&module).into_bytes()),
             diagnostics }
    }
    else
    {
      Self { xlil_text: None,
             diagnostics }
    }
  }

  fn failed(message: String) -> Self
  {
    Self { xlil_text: None,
           diagnostics: vec![message] }
  }
}

/// Parses, type-checks, and lowers an XHIR v0 program through MIR and XLIL.
///
/// # Safety
///
/// `text` must reference `length` readable bytes when non-null. `session` must be writable.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn xslang_compiler_core_direct_xhir_create(text: *const u8,
                                                                 length: u64,
                                                                 session: *mut *mut DirectIrSession)
                                                                 -> FfiStatus
{
  unsafe { create_direct_ir(text, length, session, DirectIrSession::from_xhir) }
}

/// Parses, verifies, borrow-checks, optimizes, and lowers an XMIR v0 program.
///
/// # Safety
///
/// `text` must reference `length` readable bytes when non-null. `session` must be writable.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn xslang_compiler_core_direct_xmir_create(text: *const u8,
                                                                 length: u64,
                                                                 session: *mut *mut DirectIrSession)
                                                                 -> FfiStatus
{
  unsafe { create_direct_ir(text, length, session, DirectIrSession::from_xmir) }
}

unsafe fn create_direct_ir(text: *const u8,
                           length: u64,
                           session: *mut *mut DirectIrSession,
                           compile: fn(&[u8]) -> DirectIrSession)
                           -> FfiStatus
{
  if session.is_null()
  {
    return FfiStatus::NullArgument;
  }
  // SAFETY: The caller promises a writable output pointer.
  unsafe { *session = std::ptr::null_mut() };
  if text.is_null() && length != 0
  {
    return FfiStatus::NullArgument;
  }
  let Ok(length) = usize::try_from(length)
  else
  {
    return FfiStatus::InvalidPacket;
  };
  let bytes = if length == 0
  {
    &[]
  }
  else
  {
    // SAFETY: The caller promises a readable input range for this call.
    unsafe { slice::from_raw_parts(text, length) }
  };
  let value = Box::new(compile(bytes));
  // SAFETY: The caller promises a writable output pointer.
  unsafe { *session = Box::into_raw(value) };
  FfiStatus::Ok
}

/// Returns the number of diagnostics owned by a direct-IR session.
///
/// # Safety
///
/// `session` must be null or point to a live direct-IR session.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn xslang_compiler_core_direct_ir_diagnostic_count(session: *const DirectIrSession) -> u64
{
  unsafe { session.as_ref() }.map_or(0, |value| value.diagnostics.len() as u64)
}

/// Returns one borrowed diagnostic string.
///
/// # Safety
///
/// `session` must be null or live; `length` must be null or writable.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn xslang_compiler_core_direct_ir_diagnostic_text(session: *const DirectIrSession,
                                                                        index: u64,
                                                                        length: *mut u64)
                                                                        -> *const u8
{
  let text =
    usize::try_from(index).ok()
                          .and_then(|index| unsafe { session.as_ref() }.and_then(|value| value.diagnostics.get(index)));
  unsafe { borrowed_text(text.map(String::as_bytes), length) }
}

/// Returns borrowed XLIL v0 text after a successful direct-IR compilation.
///
/// # Safety
///
/// `session` must be null or live; `length` must be null or writable.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn xslang_compiler_core_direct_ir_xlil_text(session: *const DirectIrSession,
                                                                  length: *mut u64)
                                                                  -> *const u8
{
  let text = unsafe { session.as_ref() }.and_then(|value| value.xlil_text.as_deref());
  unsafe { borrowed_text(text, length) }
}

unsafe fn borrowed_text(text: Option<&[u8]>, length: *mut u64) -> *const u8
{
  if !length.is_null()
  {
    // SAFETY: The caller promises a writable length pointer.
    unsafe { *length = text.map_or(0, |value| value.len() as u64) };
  }
  text.map_or(std::ptr::null(), <[u8]>::as_ptr)
}

/// Releases a direct-IR session created by this module.
///
/// # Safety
///
/// `session` must be null or a live pointer returned by a matching create function and not previously released.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn xslang_compiler_core_direct_ir_free(session: *mut DirectIrSession)
{
  if !session.is_null()
  {
    // SAFETY: The pointer was returned by the matching create function and is released once.
    drop(unsafe { Box::from_raw(session) });
  }
}

#[cfg(test)]
mod tests
{
  use super::*;

  const VALID_XMIR: &str = ".xmir version 0\nprogram root\n\nfunction main\nreturns i32\n\nlocals\n  local 0\n    \
                            name result\n    type i32\n    mutability immutable\n.end\n\ncontrol_flow\n  block 0\n    \
                            statements\n      statement const.i32\n        target local 0\n        value 7\n    \
                            terminator return\n      value local 0\n.end\n.function end\n.program end\n";
  const VALID_XHIR: &str = ".xhir version 0\nprogram root\n\nfunction main\n  signature\n    returns Long\n  .end\n  \
                            body\n    return\n      literal integer 7\n  .end\n.function end\n.program end\n";

  #[test]
  fn compiles_checked_xhir_program_through_mir_to_xlil()
  {
    let session = DirectIrSession::from_xhir(VALID_XHIR.as_bytes());
    assert!(session.diagnostics.is_empty(), "{:?}", session.diagnostics);
    let text = str::from_utf8(session.xlil_text.as_deref().expect("XLIL should exist")).expect("valid UTF-8");
    assert!(text.contains(".func main : () -> i32"));
    assert!(text.contains("%r0:i32 = const.i32 7"));
  }

  #[test]
  fn compiles_verified_xmir_program_to_xlil_registry()
  {
    let session = DirectIrSession::from_xmir(VALID_XMIR.as_bytes());
    assert!(session.diagnostics.is_empty());
    let text = str::from_utf8(session.xlil_text.as_deref().expect("XLIL should exist")).expect("valid UTF-8");
    assert!(text.contains(".func main : () -> i32"));
    assert!(text.contains("%r0:i32 = const.i32 7"));
  }

  #[test]
  fn rejects_structurally_invalid_xmir_before_lowering()
  {
    let session = DirectIrSession::from_xmir(VALID_XMIR.replace("value local 0", "value local 9").as_bytes());
    assert!(session.xlil_text.is_none());
    assert!(session.diagnostics
                   .iter()
                   .any(|message| message.contains("local id 9 is not declared")),
            "{:?}",
            session.diagnostics);
  }
}
