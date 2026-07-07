/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use std::fmt::Write;

use super::symbols::{Import, Module, SymbolKind, Visibility};
use super::type_check::{Expression, Function, Literal, PrimitiveType, Statement, Type};

pub mod parser;

pub use parser::{XhirParseDiagnostic, parse_xhir_function, parse_xhir_module_symbols};

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
  let version = lines.next()?.strip_prefix("xhir version ")?.parse().ok()?;
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
  let _ = writeln!(output, "xhir version 0");
  let _ = writeln!(output, "module {}", module.name);
  if !module.imports.is_empty()
  {
    let _ = writeln!(output);
    let _ = writeln!(output, "imports");
    for import in &module.imports
    {
      write_import(&mut output, import);
    }
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
  }
  output
}

#[must_use]
pub fn function_to_xhir(function: &Function) -> String
{
  let mut output = String::new();
  let _ = writeln!(output, "xhir version 0");
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
  }
  if !function.body.is_empty()
  {
    let _ = writeln!(output, "  body");
    for statement in &function.body
    {
      write_statement(&mut output, statement, 2);
    }
  }
  output
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

fn write_statement(output: &mut String, statement: &Statement, indent: usize)
{
  let pad = "  ".repeat(indent);
  match statement
  {
    Statement::Let { local,
                     initializer, } =>
    {
      let mutability = if local.mutable
      {
        "mutable"
      }
      else
      {
        "immutable"
      };
      let _ = writeln!(output, "{pad}let {}", local.name);
      let _ = writeln!(output, "{pad}  type {}", type_name(&local.ty));
      let _ = writeln!(output, "{pad}  mutability {mutability}");
      if let Some(initializer) = initializer
      {
        let _ = writeln!(output, "{pad}  initializer");
        write_expression(output, initializer, indent + 2);
      }
    }
    Statement::Expr(expression) =>
    {
      let _ = writeln!(output, "{pad}expression");
      write_expression(output, expression, indent + 1);
    }
    Statement::Return { value, .. } =>
    {
      let _ = writeln!(output, "{pad}return");
      if let Some(value) = value
      {
        write_expression(output, value, indent + 1);
      }
    }
  }
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
    Literal::Nil => "nil".to_string(),
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
    PrimitiveType::Bool => "bool",
    PrimitiveType::I16 => "i16",
    PrimitiveType::I32 => "i32",
    PrimitiveType::I64 => "i64",
    PrimitiveType::I128 => "i128",
    PrimitiveType::U8 => "u8",
    PrimitiveType::I8 => "i8",
    PrimitiveType::U16 => "u16",
    PrimitiveType::U32 => "u32",
    PrimitiveType::U64 => "u64",
    PrimitiveType::U128 => "u128",
    PrimitiveType::F32 => "f32",
    PrimitiveType::F64 => "f64",
    PrimitiveType::Char => "char",
    PrimitiveType::Str => "str",
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
  use crate::hir::type_check::Local;

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

    assert!(text.contains("xhir version 0\nmodule App"));
    assert!(text.contains("import println from std.io"));
    assert!(text.contains("symbol Main\n    kind function\n    visibility public"));
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
                              return_type: Some(Type::Primitive(PrimitiveType::I64)),
                              locals: vec![Local { name: "answer".to_string(),
                                                   ty: Type::Primitive(PrimitiveType::I64),
                                                   mutable: false,
                                                   span: span() }],
                              body: vec![Statement::Return { value: Some(Expression::Literal { literal:
                                                                                    Literal::Integer("42".to_string()),
                                                                                  span: span() }),
                                                span: span() }] };

    let text = function_to_xhir(&function);

    assert!(text.contains("function Main"));
    assert!(text.contains("returns i64"));
    assert!(text.contains("local answer: i64 immutable"));
    assert!(text.contains("literal integer 42"));
    assert!(!text.contains("bb0"));

    let header = parse_xhir_header(&text).expect("header should parse");
    assert_eq!(header.kind, XhirDocumentKind::Function);
    assert_eq!(header.name, "Main");

    let parsed = parse_xhir_function(&text).expect("function should parse");
    assert_eq!(parsed.name, "Main");
    assert_eq!(parsed.return_type, Some(Type::Primitive(PrimitiveType::I64)));
    assert_eq!(parsed.locals.len(), 1);
    assert_eq!(parsed.body.len(), 1);
  }

  #[test]
  fn rejects_non_xhir_header()
  {
    assert_eq!(parse_xhir_header(".func Main : () -> void\n"), None);
  }

  #[test]
  fn rejects_invalid_xhir_module_symbols()
  {
    let diagnostics = parse_xhir_module_symbols("xhir version 0\nmodule App\n\ndeclarations\n  symbol Main\n    kind \
                                                 nope\n").expect_err("invalid symbol kind should fail");

    assert_eq!(diagnostics.len(), 2);
  }

  #[test]
  fn rejects_invalid_xhir_function()
  {
    let diagnostics =
      parse_xhir_function("xhir version 0\nfunction Main\n  signature\n    returns ?\n").expect_err("unknown return \
                                                                                                     type should fail");

    assert_eq!(diagnostics.len(), 1);
  }
}
