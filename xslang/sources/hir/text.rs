/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use std::fmt::Write;

use super::match_model::MatchPattern;
use super::result_desugar::{DesugaredBlock, DesugaredExpression, DesugaredFunction, DesugaredStatement};
use super::symbols::{Import, Module, SymbolKind, Visibility};
use super::type_check::{BinaryOperator, Block, Expression, Function, Literal, PrimitiveType, Statement, Type};
use super::type_check::{Diagnostic as TypeDiagnostic, DiagnosticCode as TypeDiagnosticCode};

pub mod parser;

mod block_writer;
#[cfg(test)]
mod call_tests;
#[cfg(test)]
mod conditional_tests;
#[cfg(test)]
mod for_tests;
#[cfg(test)]
mod match_tests;
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
    TypeDiagnosticCode::ResultPropagationRequiresResult => "result_propagation_requires_result",
    TypeDiagnosticCode::ResultPropagationReturnMismatch => "result_propagation_return_mismatch",
    TypeDiagnosticCode::ConditionTypeMismatch => "condition_type_mismatch",
    TypeDiagnosticCode::MissingBlockValue => "missing_block_value",
    TypeDiagnosticCode::BreakOutsideLoop => "break_outside_loop",
    TypeDiagnosticCode::ContinueOutsideLoop => "continue_outside_loop",
    TypeDiagnosticCode::MatchRequiresFinalElse => "match_requires_final_else",
    TypeDiagnosticCode::DuplicateMatchPattern => "duplicate_match_pattern",
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
    "result_propagation_requires_result" => Some(TypeDiagnosticCode::ResultPropagationRequiresResult),
    "result_propagation_return_mismatch" => Some(TypeDiagnosticCode::ResultPropagationReturnMismatch),
    "condition_type_mismatch" => Some(TypeDiagnosticCode::ConditionTypeMismatch),
    "missing_block_value" => Some(TypeDiagnosticCode::MissingBlockValue),
    "break_outside_loop" => Some(TypeDiagnosticCode::BreakOutsideLoop),
    "continue_outside_loop" => Some(TypeDiagnosticCode::ContinueOutsideLoop),
    "match_requires_final_else" => Some(TypeDiagnosticCode::MatchRequiresFinalElse),
    "duplicate_match_pattern" => Some(TypeDiagnosticCode::DuplicateMatchPattern),
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
    Expression::Assign { target,
                         value,
                         .. } =>
    {
      let _ = writeln!(output, "{pad}assign {target}");
      write_expression(output, value, indent + 1);
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
    DesugaredExpression::Assign { target,
                                  value,
                                  .. } =>
    {
      let _ = writeln!(output, "{pad}assign {target}");
      write_desugared_expression(output, value, indent + 1);
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
    BinaryOperator::ShiftLeft => "shift_left",
    BinaryOperator::ShiftRight => "shift_right",
    BinaryOperator::Equal => "eq",
    BinaryOperator::Less => "lt",
    BinaryOperator::LessEqual => "le",
    BinaryOperator::Greater => "gt",
    BinaryOperator::GreaterEqual => "ge",
  }
}

fn literal_name(literal: &Literal) -> String
{
  match literal
  {
    Literal::Bool(value) => format!("bool {value}"),
    Literal::Integer(value) => format!("integer {value}"),
    Literal::Float(value) => format!("float {value}"),
    Literal::Char(value) => format!("char {value}"),
    Literal::String(value) => format!("string {value:?}"),
    Literal::None => "None".to_string(),
  }
}

fn type_name(ty: &Type) -> String
{
  match ty
  {
    Type::Primitive(primitive) => primitive_type_name(*primitive).to_string(),
    Type::Named(name) => name.clone(),
  }
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
mod tests
{
  use super::*;
  use crate::hir::async_check::Span;
  use crate::hir::symbols::{Symbol, SymbolKind};
  use crate::hir::type_check::{Diagnostic, DiagnosticCode, Local};

  fn span() -> Span
  {
    Span::new(1, 0, 1)
  }

  #[test]
  fn writes_symbol_module_as_structured_xhir()
  {
    let module = Module { name: "App".to_string(),
                          imports: vec![Import::Selected { module: "std.io".to_string(),
                                                           name: "println".to_string(),
                                                           alias: None,
                                                           span: span() }],
                          symbols: vec![Symbol { name: "Main".to_string(),
                                                 kind: SymbolKind::Function,
                                                 visibility: Visibility::Public,
                                                 span: span() }] };

    let text = module_symbols_to_xhir(&module);

    assert!(text.contains(".xhir version 0\nmodule App"));
    assert!(text.contains("import println from std.io"));
    assert!(text.contains("symbol Main\n    kind function\n    visibility public"));
    assert!(text.contains(".end\n.program end\n"));
    assert!(!text.contains(".func"));
    assert!(!text.contains("%0"));

    let header = parse_xhir_header(&text).expect("header should parse");
    assert_eq!(header.kind, XhirDocumentKind::Module);
    assert_eq!(header.name, "App");

    let parsed = parse_xhir_module_symbols(&text).expect("module symbols should parse");
    assert_eq!(parsed.name, "App");
    assert_eq!(parsed.imports.len(), 1);
    assert_eq!(parsed.symbols.len(), 1);
    assert_eq!(parsed.symbols[0].kind, SymbolKind::Function);
  }

  #[test]
  fn writes_checked_function_as_structured_xhir()
  {
    let function = Function { name: "Main".to_string(),
                              return_type: Some(Type::Primitive(PrimitiveType::Int)),
                              locals: vec![Local { name: "answer".to_string(),
                                                   ty: Type::Primitive(PrimitiveType::Int),
                                                   mutable: false,
                                                   span: span() }],
                              body: vec![Statement::Return { value: Some(Expression::Literal { literal:
                                                                                    Literal::Integer("42".to_string()),
                                                                                  span: span() }),
                                                span: span() }] };

    let text = function_to_xhir(&function);

    assert!(text.contains("function Main"));
    assert!(text.contains("returns Int"));
    assert!(text.contains("local answer: Int immutable"));
    assert!(text.contains("literal integer 42"));
    assert!(text.contains(".end\n.program end\n"));
    assert!(!text.contains("bb0"));

    let header = parse_xhir_header(&text).expect("header should parse");
    assert_eq!(header.kind, XhirDocumentKind::Function);
    assert_eq!(header.name, "Main");

    let parsed = parse_xhir_function(&text).expect("function should parse");
    assert_eq!(parsed.name, "Main");
    assert_eq!(parsed.return_type, Some(Type::Primitive(PrimitiveType::Int)));
    assert_eq!(parsed.locals.len(), 1);
    assert_eq!(parsed.body.len(), 1);
  }

  #[test]
  fn roundtrips_binary_expression()
  {
    let function = Function { name: "Compare".to_string(),
                              return_type: Some(Type::Primitive(PrimitiveType::Bool)),
                              locals: vec![Local { name: "left".to_string(),
                                                   ty: Type::Primitive(PrimitiveType::Long),
                                                   mutable: false,
                                                   span: span() },
                                           Local { name: "right".to_string(),
                                                   ty: Type::Primitive(PrimitiveType::Long),
                                                   mutable: false,
                                                   span: span() }],
                              body: vec![Statement::Return { value:
                                                                Some(Expression::Binary {
                                                                  operator: BinaryOperator::LessEqual,
                                                                  left: Box::new(Expression::Local {
                                                                    name: "left".to_string(),
                                                                    span: span(),
                                                                  }),
                                                                  right: Box::new(Expression::Local {
                                                                    name: "right".to_string(),
                                                                    span: span(),
                                                                  }),
                                                                  span: span(),
                                                                }),
                                                              span: span() }] };

    let text = function_to_xhir(&function);
    let parsed = parse_xhir_function(&text).expect("binary XHIR should parse");

    assert!(text.contains("binary le"));
    assert!(matches!(&parsed.body[0], Statement::Return { value:
                                                            Some(Expression::Binary { operator:
                                                                                        BinaryOperator::LessEqual,
                                                                                      .. }),
                                                          .. }));
  }

  #[test]
  fn roundtrips_result_propagation_expression()
  {
    let function = Function { name: "TryWork".to_string(),
                              return_type: None,
                              locals: vec![Local { name: "work".to_string(),
                                                   ty: Type::Primitive(PrimitiveType::Int),
                                                   mutable: false,
                                                   span: span() }],
                              body: vec![Statement::Expr(Expression::ResultPropagation {
                                value: Box::new(Expression::Local { name: "work".to_string(),
                                                                    span: span() }),
                                span: span(),
                              })] };

    let text = function_to_xhir(&function);
    let parsed = parse_xhir_function(&text).expect("propagation XHIR should parse");

    assert!(text.contains("propagate"));
    assert!(matches!(&parsed.body[0], Statement::Expr(Expression::ResultPropagation { .. })));
  }

  #[test]
  fn writes_desugared_result_match_as_structured_xhir()
  {
    let function = DesugaredFunction { name: "TryWork".to_string(),
                                       return_type: Some(Type::Named("Result<()>".to_string())),
                                       locals: vec![Local { name: "work".to_string(),
                                                            ty:
                                                              Type::Named("Result<Long, Result.Error>".to_string()),
                                                            mutable: false,
                                                            span: span() }],
                                       body: vec![DesugaredStatement::Expr(DesugaredExpression::ResultMatch {
        value: Box::new(DesugaredExpression::Local { name: "work".to_string(),
                                                     span: span() }),
        success_binding: "__xs_try_ok_0".to_string(),
        error_binding: "__xs_try_error_0".to_string(),
        success_type: Type::Primitive(PrimitiveType::Long),
        error_type: Type::Named("Result.Error".to_string()),
        span: span(),
      })] };

    let text = desugared_function_to_xhir(&function);

    assert!(text.contains("function TryWork"));
    assert!(text.contains("result_match"));
    assert!(text.contains("ok __xs_try_ok_0: Long"));
    assert!(text.contains("error __xs_try_error_0: Result.Error"));
    assert!(text.contains("value\n          local work"));
    assert!(!text.contains("propagate"));
  }

  #[test]
  fn roundtrips_panic_statement()
  {
    let function = Function { name: "Stop".to_string(),
                              return_type: None,
                              locals: vec![],
                              body: vec![Statement::Panic { span: span() }] };

    let text = function_to_xhir(&function);
    let parsed = parse_xhir_function(&text).expect("panic XHIR should parse");

    assert!(text.contains("panic"));
    assert!(matches!(parsed.body[0], Statement::Panic { .. }));
  }

  #[test]
  fn rejects_non_xhir_header()
  {
    assert_eq!(parse_xhir_header(".func Main : () -> void\n"), None);
  }

  #[test]
  fn rejects_unsupported_xhir_version()
  {
    let diagnostics =
      parse_xhir_function(".xhir version 1\nfunction Main\n  signature\n    returns void\n").expect_err("unsupported \
                                                                                                         XHIR version \
                                                                                                         must fail");

    assert!(diagnostics[0].message.contains("unsupported XHIR version 1"));
  }

  #[test]
  fn parses_explicit_end_markers_without_indentation()
  {
    let text = ".xhir version 0\nfunction Main\nsignature\nreturns Int\n.end\nlocals\nlocal answer: Int \
                immutable\n.end\nbody\nreturn\nliteral integer 42\n.end\n.program end\n";

    let parsed = parse_xhir_function(text).expect("XHIR should not be indentation based");

    assert_eq!(parsed.name, "Main");
    assert_eq!(parsed.return_type, Some(Type::Primitive(PrimitiveType::Int)));
    assert_eq!(parsed.locals.len(), 1);
    assert_eq!(parsed.body.len(), 1);
  }

  #[test]
  fn rejects_invalid_xhir_module_symbols()
  {
    let diagnostics = parse_xhir_module_symbols(".xhir version 0\nmodule App\n\ndeclarations\n  symbol Main\n    \
                                                 kind nope\n").expect_err("invalid symbol kind should fail");

    assert_eq!(diagnostics.len(), 2);
  }

  #[test]
  fn rejects_invalid_xhir_function()
  {
    let diagnostics =
      parse_xhir_function(".xhir version 0\nfunction Main\n  signature\n    returns ?\n").expect_err("unknown return \
                                                                                                      type should \
                                                                                                      fail");

    assert_eq!(diagnostics.len(), 1);
  }

  #[test]
  fn roundtrips_typecheck_analysis()
  {
    let diagnostics = vec![Diagnostic { code: DiagnosticCode::LiteralTypeMismatch,
                                        message: "literal is not assignable".to_string(),
                                        span: span() },
                           Diagnostic { code: DiagnosticCode::UnknownLocal,
                                        message: "unknown local 'value'".to_string(),
                                        span: Span::new(1, 4, 8) }];

    let text = typecheck_analysis_to_xhir(&diagnostics);
    let parsed = parse_xhir_typecheck_analysis(&text).expect("typecheck analysis should parse");

    assert_eq!(parsed, diagnostics);
    assert!(text.contains("analysis typecheck"));
    assert!(!text.contains(".diagnostic"));
  }

  #[test]
  fn rejects_invalid_typecheck_analysis()
  {
    let diagnostics = parse_xhir_typecheck_analysis("analysis typecheck\n  diagnostic nope\n    span bad\n    \
                                                     message broken\n").expect_err("invalid analysis must fail");

    assert_eq!(diagnostics.len(), 2);
  }
}
