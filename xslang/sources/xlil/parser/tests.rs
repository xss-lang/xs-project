/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;
use crate::xlil::writer::module_to_string;

#[test]
fn parses_function_declaration()
{
  let module =
    parse_module(".xlil version 0\n.xlil module App\n.extern xs$App$External : (i64) -> i64\n").expect("parse should \
                                                                                                        succeed");

  assert_eq!(module.name, "App");
  assert_eq!(module.functions.len(), 1);
  assert!(!module.functions[0].is_definition);
  assert_eq!(module.functions[0].parameters, vec![Type::I64]);
}

#[test]
fn parses_explicit_parameter_records()
{
  let text =
    ".xlil version 0\n.xlil module App\n.func Identity : (i64) -> i64\n.param %r0:i64\nbb0.entry:\n  ret %r0\n.end\n";
  let module = parse_module(text).expect("parameterized function should parse");

  assert_eq!(module.functions[0].parameter_value(0), Some(ValueId(0)));
  assert_eq!(module.functions[0].values.len(), 1);
}

#[test]
fn rejects_missing_version_header()
{
  let diagnostics =
    parse_module(".xlil module App\n.extern xs$App$External : (i64) -> i64\n").expect_err("parse must fail");

  assert_eq!(diagnostics[0].code, DiagnosticCode::InvalidVersionHeader);
}

#[test]
fn rejects_empty_input()
{
  let diagnostics = parse_module("").expect_err("empty input must fail");

  assert_eq!(diagnostics[0].code, DiagnosticCode::EmptyInput);
}

#[test]
fn rejects_unsupported_version()
{
  let diagnostics = parse_module(".xlil version 1\n.xlil module App\n").expect_err("unsupported version must fail");

  assert_eq!(diagnostics[0].code, DiagnosticCode::InvalidVersionHeader);
  assert!(diagnostics[0].message.contains("version 1 is not supported"));
}

#[test]
fn rejects_invalid_version_number()
{
  let diagnostics = parse_module(".xlil version current\n.xlil module App\n").expect_err("invalid version must fail");

  assert_eq!(diagnostics[0].code, DiagnosticCode::InvalidVersionHeader);
  assert!(diagnostics[0].message.contains("version number is invalid"));
}

#[test]
fn roundtrips_const_i64_function()
{
  let text = ".xlil version 0\n.xlil module App\n.func xs$App$Value : () -> i64\nbb0.entry:\n  %r0:i64 = const 42\n  \
              ret %r0\n.end\n";

  let module = parse_module(text).expect("parse should succeed");

  assert_eq!(module_to_string(&module), text);
}

#[test]
fn roundtrips_const_i32_function()
{
  let text =
    ".xlil version 0\n.xlil module App\n.func main : () -> i32\nbb0.entry:\n  %r0:i32 = const.i32 0\n  ret %r0\n.end\n";
  let parsed = parse_module(text).expect("XLIL must parse");
  assert_eq!(crate::xlil::writer::module_to_string(&parsed), text);
}

#[test]
fn roundtrips_call_function()
{
  let text = ".xlil version 0\n.xlil module App\n.func xs$App$Call : () -> i64\nbb0.entry:\n  %r0:i64 = const 7\n  \
              %r1:i64 = call xs$App$Callee(%r0)\n  ret %r1\n.end\n";

  let module = parse_module(text).expect("parse should succeed");

  assert_eq!(module_to_string(&module), text);
}

#[test]
fn roundtrips_add_i64_function()
{
  let text = ".xlil version 0\n.xlil module App\n.func xs$App$Add : () -> i64\nbb0.entry:\n  %r0:i64 = const 2\n  \
              %r1:i64 = const 3\n  %r2:i64 = add.i64 %r0, %r1\n  ret %r2\n.end\n";

  let module = parse_module(text).expect("parse should succeed");

  assert_eq!(module_to_string(&module), text);
}

#[test]
fn roundtrips_sub_i64_function()
{
  let text = ".xlil version 0\n.xlil module App\n.func xs$App$Sub : () -> i64\nbb0.entry:\n  %r0:i64 = const 8\n  \
              %r1:i64 = const 3\n  %r2:i64 = sub.i64 %r0, %r1\n  ret %r2\n.end\n";

  let module = parse_module(text).expect("parse should succeed");

  assert_eq!(module_to_string(&module), text);
}

#[test]
fn roundtrips_mul_i64_function()
{
  let text = ".xlil version 0\n.xlil module App\n.func xs$App$Mul : () -> i64\nbb0.entry:\n  %r0:i64 = const 6\n  \
              %r1:i64 = const 7\n  %r2:i64 = mul.i64 %r0, %r1\n  ret %r2\n.end\n";

  let module = parse_module(text).expect("parse should succeed");

  assert_eq!(module_to_string(&module), text);
}

#[test]
fn roundtrips_eq_i64_function()
{
  let text = ".xlil version 0\n.xlil module App\n.func xs$App$Eq : () -> bool\nbb0.entry:\n  %r0:i64 = const 7\n  \
              %r1:i64 = const 7\n  %r2:bool = eq.i64 %r0, %r1\n  ret %r2\n.end\n";

  let module = parse_module(text).expect("parse should succeed");

  assert_eq!(module_to_string(&module), text);
}

#[test]
fn roundtrips_i32_instruction_family()
{
  let text =
    ".xlil version 0\n.xlil module App\n.func xs$App$I32 : () -> bool\nbb0.entry:\n  %r0:i32 = const.i32 2\n  %r1:i32 \
     = const.i32 3\n  %r2:i32 = add.i32 %r0, %r1\n  %r3:i32 = sub.i32 %r0, %r1\n  %r4:i32 = mul.i32 %r0, %r1\n  \
     %r5:bool = eq.i32 %r0, %r1\n  %r6:bool = lt.i32 %r0, %r1\n  %r7:bool = le.i32 %r0, %r1\n  %r8:bool = gt.i32 %r0, \
     %r1\n  %r9:bool = ge.i32 %r0, %r1\n  ret %r9\n.end\n";

  let module = parse_module(text).expect("parse should succeed");

  assert_eq!(module_to_string(&module), text);
}

#[test]
fn roundtrips_const_bool_function()
{
  let text = ".xlil version 0\n.xlil module App\n.func xs$App$Truth : () -> bool\nbb0.entry:\n  %r0:bool = const.bool \
              true\n  ret %r0\n.end\n";

  let module = parse_module(text).expect("parse should succeed");

  assert_eq!(module_to_string(&module), text);
}

#[test]
fn roundtrips_branch_if_function()
{
  let text = ".xlil version 0\n.xlil module App\n.func xs$App$BranchIf : () -> void\nbb0.entry:\n  %r0:bool = \
              const.bool true\n  br_if %r0, bb1, bb2\nbb1.then:\n  ret\nbb2.else:\n  ret\n.end\n";

  let module = parse_module(text).expect("parse should succeed");

  assert_eq!(module_to_string(&module), text);
}

#[test]
fn roundtrips_panic_function()
{
  let text = ".xlil version 0\n.xlil module App\n.func xs$App$Panic : () -> void\nbb0.entry:\n  panic\n.end\n";

  let module = parse_module(text).expect("parse should succeed");

  assert_eq!(module_to_string(&module), text);
}

#[test]
fn roundtrips_stack_slot_memory()
{
  let text = ".xlil version 0\n.xlil module App\n.func Memory : () -> i32\n.slot %s0:i32\nbb0.entry:\n  %r0:i32 = \
              const.i32 7\n  store %r0, %s0\n  %r1:i32 = load %s0\n  ret %r1\n.end\n";

  let module = parse_module(text).expect("parse should succeed");

  assert!(crate::xlil::verify::verify_module(&module).is_empty());
  assert_eq!(module_to_string(&module), text);
}

#[test]
fn roundtrips_f32_and_f64_constant_bits()
{
  let text = ".xlil version 0\n.xlil module FloatConstants\n.func F32Value : () -> f32\nbb0.entry:\n  %r0:f32 = \
              const.f32 0x3fc00000\n  ret %r0\n.end\n.func F64Value : () -> f64\nbb0.entry:\n  %r0:f64 = const.f64 \
              0x3ff8000000000000\n  ret %r0\n.end\n";
  let module = parse_module(text).expect("floating constants should parse");

  assert!(crate::xlil::verify::verify_module(&module).is_empty());
  assert_eq!(module_to_string(&module), text);
}

#[test]
fn rejects_legacy_plain_value_ids()
{
  let text = ".xlil version 0\n.xlil module App\n.func xs$App$Legacy : () -> i64\nbb0.entry:\n  %0:i64 = const 42\n  \
              ret %0\n.end\n";

  let diagnostics = parse_module(text).expect_err("legacy plain value ids must fail");

  assert_eq!(diagnostics[0].code, DiagnosticCode::InvalidValueId);
}

#[test]
fn parses_missing_terminator_marker()
{
  let text =
    ".xlil version 0\n.xlil module App\n.func xs$App$Broken : () -> void\nbb0.entry:\n  .missing_terminator\n.end\n";

  let module = parse_module(text).expect("parse should succeed");

  assert_eq!(module.functions[0].blocks[0].terminator, None);
}

#[test]
fn rejects_unknown_type()
{
  let diagnostics =
    parse_module(".xlil version 0\n.xlil module App\n.extern bad : () -> nope\n").expect_err("parse must fail");

  assert_eq!(diagnostics[0].code, DiagnosticCode::InvalidType);
}

#[test]
fn roundtrips_branch_function()
{
  let text = ".xlil version 0\n.xlil module App\n.func xs$App$Branch : () -> void\nbb0.entry:\n  br bb1\nbb1.exit:\n  \
              ret\n.end\n";

  let module = parse_module(text).expect("parse should succeed");

  assert_eq!(module_to_string(&module), text);
}
