/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use crate::hir::async_check::Span;
use crate::hir::symbols::{Import, Module, Symbol, SymbolKind, Visibility};
use crate::hir::type_check::{Expression, Function, Literal, Local, PrimitiveType, Statement, Type};

use super::{SUPPORTED_XHIR_VERSION, is_supported_xhir_version};

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct XhirParseDiagnostic
{
  pub line: usize,
  pub message: String,
}

#[must_use]
pub fn parse_xhir_module_symbols(text: &str) -> Result<Module, Vec<XhirParseDiagnostic>>
{
  let mut parser = Parser { lines: text.lines().collect(),
                            index: 0,
                            diagnostics: Vec::new() };
  let module = parser.module_symbols();
  if parser.diagnostics.is_empty()
  {
    Ok(module)
  }
  else
  {
    Err(parser.diagnostics)
  }
}

#[must_use]
pub fn parse_xhir_function(text: &str) -> Result<Function, Vec<XhirParseDiagnostic>>
{
  let mut parser = Parser { lines: text.lines().collect(),
                            index: 0,
                            diagnostics: Vec::new() };
  let function = parser.function();
  if parser.diagnostics.is_empty()
  {
    Ok(function)
  }
  else
  {
    Err(parser.diagnostics)
  }
}

struct Parser<'a>
{
  lines: Vec<&'a str>,
  index: usize,
  diagnostics: Vec<XhirParseDiagnostic>,
}

impl Parser<'_>
{
  fn module_symbols(&mut self) -> Module
  {
    self.xhir_version();
    let name = self.module_name();
    let mut module = Module { name,
                              imports: Vec::new(),
                              symbols: Vec::new() };
    while let Some(line) = self.next_non_empty()
    {
      match line.as_str()
      {
        "imports" => self.imports(&mut module),
        "declarations" => self.declarations(&mut module),
        other =>
        {
          self.report(format!("unexpected XHIR section '{other}'"));
          self.index += 1;
        }
      }
    }
    module
  }

  fn function(&mut self) -> Function
  {
    self.xhir_version();
    let name = self.function_name();
    let mut function = Function { name,
                                  return_type: None,
                                  locals: Vec::new(),
                                  body: Vec::new() };
    while let Some(line) = self.next_non_empty()
    {
      match line.as_str()
      {
        "  signature" => self.signature(&mut function),
        "  locals" => self.locals(&mut function),
        "  body" => self.body(&mut function),
        other =>
        {
          self.report(format!("unexpected XHIR function section '{other}'"));
          self.index += 1;
        }
      }
    }
    function
  }

  fn module_name(&mut self) -> String
  {
    let Some(line) = self.current()
    else
    {
      self.report("missing module declaration".to_string());
      return String::new();
    };
    let Some(name) = line.strip_prefix("module ")
    else
    {
      self.report("expected module declaration".to_string());
      return String::new();
    };
    self.index += 1;
    name.to_string()
  }

  fn function_name(&mut self) -> String
  {
    let Some(line) = self.current()
    else
    {
      self.report("missing function declaration".to_string());
      return String::new();
    };
    let Some(name) = line.strip_prefix("function ")
    else
    {
      self.report("expected function declaration".to_string());
      return String::new();
    };
    self.index += 1;
    name.to_string()
  }

  fn signature(&mut self, function: &mut Function)
  {
    self.index += 1;
    let Some(line) = self.current()
    else
    {
      self.report("missing function return type".to_string());
      return;
    };
    let Some(name) = line.strip_prefix("    returns ")
    else
    {
      self.report("expected function return type".to_string());
      return;
    };
    self.index += 1;
    function.return_type = if name == "void"
    {
      None
    }
    else
    {
      self.parse_type(&name)
    };
  }

  fn locals(&mut self, function: &mut Function)
  {
    self.index += 1;
    while let Some(line) = self.current()
    {
      let Some(rest) = line.strip_prefix("    local ")
      else
      {
        break;
      };
      match parse_local_record(&rest)
      {
        Some(local) => function.locals.push(local),
        None => self.report(format!("invalid local record '{line}'")),
      }
      self.index += 1;
    }
  }

  fn body(&mut self, function: &mut Function)
  {
    self.index += 1;
    while let Some(line) = self.current()
    {
      match line.as_str()
      {
        line if line.starts_with("    let ") => function.body.push(self.let_statement()),
        "    expression" => function.body.push(self.expression_statement()),
        "    return" => function.body.push(self.return_statement()),
        _ => break,
      }
    }
  }

  fn let_statement(&mut self) -> Statement
  {
    let name = self.current()
                   .and_then(|line| line.strip_prefix("    let ").map(ToString::to_string))
                   .unwrap_or_default();
    self.index += 1;
    let ty = self.local_type();
    let mutable = self.local_mutability();
    let initializer = if self.current().as_deref() == Some("      initializer")
    {
      self.index += 1;
      self.expression_with_indent("        ")
    }
    else
    {
      None
    };
    Statement::Let { local: Local { name,
                                    ty,
                                    mutable,
                                    span: span() },
                     initializer }
  }

  fn expression_statement(&mut self) -> Statement
  {
    self.index += 1;
    Statement::Expr(self.expression_with_indent("      ")
                        .unwrap_or(Expression::Literal { literal: Literal::None,
                                                         span: span() }))
  }

  fn return_statement(&mut self) -> Statement
  {
    self.index += 1;
    let value = self.expression_with_indent("      ");
    Statement::Return { value,
                        span: span() }
  }

  fn expression_with_indent(&mut self, indent: &str) -> Option<Expression>
  {
    let line = self.current()?;
    let rest = line.strip_prefix(indent)?;
    if let Some(value) = rest.strip_prefix("literal ")
    {
      self.index += 1;
      return Some(Expression::Literal { literal: self.literal(value),
                                        span: span() });
    }
    if let Some(name) = rest.strip_prefix("local ")
    {
      self.index += 1;
      return Some(Expression::Local { name: name.to_string(),
                                      span: span() });
    }
    if let Some(target) = rest.strip_prefix("assign ")
    {
      self.index += 1;
      let nested_indent = format!("{indent}  ");
      let value = self.expression_with_indent(&nested_indent)
                      .unwrap_or(Expression::Literal { literal: Literal::None,
                                                       span: span() });
      return Some(Expression::Assign { target: target.to_string(),
                                       value: Box::new(value),
                                       span: span() });
    }
    None
  }

  fn local_type(&mut self) -> Type
  {
    let Some(line) = self.current()
    else
    {
      self.report("missing local type".to_string());
      return Type::Named(String::new());
    };
    self.index += 1;
    let Some(name) = line.strip_prefix("      type ")
    else
    {
      self.report("expected local type".to_string());
      return Type::Named(String::new());
    };
    self.parse_type(&name).unwrap_or(Type::Named(name.to_string()))
  }

  fn local_mutability(&mut self) -> bool
  {
    let Some(line) = self.current()
    else
    {
      self.report("missing local mutability".to_string());
      return false;
    };
    self.index += 1;
    match line.strip_prefix("      mutability ")
    {
      Some("mutable") => true,
      Some("immutable") => false,
      Some(value) =>
      {
        self.report(format!("unknown mutability '{value}'"));
        false
      }
      None =>
      {
        self.report("expected local mutability".to_string());
        false
      }
    }
  }

  fn parse_type(&mut self, name: &str) -> Option<Type>
  {
    if let Some(primitive) = primitive_type(name)
    {
      return Some(Type::Primitive(primitive));
    }
    if is_named_type(name)
    {
      return Some(Type::Named(name.to_string()));
    }
    self.report(format!("unknown type '{name}'"));
    None
  }

  fn literal(&mut self, text: &str) -> Literal
  {
    if let Some(value) = text.strip_prefix("bool ")
    {
      return Literal::Bool(value == "true");
    }
    if let Some(value) = text.strip_prefix("integer ")
    {
      return Literal::Integer(value.to_string());
    }
    if let Some(value) = text.strip_prefix("float ")
    {
      return Literal::Float(value.to_string());
    }
    if let Some(value) = text.strip_prefix("char ")
    {
      return Literal::Char(value.chars().next().unwrap_or('\0'));
    }
    if let Some(value) = text.strip_prefix("string ")
    {
      return Literal::String(value.trim_matches('"').to_string());
    }
    Literal::None
  }

  fn imports(&mut self, module: &mut Module)
  {
    self.index += 1;
    while let Some(line) = self.current()
    {
      if line.is_empty()
      {
        self.index += 1;
        continue;
      }
      if !line.starts_with("  import ")
      {
        break;
      }
      if let Some(import) = parse_import_line(&line)
      {
        module.imports.push(import);
      }
      else
      {
        self.report(format!("invalid import record '{line}'"));
      }
      self.index += 1;
    }
  }

  fn declarations(&mut self, module: &mut Module)
  {
    self.index += 1;
    while let Some(line) = self.current()
    {
      if line.is_empty()
      {
        self.index += 1;
        continue;
      }
      let Some(name) = line.strip_prefix("  symbol ")
      else
      {
        break;
      };
      self.index += 1;
      let kind = self.symbol_kind();
      let visibility = self.visibility();
      module.symbols.push(Symbol { name: name.to_string(),
                                   kind,
                                   visibility,
                                   span: span() });
    }
  }

  fn symbol_kind(&mut self) -> SymbolKind
  {
    let Some(line) = self.current()
    else
    {
      self.report("missing symbol kind".to_string());
      return SymbolKind::Function;
    };
    self.index += 1;
    let Some(name) = line.strip_prefix("    kind ")
    else
    {
      self.report("expected symbol kind".to_string());
      return SymbolKind::Function;
    };
    match name
    {
      "function" => SymbolKind::Function,
      "class" => SymbolKind::Class,
      "interface" => SymbolKind::Interface,
      "enum" => SymbolKind::Enum,
      "data" => SymbolKind::Data,
      "macro" => SymbolKind::Macro,
      _ =>
      {
        self.report(format!("unknown symbol kind '{name}'"));
        SymbolKind::Function
      }
    }
  }

  fn visibility(&mut self) -> Visibility
  {
    let Some(line) = self.current()
    else
    {
      self.report("missing symbol visibility".to_string());
      return Visibility::Private;
    };
    self.index += 1;
    let Some(name) = line.strip_prefix("    visibility ")
    else
    {
      self.report("expected symbol visibility".to_string());
      return Visibility::Private;
    };
    match name
    {
      "public" => Visibility::Public,
      "internal" => Visibility::Internal,
      "private" => Visibility::Private,
      _ =>
      {
        self.report(format!("unknown visibility '{name}'"));
        Visibility::Private
      }
    }
  }

  fn xhir_version(&mut self)
  {
    let Some(line) = self.current()
    else
    {
      self.report("missing XHIR version header".to_string());
      return;
    };
    let Some(version) = line.strip_prefix(".xhir version ")
    else
    {
      self.report("expected XHIR version header".to_string());
      self.index += 1;
      return;
    };
    match version.parse::<u32>()
    {
      Ok(version) if is_supported_xhir_version(version) =>
      {}
      Ok(version) =>
      {
        self.report(format!("unsupported XHIR version {version}; supported version is {SUPPORTED_XHIR_VERSION}"))
      }
      Err(_) => self.report("invalid XHIR version number".to_string()),
    }
    self.index += 1;
  }

  fn next_non_empty(&mut self) -> Option<String>
  {
    while self.current().as_deref() == Some("")
    {
      self.index += 1;
    }
    self.current()
  }

  fn current(&self) -> Option<String>
  {
    self.lines.get(self.index).map(|line| (*line).to_string())
  }

  fn report(&mut self, message: String)
  {
    self.diagnostics.push(XhirParseDiagnostic { line: self.index + 1,
                                                message });
  }
}

fn parse_import_line(line: &str) -> Option<Import>
{
  let rest = line.strip_prefix("  import ")?;
  if let Some(module) = rest.strip_prefix("module ")
  {
    return Some(Import::Module { module: module.to_string(),
                                 span: span() });
  }
  if let Some(module) = rest.strip_prefix("all from ")
  {
    return Some(Import::All { module: module.to_string(),
                              span: span() });
  }
  let (left, module) = rest.rsplit_once(" from ")?;
  if let Some((name, alias)) = left.split_once(" as ")
  {
    return Some(Import::Selected { module: module.to_string(),
                                   name: name.to_string(),
                                   alias: Some(alias.to_string()),
                                   span: span() });
  }
  Some(Import::Selected { module: module.to_string(),
                          name: left.to_string(),
                          alias: None,
                          span: span() })
}

fn parse_local_record(text: &str) -> Option<Local>
{
  let (name, rest) = text.split_once(": ")?;
  let (ty, mutability) = rest.rsplit_once(' ')?;
  Some(Local { name: name.to_string(),
               ty: primitive_type(ty).map_or_else(|| Type::Named(ty.to_string()), Type::Primitive),
               mutable: mutability == "mutable",
               span: span() })
}

fn primitive_type(name: &str) -> Option<PrimitiveType>
{
  match name
  {
    "Bool" => Some(PrimitiveType::Bool),
    "Byte" => Some(PrimitiveType::Byte),
    "SByte" => Some(PrimitiveType::SByte),
    "Char" => Some(PrimitiveType::Char),
    "Short" => Some(PrimitiveType::Short),
    "Long" => Some(PrimitiveType::Long),
    "Int" => Some(PrimitiveType::Int),
    "Integer" => Some(PrimitiveType::Integer),
    "UShort" => Some(PrimitiveType::UShort),
    "ULong" => Some(PrimitiveType::ULong),
    "UInt" => Some(PrimitiveType::UInt),
    "UInteger" => Some(PrimitiveType::UInteger),
    "SFloat" => Some(PrimitiveType::SFloat),
    "Float" => Some(PrimitiveType::Float),
    "Str" => Some(PrimitiveType::Str),
    _ => None,
  }
}

fn is_named_type(name: &str) -> bool
{
  let mut chars = name.chars();
  let Some(first) = chars.next()
  else
  {
    return false;
  };
  (first == '_' || first.is_alphabetic()) && chars.all(|c| c == '_' || c == '.' || c.is_alphanumeric())
}

const fn span() -> Span
{
  Span::new(0, 0, 0)
}
