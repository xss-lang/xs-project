/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use crate::xlil::{
  Block, BlockId, Function, Instruction, Module, SUPPORTED_XLIL_VERSION, Terminator, Type, Value, ValueId,
  is_supported_xlil_version, type_from_name,
};

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum DiagnosticCode
{
  EmptyInput,
  InvalidModuleHeader,
  InvalidVersionHeader,
  InvalidFunctionRecord,
  InvalidSignature,
  InvalidType,
  InvalidBlockRecord,
  InvalidInstruction,
  InvalidTerminator,
  InvalidValueId,
  InvalidInteger,
  UnexpectedEndOfInput,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Diagnostic
{
  pub code: DiagnosticCode,
  pub line: usize,
  pub message: String,
}

pub fn parse_module(text: &str) -> Result<Module, Vec<Diagnostic>>
{
  let mut parser = Parser { lines: text.lines().collect(),
                            cursor: 0,
                            diagnostics: vec![] };
  let module = parser.module();
  if parser.diagnostics.is_empty()
  {
    Ok(module.unwrap_or_else(|| Module::new("")))
  }
  else
  {
    Err(parser.diagnostics)
  }
}

struct Parser<'text>
{
  lines: Vec<&'text str>,
  cursor: usize,
  diagnostics: Vec<Diagnostic>,
}

impl Parser<'_>
{
  fn module(&mut self) -> Option<Module>
  {
    if !self.version_header()
    {
      return None;
    }
    let Some(header) = self.next_line()
    else
    {
      self.report(DiagnosticCode::InvalidModuleHeader, 2, "XLIL module header is missing");
      return None;
    };
    let Some(name) = header.strip_prefix(".xlil module ")
    else
    {
      self.report(DiagnosticCode::InvalidModuleHeader, 2, "XLIL module header is invalid");
      return None;
    };
    let mut module = Module::new(name);
    while self.cursor < self.lines.len()
    {
      if self.current_line().is_some_and(str::is_empty)
      {
        self.cursor += 1;
        continue;
      }
      let Some(function) = self.function()
      else
      {
        break;
      };
      module.add_function(function);
    }
    Some(module)
  }

  fn version_header(&mut self) -> bool
  {
    let Some(version) = self.next_line()
    else
    {
      self.report(DiagnosticCode::EmptyInput, 1, "XLIL input is empty");
      return false;
    };
    let Some(version) = version.strip_prefix(".xlil version ")
    else
    {
      self.report(DiagnosticCode::InvalidVersionHeader,
                  1,
                  "XLIL version header is invalid");
      return false;
    };
    match version.parse::<u32>()
    {
      Ok(version) if is_supported_xlil_version(version) => true,
      Ok(version) =>
      {
        self.report(DiagnosticCode::InvalidVersionHeader,
                    1,
                    &format!("XLIL version {version} is not supported; supported version is {SUPPORTED_XLIL_VERSION}"));
        false
      }
      Err(_) =>
      {
        self.report(DiagnosticCode::InvalidVersionHeader,
                    1,
                    "XLIL version number is invalid");
        false
      }
    }
  }

  fn function(&mut self) -> Option<Function>
  {
    let line_number = self.line_number();
    let line = self.next_line()?;
    if let Some(signature) = line.strip_prefix(".extern ")
    {
      return self.signature(signature, false, line_number);
    }
    if let Some(signature) = line.strip_prefix(".func ")
    {
      let mut function = self.signature(signature, true, line_number)?;
      self.body(&mut function);
      return Some(function);
    }
    self.report(DiagnosticCode::InvalidFunctionRecord,
                line_number,
                "XLIL function record is invalid");
    None
  }

  fn signature(&mut self, signature: &str, is_definition: bool, line: usize) -> Option<Function>
  {
    let Some((name, rest)) = signature.split_once(" : ")
    else
    {
      self.report(DiagnosticCode::InvalidSignature, line, "XLIL signature is invalid");
      return None;
    };
    let Some((parameters, return_type)) = rest.split_once(" -> ")
    else
    {
      self.report(DiagnosticCode::InvalidSignature, line, "XLIL signature is invalid");
      return None;
    };
    let Some(parameters) = parameters.strip_prefix('(')
                                     .and_then(|parameters| parameters.strip_suffix(')'))
    else
    {
      self.report(DiagnosticCode::InvalidSignature, line, "XLIL parameter list is invalid");
      return None;
    };
    let return_type = self.type_name(return_type, line)?;
    let parameters = self.parameters(parameters, line)?;
    Some(if is_definition
         {
           Function::definition(name, return_type, parameters)
         }
         else
         {
           Function::declaration(name, return_type, parameters)
         })
  }

  fn parameters(&mut self, parameters: &str, line: usize) -> Option<Vec<Type>>
  {
    if parameters.is_empty()
    {
      return Some(vec![]);
    }
    let mut parsed = Vec::new();
    for parameter in parameters.split(", ")
    {
      parsed.push(self.type_name(parameter, line)?);
    }
    Some(parsed)
  }

  fn type_name(&mut self, name: &str, line: usize) -> Option<Type>
  {
    let parsed = type_from_name(name);
    if parsed.is_none()
    {
      self.report(DiagnosticCode::InvalidType, line, "XLIL type name is unknown");
    }
    parsed
  }

  fn body(&mut self, function: &mut Function)
  {
    let mut parameter = 0;
    while self.cursor < self.lines.len()
    {
      let line = self.current_line().unwrap_or_default();
      if line == ".end"
      {
        self.cursor += 1;
        if parameter != function.parameters.len()
        {
          self.report(DiagnosticCode::InvalidInstruction,
                      self.line_number() - 1,
                      "XLIL function parameter records are incomplete");
        }
        return;
      }
      if let Some(record) = line.strip_prefix(".param ")
      {
        let line = self.line_number();
        let record = record.to_string();
        self.cursor += 1;
        self.parameter_record(function, parameter, &record, line);
        parameter += 1;
        continue;
      }
      if parameter != function.parameters.len()
      {
        self.report(DiagnosticCode::InvalidInstruction,
                    self.line_number(),
                    "XLIL function parameter records are incomplete");
        return;
      }
      if !line.starts_with("bb")
      {
        self.report(DiagnosticCode::InvalidBlockRecord,
                    self.line_number(),
                    "XLIL block record is invalid");
        return;
      }
      let block = self.block(function);
      function.blocks.push(block);
    }
    self.report(DiagnosticCode::UnexpectedEndOfInput,
                self.line_number(),
                "XLIL function body is not closed");
  }

  fn parameter_record(&mut self, function: &Function, parameter: usize, record: &str, line: usize)
  {
    let Some((value, type_name)) = record.split_once(':')
    else
    {
      self.report(DiagnosticCode::InvalidInstruction,
                  line,
                  "XLIL parameter record is invalid");
      return;
    };
    let Some(value) = value.strip_prefix('%').and_then(|value| self.value_id(value, line))
    else
    {
      return;
    };
    let Some(value_type) = self.type_name(type_name, line)
    else
    {
      return;
    };
    if parameter >= function.parameters.len() ||
       value != ValueId(parameter as u32) ||
       value_type != function.parameters[parameter]
    {
      self.report(DiagnosticCode::InvalidInstruction,
                  line,
                  "XLIL parameter record does not match function signature");
    }
  }

  fn block(&mut self, function: &mut Function) -> Block
  {
    let line_number = self.line_number();
    let line = self.next_line().unwrap_or_default();
    let (id, label) = self.block_header(&line, line_number)
                          .unwrap_or((BlockId(0), String::new()));
    let mut block = Block { id,
                            label,
                            instructions: vec![],
                            terminator: None };
    while self.cursor < self.lines.len()
    {
      let line = self.current_line().unwrap_or_default().to_string();
      if line == ".end" || line.starts_with("bb")
      {
        return block;
      }
      let line_number = self.line_number();
      self.cursor += 1;
      if let Some(instruction) = line.strip_prefix("  %")
      {
        if let Some(instruction) = self.instruction(function, instruction, line_number)
        {
          block.instructions.push(instruction);
        }
      }
      else if let Some(call) = line.strip_prefix("  call ")
      {
        if let Some(instruction) = self.void_call(function, call, line_number)
        {
          block.instructions.push(instruction);
        }
      }
      else if let Some(terminator) = line.strip_prefix("  ret")
      {
        block.terminator = self.return_terminator(terminator, line_number);
      }
      else if let Some(target) = line.strip_prefix("  br ")
      {
        block.terminator = self.branch_terminator(target, line_number);
      }
      else if let Some(branch_if) = line.strip_prefix("  br_if ")
      {
        block.terminator = self.branch_if_terminator(branch_if, line_number);
      }
      else if line == "  .missing_terminator"
      {
        block.terminator = None;
      }
      else
      {
        self.report(DiagnosticCode::InvalidInstruction,
                    line_number,
                    "XLIL instruction is invalid");
      }
    }
    block
  }

  fn block_header(&mut self, line: &str, line_number: usize) -> Option<(BlockId, String)>
  {
    let line = line.strip_suffix(':')?;
    let (id, label) = line.split_once('.')?;
    let id = id.strip_prefix("bb")?.parse::<u32>().ok()?;
    Some((BlockId(id), label.to_string())).or_else(|| {
                                            self.report(DiagnosticCode::InvalidBlockRecord,
                                                        line_number,
                                                        "XLIL block header is invalid");
                                            None
                                          })
  }

  fn instruction(&mut self, function: &mut Function, text: &str, line: usize) -> Option<Instruction>
  {
    if let Some((result, call)) = text.split_once(" = call ")
    {
      return self.value_call(function, result, call, line);
    }
    if let Some((result, operands)) = text.split_once(" = add.i64 ")
    {
      return self.binary_i64(function, result, operands, "add.i64", line);
    }
    if let Some((result, operands)) = text.split_once(" = sub.i64 ")
    {
      return self.binary_i64(function, result, operands, "sub.i64", line);
    }
    if let Some((result, operands)) = text.split_once(" = mul.i64 ")
    {
      return self.binary_i64(function, result, operands, "mul.i64", line);
    }
    if let Some((result, operands)) = text.split_once(" = eq.i64 ")
    {
      return self.eq_i64(function, result, operands, line);
    }
    if let Some((result, rest)) = text.split_once(" = const.bool ")
    {
      return self.const_bool(function, result, rest, line);
    }
    let Some((result, rest)) = text.split_once(" = const ")
    else
    {
      self.report(DiagnosticCode::InvalidInstruction, line, "XLIL instruction is invalid");
      return None;
    };
    let Some(result) = result.strip_suffix(":i64")
    else
    {
      self.report(DiagnosticCode::InvalidInstruction,
                  line,
                  "XLIL const result type is invalid");
      return None;
    };
    let result = self.value_id(result, line)?;
    let value = rest.parse::<i64>().map_err(|_| ()).ok();
    let Some(value) = value
    else
    {
      self.report(DiagnosticCode::InvalidInteger, line, "XLIL const immediate is invalid");
      return None;
    };
    function.values.push(Value { id: result,
                                 value_type: Type::I64 });
    Some(Instruction::ConstI64 { result,
                                 value })
  }

  fn binary_i64(&mut self,
                function: &mut Function,
                result: &str,
                operands: &str,
                instruction: &str,
                line: usize)
                -> Option<Instruction>
  {
    let Some(result) = result.strip_suffix(":i64")
    else
    {
      self.report(DiagnosticCode::InvalidInstruction,
                  line,
                  &format!("XLIL {instruction} result type is invalid"));
      return None;
    };
    let result = self.value_id(result, line)?;
    let Some((left, right)) = operands.split_once(", ")
    else
    {
      self.report(DiagnosticCode::InvalidInstruction,
                  line,
                  &format!("XLIL {instruction} operands are invalid"));
      return None;
    };
    let left = self.value_operand(left, line)?;
    let right = self.value_operand(right, line)?;
    function.values.push(Value { id: result,
                                 value_type: Type::I64 });
    match instruction
    {
      "add.i64" => Some(Instruction::AddI64 { result,
                                              left,
                                              right }),
      "sub.i64" => Some(Instruction::SubI64 { result,
                                              left,
                                              right }),
      "mul.i64" => Some(Instruction::MulI64 { result,
                                              left,
                                              right }),
      _ => None,
    }
  }

  fn eq_i64(&mut self, function: &mut Function, result: &str, operands: &str, line: usize) -> Option<Instruction>
  {
    let Some(result) = result.strip_suffix(":bool")
    else
    {
      self.report(DiagnosticCode::InvalidInstruction,
                  line,
                  "XLIL eq.i64 result type is invalid");
      return None;
    };
    let result = self.value_id(result, line)?;
    let Some((left, right)) = operands.split_once(", ")
    else
    {
      self.report(DiagnosticCode::InvalidInstruction,
                  line,
                  "XLIL eq.i64 operands are invalid");
      return None;
    };
    let left = self.value_operand(left, line)?;
    let right = self.value_operand(right, line)?;
    function.values.push(Value { id: result,
                                 value_type: Type::BOOL });
    Some(Instruction::EqI64 { result,
                              left,
                              right })
  }

  fn const_bool(&mut self, function: &mut Function, result: &str, value: &str, line: usize) -> Option<Instruction>
  {
    let Some(result) = result.strip_suffix(":bool")
    else
    {
      self.report(DiagnosticCode::InvalidInstruction,
                  line,
                  "XLIL const.bool result type is invalid");
      return None;
    };
    let result = self.value_id(result, line)?;
    let value = match value
    {
      "true" => true,
      "false" => false,
      _ =>
      {
        self.report(DiagnosticCode::InvalidInstruction,
                    line,
                    "XLIL const.bool immediate is invalid");
        return None;
      }
    };
    function.values.push(Value { id: result,
                                 value_type: Type::BOOL });
    Some(Instruction::ConstBool { result,
                                  value })
  }

  fn value_call(&mut self, function: &mut Function, result: &str, call: &str, line: usize) -> Option<Instruction>
  {
    let Some((result, return_type)) = result.split_once(':')
    else
    {
      self.report(DiagnosticCode::InvalidInstruction, line, "XLIL call result is invalid");
      return None;
    };
    let result = self.value_id(result, line)?;
    let return_type = self.type_name(return_type, line)?;
    if return_type == Type::VOID
    {
      self.report(DiagnosticCode::InvalidInstruction,
                  line,
                  "XLIL value call result cannot have void type");
      return None;
    }
    let (function_name, arguments) = self.call_operands(call, line)?;
    function.values.push(Value { id: result,
                                 value_type: return_type });
    Some(Instruction::Call { result: Some(result),
                             function: function_name,
                             arguments,
                             return_type })
  }

  fn void_call(&mut self, _function: &mut Function, call: &str, line: usize) -> Option<Instruction>
  {
    let (function_name, arguments) = self.call_operands(call, line)?;
    Some(Instruction::Call { result: None,
                             function: function_name,
                             arguments,
                             return_type: Type::VOID })
  }

  fn call_operands(&mut self, call: &str, line: usize) -> Option<(String, Vec<ValueId>)>
  {
    let Some((function, arguments)) = call.split_once('(')
    else
    {
      self.report(DiagnosticCode::InvalidInstruction,
                  line,
                  "XLIL call operands are invalid");
      return None;
    };
    let Some(arguments) = arguments.strip_suffix(')')
    else
    {
      self.report(DiagnosticCode::InvalidInstruction,
                  line,
                  "XLIL call arguments are invalid");
      return None;
    };
    let mut parsed = Vec::new();
    if !arguments.is_empty()
    {
      for argument in arguments.split(", ")
      {
        parsed.push(self.value_operand(argument, line)?);
      }
    }
    Some((function.to_string(), parsed))
  }

  fn value_operand(&mut self, text: &str, line: usize) -> Option<ValueId>
  {
    let Some(value) = text.strip_prefix('%')
    else
    {
      self.report(DiagnosticCode::InvalidInstruction,
                  line,
                  "XLIL value operand is invalid");
      return None;
    };
    self.value_id(value, line)
  }

  fn return_terminator(&mut self, text: &str, line: usize) -> Option<Terminator>
  {
    if text.is_empty()
    {
      return Some(Terminator::Return(None));
    }
    let Some(value) = text.strip_prefix(" %")
    else
    {
      self.report(DiagnosticCode::InvalidTerminator,
                  line,
                  "XLIL return terminator is invalid");
      return None;
    };
    Some(Terminator::Return(Some(self.value_id(value, line)?)))
  }

  fn branch_terminator(&mut self, text: &str, line: usize) -> Option<Terminator>
  {
    let Some(target) = text.strip_prefix("bb")
    else
    {
      self.report(DiagnosticCode::InvalidTerminator,
                  line,
                  "XLIL branch terminator is invalid");
      return None;
    };
    let Some(target) = target.parse::<u32>().ok().map(BlockId)
    else
    {
      self.report(DiagnosticCode::InvalidTerminator, line, "XLIL branch target is invalid");
      return None;
    };
    Some(Terminator::Branch(target))
  }

  fn branch_if_terminator(&mut self, text: &str, line: usize) -> Option<Terminator>
  {
    let Some((condition, targets)) = text.split_once(", ")
    else
    {
      self.report(DiagnosticCode::InvalidTerminator,
                  line,
                  "XLIL br_if terminator is invalid");
      return None;
    };
    let condition = self.value_operand(condition, line)?;
    let Some((then_block, else_block)) = targets.split_once(", ")
    else
    {
      self.report(DiagnosticCode::InvalidTerminator,
                  line,
                  "XLIL br_if targets are invalid");
      return None;
    };
    Some(Terminator::BranchIf { condition,
                                then_block: self.branch_target(then_block, line)?,
                                else_block: self.branch_target(else_block, line)? })
  }

  fn branch_target(&mut self, text: &str, line: usize) -> Option<BlockId>
  {
    let Some(target) = text.strip_prefix("bb")
    else
    {
      self.report(DiagnosticCode::InvalidTerminator, line, "XLIL branch target is invalid");
      return None;
    };
    let parsed = target.parse::<u32>().ok().map(BlockId);
    if parsed.is_none()
    {
      self.report(DiagnosticCode::InvalidTerminator, line, "XLIL branch target is invalid");
    }
    parsed
  }

  fn value_id(&mut self, text: &str, line: usize) -> Option<ValueId>
  {
    let parsed = text.strip_prefix('r')
                     .and_then(|text| text.parse::<u32>().ok())
                     .map(ValueId);
    if parsed.is_none()
    {
      self.report(DiagnosticCode::InvalidValueId, line, "XLIL value id is invalid");
    }
    parsed
  }

  fn current_line(&self) -> Option<&str>
  {
    self.lines.get(self.cursor).copied()
  }

  fn next_line(&mut self) -> Option<String>
  {
    let line = self.current_line()?.to_string();
    self.cursor += 1;
    Some(line)
  }

  fn line_number(&self) -> usize
  {
    self.cursor + 1
  }

  fn report(&mut self, code: DiagnosticCode, line: usize, message: &str)
  {
    self.diagnostics.push(Diagnostic { code,
                                       line,
                                       message: message.to_string() });
  }
}

#[cfg(test)]
mod tests
{
  use super::*;
  use crate::xlil::writer::module_to_string;

  #[test]
  fn parses_function_declaration()
  {
    let module =
      parse_module(".xlil version 0\n.xlil module App\n.extern xs$App$External : (i64) -> i64\n")
        .expect("parse should succeed");

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
    let text =
      ".xlil version 0\n.xlil module App\n.func xs$App$Value : () -> i64\nbb0.entry:\n  %r0:i64 = const 42\n  ret \
       %r0\n.end\n";

    let module = parse_module(text).expect("parse should succeed");

    assert_eq!(module_to_string(&module), text);
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
  fn roundtrips_const_bool_function()
  {
    let text = ".xlil version 0\n.xlil module App\n.func xs$App$Truth : () -> bool\nbb0.entry:\n  %r0:bool = \
                const.bool true\n  ret %r0\n.end\n";

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
  fn rejects_legacy_plain_value_ids()
  {
    let text =
      ".xlil version 0\n.xlil module App\n.func xs$App$Legacy : () -> i64\nbb0.entry:\n  %0:i64 = const 42\n  ret \
       %0\n.end\n";

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
    let text = ".xlil version 0\n.xlil module App\n.func xs$App$Branch : () -> void\nbb0.entry:\n  br \
                bb1\nbb1.exit:\n  ret\n.end\n";

    let module = parse_module(text).expect("parse should succeed");

    assert_eq!(module_to_string(&module), text);
  }
}
