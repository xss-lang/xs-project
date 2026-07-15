/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use std::fmt::Write;

use super::match_model::MatchPattern;
use super::result_desugar::{DesugaredBlock, DesugaredExpression, DesugaredFunction, DesugaredStatement};
use super::symbols::{Import, Module, SymbolKind, Visibility};
use super::type_check::{
  BinaryOperator, Block, Expression, Function, Literal, PrimitiveType, Statement, Type, UnaryOperator, UpdateOperator,
  UpdatePosition,
};
use super::type_check::{Diagnostic as TypeDiagnostic, DiagnosticCode as TypeDiagnosticCode};

pub mod parser;

mod block_writer;
#[cfg(test)]
mod call_tests;
#[cfg(test)]
mod collection_tests;
#[cfg(test)]
mod conditional_tests;
#[cfg(test)]
mod for_tests;
#[cfg(test)]
mod match_tests;
#[cfg(test)]
mod nominal_tests;
mod statement_writer;

use block_writer::{mutability_name, write_block, write_desugared_block};
pub use parser::{XhirParseDiagnostic, parse_xhir_function, parse_xhir_module_symbols};
use statement_writer::{write_desugared_statement, write_statement};

pub const SUPPORTED_XHIR_VERSION: u32 = 0;

#[must_use]
pub const fn is_supported_xhir_version(version: u32) -> bool
{
  matches!(version, SUPPORTED_XHIR_VERSION)
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum XhirDocumentKind
{
  Module,
  Function,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct XhirDocumentHeader
{
  pub version: u32,
  pub kind: XhirDocumentKind,
  pub name: String,
}

#[must_use]
pub fn parse_xhir_header(text: &str) -> Option<XhirDocumentHeader>
{
  let mut lines = text.lines();
  let version = lines.next()?.strip_prefix(".xhir version ")?.parse().ok()?;
  let declaration = lines.next()?;
  if let Some(name) = declaration.strip_prefix("module ")
  {
    return Some(XhirDocumentHeader { version,
                                     kind: XhirDocumentKind::Module,
                                     name: name.to_string() });
  }
  if let Some(name) = declaration.strip_prefix("function ")
  {
    return Some(XhirDocumentHeader { version,
                                     kind: XhirDocumentKind::Function,
                                     name: name.to_string() });
  }
  None
}

#[must_use]
pub fn module_symbols_to_xhir(module: &Module) -> String
{
  let mut output = String::new();
  let _ = writeln!(output, ".xhir version 0");
  let _ = writeln!(output, "module {}", module.name);
  if !module.imports.is_empty()
  {
    let _ = writeln!(output);
    let _ = writeln!(output, "imports");
    for import in &module.imports
    {
      write_import(&mut output, import);
    }
    let _ = writeln!(output, ".end");
  }
  if !module.symbols.is_empty()
  {
    let _ = writeln!(output);
    let _ = writeln!(output, "declarations");
    for symbol in &module.symbols
    {
      let _ = writeln!(output, "  symbol {}", symbol.name);
      let _ = writeln!(output, "    kind {}", symbol_kind_name(symbol.kind));
      let _ = writeln!(output, "    visibility {}", visibility_name(symbol.visibility));
    }
    let _ = writeln!(output, ".end");
  }
  let _ = writeln!(output, ".program end");
  output
}

#[must_use]
pub fn function_to_xhir(function: &Function) -> String
{
  let mut output = String::new();
  let _ = writeln!(output, ".xhir version 0");
  let _ = writeln!(output, "function {}", function.name);
  let _ = writeln!(output, "  signature");
  match &function.return_type
  {
    Some(return_type) =>
    {
      let _ = writeln!(output, "    returns {}", type_name(return_type));
    }
    None =>
    {
      let _ = writeln!(output, "    returns void");
    }
  }
  let _ = writeln!(output, "  .end");
  if !function.locals.is_empty()
  {
    let _ = writeln!(output, "  locals");
    for local in &function.locals
    {
      let mutability = if local.mutable
      {
        "mutable"
      }
      else
      {
        "immutable"
      };
      let _ = writeln!(output,
                       "    local {}: {} {}",
                       local.name,
                       type_name(&local.ty),
                       mutability);
    }
    let _ = writeln!(output, "  .end");
  }
  if !function.body.is_empty()
  {
    let _ = writeln!(output, "  body");
    for statement in &function.body
    {
      write_statement(&mut output, statement, 2);
    }
    let _ = writeln!(output, "  .end");
  }
  let _ = writeln!(output, ".program end");
  output
}

#[must_use]
pub fn desugared_function_to_xhir(function: &DesugaredFunction) -> String
{
  let mut output = String::new();
  write_function_header(&mut output, &function.name, function.return_type.as_ref());
  if !function.locals.is_empty()
  {
    write_locals(&mut output, &function.locals);
  }
  if !function.body.is_empty()
  {
    let _ = writeln!(output, "  body");
    for statement in &function.body
    {
      write_desugared_statement(&mut output, statement, 2);
    }
    let _ = writeln!(output, "  .end");
  }
  let _ = writeln!(output, ".program end");
  output
}

#[must_use]
pub fn typecheck_analysis_to_xhir(diagnostics: &[TypeDiagnostic]) -> String
{
  let mut output = String::new();
  let _ = writeln!(output, "analysis typecheck");
  for diagnostic in diagnostics
  {
    let _ = writeln!(output, "  diagnostic {}", type_diagnostic_code_name(&diagnostic.code));
    let _ = writeln!(output,
                     "    span {}:{}..{}",
                     diagnostic.span.file_id, diagnostic.span.start, diagnostic.span.end);
    let _ = writeln!(output, "    message {}", diagnostic.message);
  }
  output
}

pub fn parse_xhir_typecheck_analysis(text: &str) -> Result<Vec<TypeDiagnostic>, Vec<XhirParseDiagnostic>>
{
  let mut lines = text.lines().enumerate();
  let Some((_, "analysis typecheck")) = lines.next()
  else
  {
    let message = "expected typecheck analysis section".to_string();
    return Err(vec![XhirParseDiagnostic { line: 1,
                                          message }]);
  };
  let mut parsed = Vec::new();
  let mut diagnostics = Vec::new();
  while let Some((line_index, line)) = lines.next()
  {
    let Some(code_name) = line.strip_prefix("  diagnostic ")
    else
    {
      let message = format!("expected typecheck diagnostic record, found '{line}'");
      diagnostics.push(XhirParseDiagnostic { line: line_index + 1,
                                             message });
      continue;
    };
    let code = parse_type_diagnostic_code(code_name, line_index + 1, &mut diagnostics);
    let span = parse_analysis_span(&mut lines, &mut diagnostics);
    let message = parse_analysis_message(&mut lines, &mut diagnostics);
    if let (Some(code), Some(span), Some(message)) = (code, span, message)
    {
      parsed.push(TypeDiagnostic { code,
                                   message,
                                   span });
    }
  }
  if diagnostics.is_empty()
  {
    Ok(parsed)
  }
  else
  {
    Err(diagnostics)
  }
}

fn write_import(output: &mut String, import: &Import)
{
  match import
  {
    Import::Module { module, .. } =>
    {
      let _ = writeln!(output, "  import module {module}");
    }
    Import::All { module, .. } =>
    {
      let _ = writeln!(output, "  import all from {module}");
    }
    Import::Selected { module,
                       name,
                       alias,
                       .. } =>
    {
      if let Some(alias) = alias
      {
        let _ = writeln!(output, "  import {name} as {alias} from {module}");
      }
      else
      {
        let _ = writeln!(output, "  import {name} from {module}");
      }
    }
  }
}

fn type_diagnostic_code_name(code: &TypeDiagnosticCode) -> &'static str
{
  match code
  {
    TypeDiagnosticCode::LiteralTypeMismatch => "literal_type_mismatch",
    TypeDiagnosticCode::ImmutableAssignment => "immutable_assignment",
    TypeDiagnosticCode::UnknownLocal => "unknown_local",
    TypeDiagnosticCode::DuplicateLocal => "duplicate_local",
    TypeDiagnosticCode::BinaryTypeMismatch => "binary_type_mismatch",
    TypeDiagnosticCode::UnaryTypeMismatch => "unary_type_mismatch",
    TypeDiagnosticCode::ResultPropagationRequiresResult => "result_propagation_requires_result",
    TypeDiagnosticCode::ResultPropagationReturnMismatch => "result_propagation_return_mismatch",
    TypeDiagnosticCode::ConditionTypeMismatch => "condition_type_mismatch",
    TypeDiagnosticCode::MissingBlockValue => "missing_block_value",
    TypeDiagnosticCode::BreakOutsideLoop => "break_outside_loop",
    TypeDiagnosticCode::ContinueOutsideLoop => "continue_outside_loop",
    TypeDiagnosticCode::MatchRequiresFinalElse => "match_requires_final_else",
    TypeDiagnosticCode::DuplicateMatchPattern => "duplicate_match_pattern",
    TypeDiagnosticCode::UnknownNominalType => "unknown_nominal_type",
    TypeDiagnosticCode::UnknownField => "unknown_field",
    TypeDiagnosticCode::MissingField => "missing_field",
    TypeDiagnosticCode::DuplicateField => "duplicate_field",
  }
}

fn parse_type_diagnostic_code(name: &str,
                              line: usize,
                              diagnostics: &mut Vec<XhirParseDiagnostic>)
                              -> Option<TypeDiagnosticCode>
{
  match name
  {
    "literal_type_mismatch" => Some(TypeDiagnosticCode::LiteralTypeMismatch),
    "immutable_assignment" => Some(TypeDiagnosticCode::ImmutableAssignment),
    "unknown_local" => Some(TypeDiagnosticCode::UnknownLocal),
    "duplicate_local" => Some(TypeDiagnosticCode::DuplicateLocal),
    "binary_type_mismatch" => Some(TypeDiagnosticCode::BinaryTypeMismatch),
    "unary_type_mismatch" => Some(TypeDiagnosticCode::UnaryTypeMismatch),
    "result_propagation_requires_result" => Some(TypeDiagnosticCode::ResultPropagationRequiresResult),
    "result_propagation_return_mismatch" => Some(TypeDiagnosticCode::ResultPropagationReturnMismatch),
    "condition_type_mismatch" => Some(TypeDiagnosticCode::ConditionTypeMismatch),
    "missing_block_value" => Some(TypeDiagnosticCode::MissingBlockValue),
    "break_outside_loop" => Some(TypeDiagnosticCode::BreakOutsideLoop),
    "continue_outside_loop" => Some(TypeDiagnosticCode::ContinueOutsideLoop),
    "match_requires_final_else" => Some(TypeDiagnosticCode::MatchRequiresFinalElse),
    "duplicate_match_pattern" => Some(TypeDiagnosticCode::DuplicateMatchPattern),
    "unknown_nominal_type" => Some(TypeDiagnosticCode::UnknownNominalType),
    "unknown_field" => Some(TypeDiagnosticCode::UnknownField),
    "missing_field" => Some(TypeDiagnosticCode::MissingField),
    "duplicate_field" => Some(TypeDiagnosticCode::DuplicateField),
    _ =>
    {
      diagnostics.push(XhirParseDiagnostic { line,
                                             message: format!("unknown typecheck diagnostic code '{name}'") });
      None
    }
  }
}

fn parse_analysis_span<'a>(lines: &mut impl Iterator<Item = (usize, &'a str)>,
                           diagnostics: &mut Vec<XhirParseDiagnostic>)
                           -> Option<crate::hir::async_check::Span>
{
  let Some((line_index, line)) = lines.next()
  else
  {
    diagnostics.push(XhirParseDiagnostic { line: 1,
                                           message: "missing diagnostic span".to_string() });
    return None;
  };
  let Some(text) = line.strip_prefix("    span ")
  else
  {
    diagnostics.push(XhirParseDiagnostic { line: line_index + 1,
                                           message: "expected diagnostic span".to_string() });
    return None;
  };
  parse_span(text).or_else(|| {
                    diagnostics.push(XhirParseDiagnostic { line: line_index + 1,
                                                           message: format!("invalid diagnostic span '{text}'") });
                    None
                  })
}

fn parse_analysis_message<'a>(lines: &mut impl Iterator<Item = (usize, &'a str)>,
                              diagnostics: &mut Vec<XhirParseDiagnostic>)
                              -> Option<String>
{
  let Some((line_index, line)) = lines.next()
  else
  {
    diagnostics.push(XhirParseDiagnostic { line: 1,
                                           message: "missing diagnostic message".to_string() });
    return None;
  };
  let Some(message) = line.strip_prefix("    message ")
  else
  {
    diagnostics.push(XhirParseDiagnostic { line: line_index + 1,
                                           message: "expected diagnostic message".to_string() });
    return None;
  };
  Some(message.to_string())
}

fn parse_span(text: &str) -> Option<crate::hir::async_check::Span>
{
  let (file_id, rest) = text.split_once(':')?;
  let (start, end) = rest.split_once("..")?;
  Some(crate::hir::async_check::Span::new(file_id.parse().ok()?, start.parse().ok()?, end.parse().ok()?))
}

fn write_expression(output: &mut String, expression: &Expression, indent: usize)
{
  let pad = "  ".repeat(indent);
  match expression
  {
    Expression::Literal { literal, .. } =>
    {
      let _ = writeln!(output, "{pad}literal {}", literal_name(literal));
    }
    Expression::Local { name, .. } =>
    {
      let _ = writeln!(output, "{pad}local {name}");
    }
    Expression::Field { path } =>
    {
      let mutability = if path.mutable
      {
        "mutable"
      }
      else
      {
        "immutable"
      };
      let _ = writeln!(output,
                       "{pad}field {mutability} {} : {}",
                       field_path_name(path),
                       type_name(&path.ty));
    }
    Expression::Object { nominal_type,
                         fields,
                         .. } =>
    {
      let _ = writeln!(output, "{pad}object {nominal_type}");
      for field in fields
      {
        let _ = writeln!(output, "{pad}  field {}", field.name);
        write_expression(output, &field.value, indent + 2);
      }
      let _ = writeln!(output, "{pad}.end");
    }
    Expression::Array { elements, .. } =>
    {
      let _ = writeln!(output, "{pad}array");
      for element in elements
      {
        let _ = writeln!(output, "{pad}  element");
        write_expression(output, element, indent + 2);
      }
      let _ = writeln!(output, "{pad}.end");
    }
    Expression::Map { entries, .. } =>
    {
      let _ = writeln!(output, "{pad}map");
      for entry in entries
      {
        let _ = writeln!(output, "{pad}  entry");
        let _ = writeln!(output, "{pad}    key");
        write_expression(output, &entry.key, indent + 3);
        let _ = writeln!(output, "{pad}    value");
        write_expression(output, &entry.value, indent + 3);
      }
      let _ = writeln!(output, "{pad}.end");
    }
    Expression::Assign { target,
                         value,
                         .. } =>
    {
      let _ = writeln!(output, "{pad}assign {target}");
      write_expression(output, value, indent + 1);
    }
    Expression::AssignField { target,
                              value,
                              .. } =>
    {
      let mutability = if target.mutable
      {
        "mutable"
      }
      else
      {
        "immutable"
      };
      let _ = writeln!(output,
                       "{pad}assign_field {mutability} {} : {}",
                       field_path_name(target),
                       type_name(&target.ty));
      write_expression(output, value, indent + 1);
    }
    Expression::Update { target,
                         operator,
                         position,
                         .. } =>
    {
      let _ = writeln!(output,
                       "{pad}update {} {} {target}",
                       update_position_name(*position),
                       update_operator_name(*operator));
    }
    Expression::Binary { operator,
                         left,
                         right,
                         .. } =>
    {
      let _ = writeln!(output, "{pad}binary {}", binary_operator_name(*operator));
      let _ = writeln!(output, "{pad}  left");
      write_expression(output, left, indent + 2);
      let _ = writeln!(output, "{pad}  right");
      write_expression(output, right, indent + 2);
    }
    Expression::Unary { operator,
                        operand,
                        .. } =>
    {
      let _ = writeln!(output, "{pad}unary {}", unary_operator_name(*operator));
      let _ = writeln!(output, "{pad}  operand");
      write_expression(output, operand, indent + 2);
    }
    Expression::ResultPropagation { value, .. } =>
    {
      let _ = writeln!(output, "{pad}propagate");
      write_expression(output, value, indent + 1);
    }
    Expression::Call { function,
                       arguments,
                       parameter_types,
                       return_type,
                       .. } =>
    {
      let parameters = parameter_types.iter().map(type_name).collect::<Vec<_>>().join(", ");
      let _ = writeln!(output,
                       "{pad}call {function} : ({parameters}) -> {}",
                       type_name(return_type));
      for argument in arguments
      {
        let _ = writeln!(output, "{pad}  argument");
        write_expression(output, argument, indent + 2);
      }
    }
    Expression::If { condition,
                     then_block,
                     else_block,
                     result_type,
                     .. } =>
    {
      let _ = writeln!(output, "{pad}if_expression {}", type_name(result_type));
      let _ = writeln!(output, "{pad}  condition");
      write_expression(output, condition, indent + 2);
      let _ = writeln!(output, "{pad}  then");
      write_block(output, then_block, indent + 2);
      let _ = writeln!(output, "{pad}  else");
      write_block(output, else_block, indent + 2);
    }
    Expression::Match { selector,
                        selector_type,
                        arms,
                        result_type,
                        .. } =>
    {
      let _ = writeln!(output,
                       "{pad}match_expression {} selector {}",
                       type_name(result_type),
                       type_name(selector_type));
      let _ = writeln!(output, "{pad}  selector");
      write_expression(output, selector, indent + 2);
      for arm in arms
      {
        match &arm.pattern
        {
          MatchPattern::Literal(literal) =>
          {
            let _ = writeln!(output, "{pad}  arm literal {}", literal_name(literal));
          }
          MatchPattern::Else =>
          {
            let _ = writeln!(output, "{pad}  arm else");
          }
        }
        let _ = writeln!(output, "{pad}    body");
        write_block(output, &arm.body, indent + 3);
      }
      let _ = writeln!(output, "{pad}.end");
    }
  }
}

fn write_desugared_expression(output: &mut String, expression: &DesugaredExpression, indent: usize)
{
  let pad = "  ".repeat(indent);
  match expression
  {
    DesugaredExpression::Literal { literal, .. } =>
    {
      let _ = writeln!(output, "{pad}literal {}", literal_name(literal));
    }
    DesugaredExpression::Local { name, .. } =>
    {
      let _ = writeln!(output, "{pad}local {name}");
    }
    DesugaredExpression::Field { path } =>
    {
      let mutability = if path.mutable
      {
        "mutable"
      }
      else
      {
        "immutable"
      };
      let _ = writeln!(output,
                       "{pad}field {mutability} {} : {}",
                       field_path_name(path),
                       type_name(&path.ty));
    }
    DesugaredExpression::Object { nominal_type,
                                  fields,
                                  .. } =>
    {
      let _ = writeln!(output, "{pad}object {nominal_type}");
      for field in fields
      {
        let _ = writeln!(output, "{pad}  field {}", field.name);
        write_desugared_expression(output, &field.value, indent + 2);
      }
      let _ = writeln!(output, "{pad}.end");
    }
    DesugaredExpression::Array { elements, .. } =>
    {
      let _ = writeln!(output, "{pad}array");
      for element in elements
      {
        let _ = writeln!(output, "{pad}  element");
        write_desugared_expression(output, element, indent + 2);
      }
      let _ = writeln!(output, "{pad}.end");
    }
    DesugaredExpression::Map { entries, .. } =>
    {
      let _ = writeln!(output, "{pad}map");
      for entry in entries
      {
        let _ = writeln!(output, "{pad}  entry");
        let _ = writeln!(output, "{pad}    key");
        write_desugared_expression(output, &entry.key, indent + 3);
        let _ = writeln!(output, "{pad}    value");
        write_desugared_expression(output, &entry.value, indent + 3);
      }
      let _ = writeln!(output, "{pad}.end");
    }
    DesugaredExpression::Assign { target,
                                  value,
                                  .. } =>
    {
      let _ = writeln!(output, "{pad}assign {target}");
      write_desugared_expression(output, value, indent + 1);
    }
    DesugaredExpression::AssignField { target,
                                       value,
                                       .. } =>
    {
      let mutability = if target.mutable
      {
        "mutable"
      }
      else
      {
        "immutable"
      };
      let _ = writeln!(output,
                       "{pad}assign_field {mutability} {} : {}",
                       field_path_name(target),
                       type_name(&target.ty));
      write_desugared_expression(output, value, indent + 1);
    }
    DesugaredExpression::Update { target,
                                  operator,
                                  position,
                                  .. } =>
    {
      let _ = writeln!(output,
                       "{pad}update {} {} {target}",
                       update_position_name(*position),
                       update_operator_name(*operator));
    }
    DesugaredExpression::Binary { operator,
                                  left,
                                  right,
                                  .. } =>
    {
      let _ = writeln!(output, "{pad}binary {}", binary_operator_name(*operator));
      let _ = writeln!(output, "{pad}  left");
      write_desugared_expression(output, left, indent + 2);
      let _ = writeln!(output, "{pad}  right");
      write_desugared_expression(output, right, indent + 2);
    }
    DesugaredExpression::Unary { operator,
                                 operand,
                                 .. } =>
    {
      let _ = writeln!(output, "{pad}unary {}", unary_operator_name(*operator));
      let _ = writeln!(output, "{pad}  operand");
      write_desugared_expression(output, operand, indent + 2);
    }
    DesugaredExpression::ResultMatch { value,
                                       success_binding,
                                       error_binding,
                                       success_type,
                                       error_type,
                                       .. } =>
    {
      let _ = writeln!(output, "{pad}result_match");
      let _ = writeln!(output, "{pad}  ok {success_binding}: {}", type_name(success_type));
      let _ = writeln!(output, "{pad}  error {error_binding}: {}", type_name(error_type));
      let _ = writeln!(output, "{pad}  value");
      write_desugared_expression(output, value, indent + 2);
    }
    DesugaredExpression::Call { function,
                                arguments,
                                parameter_types,
                                return_type,
                                .. } =>
    {
      let parameters = parameter_types.iter().map(type_name).collect::<Vec<_>>().join(", ");
      let _ = writeln!(output,
                       "{pad}call {function} : ({parameters}) -> {}",
                       type_name(return_type));
      for argument in arguments
      {
        let _ = writeln!(output, "{pad}  argument");
        write_desugared_expression(output, argument, indent + 2);
      }
    }
    DesugaredExpression::If { condition,
                              then_block,
                              else_block,
                              result_type,
                              .. } =>
    {
      let _ = writeln!(output, "{pad}if_expression {}", type_name(result_type));
      let _ = writeln!(output, "{pad}  condition");
      write_desugared_expression(output, condition, indent + 2);
      let _ = writeln!(output, "{pad}  then");
      write_desugared_block(output, then_block, indent + 2);
      let _ = writeln!(output, "{pad}  else");
      write_desugared_block(output, else_block, indent + 2);
    }
    DesugaredExpression::Match { selector,
                                 selector_type,
                                 arms,
                                 result_type,
                                 .. } =>
    {
      let _ = writeln!(output,
                       "{pad}match_expression {} selector {}",
                       type_name(result_type),
                       type_name(selector_type));
      let _ = writeln!(output, "{pad}  selector");
      write_desugared_expression(output, selector, indent + 2);
      for arm in arms
      {
        match &arm.pattern
        {
          MatchPattern::Literal(literal) =>
          {
            let _ = writeln!(output, "{pad}  arm literal {}", literal_name(literal));
          }
          MatchPattern::Else =>
          {
            let _ = writeln!(output, "{pad}  arm else");
          }
        }
        let _ = writeln!(output, "{pad}    body");
        write_desugared_block(output, &arm.body, indent + 3);
      }
      let _ = writeln!(output, "{pad}.end");
    }
  }
}

fn write_function_header(output: &mut String, name: &str, return_type: Option<&Type>)
{
  let _ = writeln!(output, ".xhir version 0");
  let _ = writeln!(output, "function {name}");
  let _ = writeln!(output, "  signature");
  match return_type
  {
    Some(return_type) =>
    {
      let _ = writeln!(output, "    returns {}", type_name(return_type));
    }
    None =>
    {
      let _ = writeln!(output, "    returns void");
    }
  }
  let _ = writeln!(output, "  .end");
}

fn write_locals(output: &mut String, locals: &[super::type_check::Local])
{
  let _ = writeln!(output, "  locals");
  for local in locals
  {
    let _ = writeln!(output,
                     "    local {}: {} {}",
                     local.name,
                     type_name(&local.ty),
                     mutability_name(local.mutable));
  }
  let _ = writeln!(output, "  .end");
}

const fn binary_operator_name(operator: BinaryOperator) -> &'static str
{
  match operator
  {
    BinaryOperator::Add => "add",
    BinaryOperator::Sub => "sub",
    BinaryOperator::Mul => "mul",
    BinaryOperator::Div => "div",
    BinaryOperator::Rem => "rem",
    BinaryOperator::BitAnd => "bit_and",
    BinaryOperator::BitOr => "bit_or",
    BinaryOperator::BitXor => "bit_xor",
    BinaryOperator::LogicalAnd => "logical_and",
    BinaryOperator::LogicalOr => "logical_or",
    BinaryOperator::ShiftLeft => "shift_left",
    BinaryOperator::ShiftRight => "shift_right",
    BinaryOperator::Equal => "eq",
    BinaryOperator::NotEqual => "ne",
    BinaryOperator::Less => "lt",
    BinaryOperator::LessEqual => "le",
    BinaryOperator::Greater => "gt",
    BinaryOperator::GreaterEqual => "ge",
  }
}

const fn unary_operator_name(operator: UnaryOperator) -> &'static str
{
  match operator
  {
    UnaryOperator::LogicalNot => "logical_not",
    UnaryOperator::Positive => "positive",
    UnaryOperator::Negative => "negative",
  }
}

const fn update_operator_name(operator: UpdateOperator) -> &'static str
{
  match operator
  {
    UpdateOperator::Increment => "increment",
    UpdateOperator::Decrement => "decrement",
  }
}

const fn update_position_name(position: UpdatePosition) -> &'static str
{
  match position
  {
    UpdatePosition::Prefix => "prefix",
    UpdatePosition::Postfix => "postfix",
  }
}

fn literal_name(literal: &Literal) -> String
{
  match literal
  {
    Literal::Bool(value) => format!("bool {value}"),
    Literal::Integer(value) => format!("integer {value}"),
    Literal::Float(value) => format!("float {value}"),
    Literal::Char(value) => format!("char {}", crate::text_literal::format_character(*value)),
    Literal::String(value) => format!("string {value:?}"),
    Literal::None => "None".to_string(),
  }
}

fn type_name(ty: &Type) -> String
{
  match ty
  {
    Type::Unit => "()".to_string(),
    Type::Primitive(primitive) => primitive_type_name(*primitive).to_string(),
    Type::Named(name) => name.clone(),
    Type::Array { element,
                  length: None, } => format!("[{}]", type_name(element)),
    Type::Array { element,
                  length: Some(length), } => format!("[{}; {length}]", type_name(element)),
    Type::Map { key,
                value, } => format!("[{}: {}]", type_name(key), type_name(value)),
  }
}

fn field_path_name(path: &crate::hir::type_check::FieldPath) -> String
{
  std::iter::once(path.root.as_str()).chain(path.fields.iter().map(String::as_str))
                                     .collect::<Vec<_>>()
                                     .join(".")
}

const fn primitive_type_name(primitive: PrimitiveType) -> &'static str
{
  match primitive
  {
    PrimitiveType::Bool => "Bool",
    PrimitiveType::Byte => "Byte",
    PrimitiveType::SByte => "SByte",
    PrimitiveType::Char => "Char",
    PrimitiveType::Short => "Short",
    PrimitiveType::Long => "Long",
    PrimitiveType::Int => "Int",
    PrimitiveType::Integer => "Integer",
    PrimitiveType::UShort => "UShort",
    PrimitiveType::ULong => "ULong",
    PrimitiveType::UInt => "UInt",
    PrimitiveType::UInteger => "UInteger",
    PrimitiveType::SFloat => "SFloat",
    PrimitiveType::Float => "Float",
    PrimitiveType::Str => "Str",
  }
}

const fn symbol_kind_name(kind: SymbolKind) -> &'static str
{
  match kind
  {
    SymbolKind::Function => "function",
    SymbolKind::Class => "class",
    SymbolKind::Interface => "interface",
    SymbolKind::Enum => "enum",
    SymbolKind::Data => "data",
    SymbolKind::Macro => "macro",
  }
}

const fn visibility_name(visibility: Visibility) -> &'static str
{
  match visibility
  {
    Visibility::Public => "public",
    Visibility::Internal => "internal",
    Visibility::Private => "private",
  }
}

#[cfg(test)]
mod tests;
